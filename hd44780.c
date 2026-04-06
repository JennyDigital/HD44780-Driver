/*  HD44780-Driver  A display driver for the HD44780 based displays.
    Copyright (C) 2024 Jennifer Gunn (JennyDigital).

	jennifer.a.gunn@outlook.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
    USA
*/

#include "main.h"
#include "stdio.h"
#include "hardware.h"
#include "hd44780.h"


// Private System Control function prototypes.
//
static inline void LCD_Command      ( uint8_t cmd );
static inline void LCD_BusyWait     ( void );

// Hardware Abstraction Layer Functions
//
static inline void    LCD_SetRS       ( uint8_t state );
static inline void    LCD_SetRNW      ( uint8_t state );
static inline void    LCD_SetE        ( uint8_t state );
static inline void    LCD_SetBusInput ( void );
static inline uint8_t LCD_Input       ( void );
static inline void    LCD_OutputUpperNibble( uint8_t ch );
#ifdef LCD_BUS8BIT
static inline void    LCD_OutputLowerNibble( uint8_t ch );
#endif
static inline void    LCD_Output      ( uint8_t ch );
static inline uint8_t LCD_IsBusy      ( void );

/** Different platforms require different delay solutions, so
  * so below is a macro to substitute your own,
  */
#ifdef _HW_INTERFACE_GD32_H
#define Delay_ms( ms_delay ) delay_millis( ms_delay )
#endif

#ifdef _HW_INTERFACE_STM32_H
#define Delay_ms( ms_delay ) HAL_Delay( ms_delay )
#endif

/* HD44780 bootstrap values for 4-bit initialization sequence. */
#define LCD_INIT_PWRON_DELAY_MS       15
#define LCD_INIT_WAKE_DELAY_MS         5
#define LCD_INIT_STEP_DELAY_MS         1
#define LCD_INIT_WAKE_NIBBLE        0x30
#define LCD_INIT_SET_4BIT_NIBBLE    0x20


/** HD44780 system variables
  *
  */
static          uint8_t   hd_xpos   = 0,
                          hd_ypos   = 0;
static          uint8_t   dd_addr;
static const    uint8_t   hd_map[]  = HD_ADDR_MAP;


/** VFD has four different intensities VFD25 VFD50 VFD75 and VFD100
  *
  * Brightness will vary from 25% to 100%
  */
#ifdef HD_ISVFD
char vfd_intensity = VFD100;
#endif

 
// HACK: Time wasting routine.  It was being optimised away until it's parameters were declared
// volatile.  This comes from use in CCS C which has this as a built-in, but they mean machine
// cycles.
//
void delay_cycles( volatile uint8_t cycles_to_waste )
{
  while( cycles_to_waste-- );
}


/** Set the RS pin state
  *
  * @param state: 1 = Data Register, 0 = Instruction Register
  * @retval none
  */
static inline void LCD_SetRS(uint8_t state)
{
  Output_Pin( LCD_RS, LCD_RS_BANK, state );
  delay_cycles( E_CYCLES );
}


/** Set the Read/Write pin state
  *
  * @param state: 1 = Read, 0 = Write
  * @retval none
  */
static inline void LCD_SetRNW(uint8_t state)
{
  Output_Pin( LCD_RNW, LCD_RNW_BANK, state );
  delay_cycles( E_CYCLES );
}


/** Set the Enable pin state
  *
  * @param state: 1 = Enable, 0 = Disable
  * @retval none
  */
static inline void LCD_SetE( uint8_t state )
{
  Output_Pin( LCD_E, LCD_E_BANK, state );
  delay_cycles( E_CYCLES );
}


/* Set the LCD bus to input mode 
 *
 * @param none
 * @retval none
 */
static inline void LCD_SetBusInput( void )
{
  Set_Input_Pin( LCD_D7, LCD_D7_BANK );
  Set_Input_Pin( LCD_D6, LCD_D6_BANK );
  Set_Input_Pin( LCD_D5, LCD_D5_BANK );
  Set_Input_Pin( LCD_D4, LCD_D4_BANK );

#ifdef LCD_BUS8BIT
  Set_Input_Pin( LCD_D3, LCD_D3_BANK );
  Set_Input_Pin( LCD_D2, LCD_D2_BANK );
  Set_Input_Pin( LCD_D1, LCD_D1_BANK );
  Set_Input_Pin( LCD_D0, LCD_D0_BANK );
#endif
}


/** Read the LCD bus input  
  *
  * @param none
  * @retval byte read from the LCD bus
  */
static inline uint8_t LCD_Input( void )
{
  uint8_t ch;

  LCD_SetBusInput();

  ch  = Read_Pin( LCD_D7, LCD_D7_BANK ); ch <<= 1;
  ch |= Read_Pin( LCD_D6, LCD_D6_BANK ); ch <<= 1;
  ch |= Read_Pin( LCD_D5, LCD_D5_BANK ); ch <<= 1;
  ch |= Read_Pin( LCD_D4, LCD_D4_BANK );

#ifdef LCD_BUS8BIT
  ch <<= 1;
  ch |= Read_Pin( LCD_D3, LCD_D3_BANK ); ch <<= 1;
  ch |= Read_Pin( LCD_D2, LCD_D2_BANK ); ch <<= 1;
  ch |= Read_Pin( LCD_D1, LCD_D1_BANK ); ch <<= 1;
  ch |= Read_Pin( LCD_D0, LCD_D0_BANK );
#endif

  return ch;
}


/* Output to LCD bus
 * @param ch: byte to output to the LCD bus
 *
 * @retval none
 */
static inline void LCD_Output( uint8_t ch )
{
  LCD_OutputUpperNibble( ch );
#ifdef LCD_BUS8BIT
  LCD_OutputLowerNibble( ch );
#endif
}


/* Output only the upper nibble (D7..D4) to the LCD bus.
 * Useful during 4-bit startup when the controller has not yet latched 4-bit mode.
 */
static inline void LCD_OutputUpperNibble( uint8_t ch )
{
  Output_Pin( LCD_D7, LCD_D7_BANK, ch & 0x80 );
  Output_Pin( LCD_D6, LCD_D6_BANK, ch & 0x40 );
  Output_Pin( LCD_D5, LCD_D5_BANK, ch & 0x20 );
  Output_Pin( LCD_D4, LCD_D4_BANK, ch & 0x10 );
}


#ifdef LCD_BUS8BIT
/* Output only the lower nibble (D3..D0) to the LCD bus. */
static inline void LCD_OutputLowerNibble( uint8_t ch )
{
  Output_Pin( LCD_D3, LCD_D3_BANK, ch & 0x08 );
  Output_Pin( LCD_D2, LCD_D2_BANK, ch & 0x04 );
  Output_Pin( LCD_D1, LCD_D1_BANK, ch & 0x02 );
  Output_Pin( LCD_D0, LCD_D0_BANK, ch & 0x01 );
}
#endif


/** Read the busy flag from the LCD
  *
  * @param none
  * @retval uint8_t: busy flag state
  */
static inline uint8_t LCD_IsBusy( void )
{
  uint8_t busybit;

  /* Prep LCD for busy flag read */
  LCD_SetBusInput();
  LCD_SetRS( INSTR_REG );
  LCD_SetRNW( READ );
  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );

  /* Read busy flag */
  busybit = Read_Pin( LCD_D7, LCD_D7_BANK );

  /* Turn off the Enable pin */
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

  /* Do it again for 4-bit mode, ignoring the result */
#ifndef LCD_BUS8BIT
  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );
  
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );
#endif
  
  return busybit;
}


#ifdef LCD_READ_DD_SUPPORT

/** Read Display Data RAM at the specified address
  *
  * @param dd_read_addr: address to read from
  * @retval uint8_t: data read from DDRAM
  */
uint8_t LCD_Read_DDRAM( uint8_t dd_read_addr )
{
  uint8_t dd_data;

  /* Set DDRAM address to read from */
  LCD_Command( SET_DDRAM_ADD | dd_read_addr );
  LCD_BusyWait();
  LCD_SetRS( DATA_REG );
  LCD_SetRNW( READ );

  /* Prepare LCD for data read */
  delay_cycles( E_CYCLES );
  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );

  /* Read data */
  dd_data = LCD_Input();

  /* Turn off Enable pin */
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

/* Read second nibble for 4-bit mode */
#ifdef LCD_BUS4BIT
  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );
  dd_data <<= 4;
  dd_data |= LCD_Input();
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );
#endif  // LCD_READ_DD_SUPPORT 4-bit mode

  return dd_data;
}
#endif // LCD_READ_DD_SUPPORT


#ifdef LCD_READCHAR_SUPPORT

/** Read character at the display coordinates specified, with
  * (0,0) being the top left of the display.
  *
  * @param rc_x: X coordinate to read from
  * @param rc_y: Y coordinate to read from
  * @retval uint8_t: character read from the specified location
  */
uint8_t LCD_Readchar( uint8_t rc_x, uint8_t rc_y )
{
  int addr_to_sample;

  addr_to_sample = LCD_DDRAM_Addr( rc_x, rc_y );

  return ( LCD_Read_DDRAM( addr_to_sample ) );
}

#endif


/** Send a command to the LCD
  *
  * @param cmd: command byte to send; see HD44780 datasheet for details.
  * @retval none
  */
static inline void LCD_Command( uint8_t cmd )
{
  LCD_SetRS( INSTR_REG );
  LCD_SetRNW( WRITE );

  delay_cycles( E_CYCLES );
  LCD_Output( cmd );
  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

#ifdef LCD_BUS4BIT

  LCD_Output( cmd << 4 );
  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

#endif

  LCD_Input();
  LCD_BusyWait();

}


/** Waits until the LCD is no longer busy
  *
  * @param none
  * @retval none
  */
static inline void LCD_BusyWait( void )
{
  while( LCD_IsBusy() );
}


#ifdef LCD_UDG_SUPPORT

/** Define a user-defined character
  *
  * @param ChToSet: character code to define (0-7)
  * @param ChDataset: pointer to character data (8 bytes, read-only)
  * @retval none
  */
void LCD_Defchar( uint16_t ChToSet, const uint8_t * ChDataset )
{
  uint16_t ChAddress,
      ch_line,
      defchar_dd_addr;
   
  ChAddress = ChToSet << 3;                     // Calculate address to UDG
  
  LCD_SetRS( INSTR_REG );
  LCD_SetRNW( READ );
  LCD_SetE( ENABLE );
  defchar_dd_addr = LCD_Input() & 0b1111111;
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

#ifdef LCD_BUS4BIT

  defchar_dd_addr <<= 4;
  LCD_SetE( ENABLE );
  defchar_dd_addr |= LCD_Input() & 0b1111111;
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

#endif

  LCD_BusyWait();

  LCD_Command(SET_CGRAM_ADD | ChAddress );
  
  for( ch_line = 0; ch_line < 8; ch_line++ )
  {
    LCD_PutData( *ChDataset );
    ChDataset++;
  }
  
  LCD_BusyWait();
  LCD_Command( SET_DDRAM_ADD | defchar_dd_addr );
  LCD_BusyWait();
}

#endif


/** Move the cursor to the specified coordinates
  *
  * @param x: X coordinate (0 to XMAX)
  * @param y: Y coordinate (0 to YMAX)
  * @retval none
  */
void LCD_Locate( uint8_t x, uint8_t y )
{
  uint8_t addr = LCD_DDRAM_Addr( x, y );
  hd_xpos = x; hd_ypos = y;
  
  LCD_Command( SET_DDRAM_ADD | addr );
}


/** Write data to the LCD
  *
  * @param dat: data byte to write
  * @retval none
  */
void LCD_PutData( uint8_t dat )
{
    LCD_BusyWait();
    LCD_SetRS( DATA_REG );
    LCD_SetRNW( WRITE );
    delay_cycles( E_CYCLES );
    LCD_Output( dat );
    LCD_SetE( ENABLE );
    delay_cycles( E_CYCLES );
    LCD_SetE( DISABLE );
    delay_cycles( E_CYCLES );

#ifdef LCD_BUS4BIT

    LCD_Output( dat << 4 );
    LCD_SetE( ENABLE );
    delay_cycles( E_CYCLES );
    LCD_SetE( DISABLE );
    delay_cycles( E_CYCLES );

#endif

    LCD_Input();

}



/** Scroll the display up one line, this is only available if enabled.
  *
  * @param none
  * @retval none
  */
#ifdef LCD_SCROLL_SUPPORT

void LCD_ScrollUp( void )
{
  uint8_t  line_pos,
        line,
        ch_moving,
        new_addr;

/* Don't scroll if there is only one line */
   if( YMAX == 0 ) return;

/* Make a copy one line up */
  for( line = 1; line <= YMAX; line++)
    for( line_pos = 0; line_pos <= XMAX; line_pos++ )
    {
      ch_moving = LCD_Readchar( line_pos, line );
      new_addr = LCD_DDRAM_Addr( line_pos, line - 1 );
      LCD_Command( SET_DDRAM_ADD | new_addr );
      LCD_PutData( ch_moving );
    }
  
  /* Clear the last line */
  for(line_pos = 0; line_pos <= XMAX; line_pos++)
  {
    LCD_Command(
                SET_DDRAM_ADD |
                LCD_DDRAM_Addr( line_pos, YMAX )
               );
    LCD_PutData( 0x20 ); 
  }
}
#endif


/** Get the DDRAM address for the specified coordinates
  *
  * @param dd_x: X coordinate (0 to XMAX)
  * @param dd_y: Y coordinate (0 to YMAX)
  * @retval uint8_t: DDRAM address for the specified coordinates
  */
uint8_t LCD_DDRAM_Addr( uint8_t dd_x, uint8_t dd_y )
{  
  /* Clamp coordinates to valid range to avoid indexing hd_map out-of-bounds. */
  if( dd_x > XMAX ) dd_x = XMAX;
  if( dd_y > YMAX ) dd_y = YMAX;

  return ( hd_map[ dd_x + ( XMAX + 1 ) * dd_y ] );
}



#define LCD_Putc( ch_to_put ) LCD_Putchar( ch_to_put );

/* Non stdio.h version of putchar 
 * for use with compilers like CCS C
 *
 * @brief Write a character to the LCD
 *
 * @param ch_to_put: character to write
 * @retval none
 */
uint8_t LCD_Putchar( uint8_t ch )
{
  if( hd_xpos > XMAX )
  {
    hd_xpos = 0;
    hd_ypos++;
  }

  if( hd_ypos > YMAX )
#ifdef LCD_SCROLL_SUPPORT
    {
      hd_ypos = YMAX;
      LCD_ScrollUp();
    }
#else   // The alternative is to wrap around and overwrite. Meh!
    {
      hd_ypos = 0;
    }
#endif
     
  dd_addr = LCD_DDRAM_Addr( hd_xpos, hd_ypos );
  
  switch( ch )
  {
    case '\n':
      hd_ypos++;
#ifdef HD_NL_DOES_CR
      hd_xpos = 0;
#endif
      if( hd_ypos > YMAX )
#ifdef LCD_SCROLL_SUPPORT
      {
        LCD_ScrollUp();
        hd_ypos = YMAX;
      }
#else // The alternative is to wrap around and overwrite. Meh!
      {
        hd_xpos=0;
        hd_ypos=0;
      }
#endif
      LCD_Command( SET_DDRAM_ADD | LCD_DDRAM_Addr( hd_xpos, hd_ypos ) );
      break;

    case '\r':
      {
        hd_xpos = 0;
        LCD_Command( SET_DDRAM_ADD | LCD_DDRAM_Addr( hd_xpos, hd_ypos ) );
        break;
      }

    default:
      LCD_Command( SET_DDRAM_ADD | dd_addr );
      LCD_PutData( ch );
      hd_xpos++;

#ifdef LCD_SCROLL_SUPPORT
      if( hd_ypos > YMAX )
      {
        LCD_ScrollUp();
        hd_ypos = YMAX;
      }
#else
    if( hd_ypos > YMAX )
    {
      /* Wrap to top if scrolling is not supported */
      hd_ypos = 0;
    }
#endif

    LCD_Command( SET_DDRAM_ADD | LCD_DDRAM_Addr( hd_xpos, hd_ypos ) );
  }
  return ch;
}

/* CrossWorks ARM printf support */
#ifdef __CROSSWORKS_ARM
int __putchar(int ch, __printf_tag_ptr ptr)
{
  return LCD_Putchar( ch );
}
#endif

/* GNU putchar support. (Some might prefer _write instead) */
#if defined(__GNUC__) && !defined(__CROSSWORKS_ARM)

PUTCHAR_PROTOTYPE
{
  LCD_Putchar( ch );
  return ch;
}
#endif


/** Clear the LCD display
  *
  * @param none
  * @retval none
  */
void LCD_Clear(void)
{
  LCD_BusyWait();
  LCD_Command(CLR_DISP);
  LCD_BusyWait();
  LCD_Locate( 0, 0 );
}


/** Set the cursor state
  *
  * @param cursor_state: 1 = Blinking cursor, 0 = No cursor.
  * @retval none
  */
void LCD_Cursor( uint8_t cursor_state )
{
  LCD_Command( 
              DISP_CTRL | DISP | ( cursor_state ? BLINK : 0 )
             );
}


/** Initialize the LCD
  *
  * @brief  Initializes the HD44780 LCD display connected to
  *         the microcontroller.  This must be called before any
  *         other LCD functions are used.
  *
  * @param none
  * @retval none
  */
void LCD_Init(void)
{
  /*Stops buffering which breaks this driver outright */
#ifdef __GNUC__
  setvbuf( stdout, NULL, _IONBF, 0 ); // No Buffering
#endif

  /*Initialise the LCD pins */
  LCD_Input();
  LCD_SetE( DISABLE );
  LCD_SetRS( INSTR_REG );
  LCD_SetRNW( WRITE );

  /* Wait for more than 15 ms after VCC rises to 4.5V */
  Delay_ms( LCD_INIT_PWRON_DELAY_MS );

  /* 4-bit wake-up sequence from HD44780 datasheet: 0x3,0x3,0x3,0x2 on D7..D4. */
#ifdef LCD_BUS4BIT
  LCD_OutputUpperNibble( LCD_INIT_WAKE_NIBBLE );
  LCD_SetE( ENABLE );
  LCD_SetE( DISABLE );
  Delay_ms( LCD_INIT_WAKE_DELAY_MS );

  LCD_OutputUpperNibble( LCD_INIT_WAKE_NIBBLE );
  LCD_SetE( ENABLE );
  LCD_SetE( DISABLE );
  Delay_ms( LCD_INIT_STEP_DELAY_MS );

  LCD_OutputUpperNibble( LCD_INIT_WAKE_NIBBLE );
  LCD_SetE( ENABLE );
  LCD_SetE( DISABLE );
  Delay_ms( LCD_INIT_STEP_DELAY_MS );

  LCD_OutputUpperNibble( LCD_INIT_SET_4BIT_NIBBLE );
  LCD_SetE( ENABLE );
  LCD_SetE( DISABLE );
  Delay_ms( LCD_INIT_STEP_DELAY_MS );
#else
  LCD_Output( FUNC_SET | BUSWIDTH | NUMLINES );
  LCD_SetE( ENABLE );
  LCD_SetE( DISABLE );
  Delay_ms( LCD_INIT_WAKE_DELAY_MS );

  LCD_Output( FUNC_SET | BUSWIDTH | NUMLINES );
  LCD_SetE( ENABLE );
  LCD_SetE( DISABLE );
  Delay_ms( LCD_INIT_STEP_DELAY_MS );
#endif

#ifdef HD_ISVFD

  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES | vfd_intensity );
  Delay_ms( 15 );
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES | vfd_intensity );
  Delay_ms( 10 );
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES | vfd_intensity );
  Delay_ms( 10 );
   
#else  
  
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES );
  Delay_ms( 15 );
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES );
  Delay_ms( 5 );
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES );
  Delay_ms( 5 );

#endif
  
  LCD_Command(DISP_CTRL | DISP | CURSOR | BLINK);
  LCD_BusyWait();
  LCD_Command(CLR_DISP);
  LCD_BusyWait();
  LCD_Command(ENT_MODE | INC);
}


// Vacuum Flourescent Display intensity control.
//
// ..side note, I love these, they are so beautiful!
//
#ifdef HD_ISVFD

void LCD_VFD_Intensity( char intensity )
{
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES | intensity );
}

#endif
