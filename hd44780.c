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

//#include "hw_interface_stm32.h"
#include "hw_interface_gd32.h"
#include "hd44780.h"

#ifdef __CROSSWORKS_ARM
	#include <stdio.h>
#endif

/** Different platforms require different delay solutions, so
  * so below is a macro to substitute your own,
  */
#ifdef _HW_INTERFACE_GD32_H
#define Delay_ms( ms_delay ) delay_millis( ms_delay )
#endif

#ifdef _HW_INTERFACE_STM32_H
#define Delay_ms( ms_delay ) HAL_Delay( ms_delay )
#endif


/** HD44780 system variables
  *
  */
int hd_xpos = 0 , hd_ypos = 0;
int dd_addr;
const char hd_map[] = HD_ADDR_MAP;
char no_wait = 1;

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
void delay_cycles( volatile char cycles_to_waste )
{
  while( cycles_to_waste-- );
}


void LCD_SetRS(char state)
{
  Output_Pin( LCD_RS, LCD_RS_BANK, state );
  delay_cycles( E_CYCLES );
}


void LCD_SetRNW(char state)
{
  Output_Pin( LCD_RNW, LCD_RNW_BANK, state );
  delay_cycles( E_CYCLES );
}


void LCD_SetE( char state )
{
  Output_Pin( LCD_E, LCD_E_BANK, state );
  delay_cycles( E_CYCLES );
}


void LCD_SetBusInput( void )
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


char LCD_Input( void )
{
  char ch;

  LCD_SetBusInput();

  ch  = Read_Pin( LCD_D7, LCD_D7_BANK ); ch <<= 1;
  ch |= Read_Pin( LCD_D6, LCD_D6_BANK ); ch <<= 1;
  ch |= Read_Pin( LCD_D5, LCD_D5_BANK ); ch <<= 1;
  ch |= Read_Pin( LCD_D4, LCD_D4_BANK );

#ifdef LCD_BUS8BIT
  ch <<= 1;
  ch |= LCD_D4( LCD_D3, LCD_D3_BANK ); ch <<= 1;
  ch |= LCD_D4( LCD_D2, LCD_D2_BANK ); ch <<= 1;
  ch |= LCD_D4( LCD_D1, LCD_D1_BANK ); ch <<= 1;
  ch |= LCD_D4( LCD_D0, LCD_D0_BANK );
#endif

  return ch;
}


// LCD Bus output function.  4 or 8-bit output capable, with no
// port dependencies to make PCB layout sweet.
//
void LCD_Output( char ch )
{

    Output_Pin( LCD_D7, LCD_D7_BANK, ch & 0x80 );
    Output_Pin( LCD_D6, LCD_D6_BANK, ch & 0x40 );
    Output_Pin( LCD_D5, LCD_D5_BANK, ch & 0x20 );
    Output_Pin( LCD_D4, LCD_D4_BANK, ch & 0x10 );
#ifdef LCD_BUS8BIT
    Output_Pin( LCD_D3, LCD_D3_BANK, ch & 0x8 );
    Output_Pin( LCD_D2, LCD_D2_BANK, ch & 0x4 );
    Output_Pin( LCD_D1, LCD_D1_BANK, ch & 0x2 );
    Output_Pin( LCD_D0, LCD_D0_BANK, ch & 0x1 );
#endif
}


// Checks for the LCD busy bit status and returns it.
//
char LCD_IsBusy( void )
{
  char busybit;

  LCD_SetBusInput();
  LCD_SetRS( INSTR_REG );
  LCD_SetRNW( READ );
  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );

  busybit = Read_Pin( LCD_D7, LCD_D7_BANK );

  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

#ifndef LCD_BUS8BIT
  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );
  
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );
#endif
  
  return busybit;
}


#ifdef LCD_READ_DD_SUPPORT
/** Read Display Data RAM
  *
  * Reads the display data referenced in the passed parameter
  * and returns it as a char.
  */
char LCD_Read_DDRAM( char dd_read_addr )
{
  char dd_data;

  LCD_Command( SET_DDRAM_ADD | dd_read_addr );
  LCD_BusyWait();
  LCD_SetRS( DATA_REG );
  LCD_SetRNW( READ );

  delay_cycles( E_CYCLES );
  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );
  dd_data = LCD_Input();
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

#ifdef LCD_BUS4BIT

  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );
  dd_data <<= 4;
  dd_data |= LCD_Input();
  LCD_SetE( DISABLE );
  
#endif

  return dd_data;
}
#endif


#ifdef LCD_READCHAR_SUPPORT
/** Read character at the display coordinates specified, with
  * the upper left cell being 0,0.
  */
char LCD_Readchar( char rc_x, char rc_y )
{
  int addr_to_sample;

  addr_to_sample = LCD_DDRAM_Addr( rc_x, rc_y );

  return ( LCD_Read_DDRAM( addr_to_sample ) );
}

#endif


// Writes a command to the LCD.
//
void LCD_Command( char cmd )
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


void LCD_BusyWait( void )
{
  while( LCD_IsBusy() );
}


// Special characters not present in the LCD can be defined using this function.
// 
// ChToSet is the user designed character code and *ChDataset is a pointer to the
// character data.
//
#ifdef LCD_UDG_SUPPORT

void LCD_Defchar( char ChToSet, uint8_t * ChDataset )
{
  int ChAddress,
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


// Function to allow arbitrary placement of text on the display.
//
void LCD_Locate(char x, char y)
{
  int addr;
  addr = LCD_DDRAM_Addr( x, y );
  hd_xpos = x; hd_ypos = y;
  
  LCD_Command( SET_DDRAM_ADD | addr );
}


// Function to write data to the LCD
//
void LCD_PutData( char dat )
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



// Scrolling support, to give a traditional primitive terminal experience to the LCD
//
#ifdef LCD_SCROLL_SUPPORT

void LCD_ScrollUp( void )
{
  char  line_pos,
        line,
        ch_moving,
        new_addr;

  // Do a sanity check.
  //
   if( YMAX == 0 ) return;

  // Make a copy one line up
  //  
  for( line = 1; line <= YMAX; line++)
    for( line_pos = 0; line_pos <= XMAX; line_pos++ )
    {
      ch_moving = LCD_Readchar( line_pos, line );
      new_addr = LCD_DDRAM_Addr( line_pos, line - 1 );
      LCD_Command( SET_DDRAM_ADD | new_addr );
      LCD_PutData( ch_moving );
    }
  
  // Clear the last line
  //  
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


char LCD_DDRAM_Addr( char dd_x, char dd_y )
{  
  return ( hd_map[ dd_x + ( XMAX + 1 ) *  dd_y ] );
}


// Putchar, by another name.
//
// This LCD driver is compatible with the CCS C compiler for PICs as well,
// and that does not support customizing of putchar, so we do it this way.
//
#define LCD_Putc( ch_to_put ) LCD_Putchar( ch_to_put );

char LCD_Putchar( char ch )
{
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
    {
      hd_ypos++;
#ifdef HD_NL_DOES_CR
      hd_xpos = 0;
#endif
      if( hd_ypos > YMAX )
      {
        LCD_ScrollUp();
        hd_ypos = YMAX;
      }
      LCD_Command( SET_DDRAM_ADD | LCD_DDRAM_Addr( hd_xpos, hd_ypos ) );
      break;
    }

    case '\r':
      {
        hd_xpos = 0;
        LCD_Command( SET_DDRAM_ADD | LCD_DDRAM_Addr( hd_xpos, hd_ypos ) );
        break;
      }

    default:
    {
      LCD_Command( SET_DDRAM_ADD | dd_addr );
      LCD_PutData( ch );
      hd_xpos++;
    }
  }

  if( hd_xpos > XMAX )
  {
    hd_xpos = 0;
    hd_ypos++;
    LCD_Command( SET_DDRAM_ADD | dd_addr );
  }
  return ch;
}


#ifdef __CROSSWORKS_ARM
int __putchar(int ch, __printf_tag_ptr ptr)
{
  return LCD_Putchar( ch );
}
#endif


#ifndef __CROSSWORKS_ARM
// A very primitive and lightweight printf implementation.
// ...good enough for now.
//
void LCD_Printf( char * string )
{
  while( ( *string ) != 0)
  {
    LCD_Putchar( *string++ );
  }
}
#endif


void LCD_Clear(void)
{
  LCD_BusyWait();
  LCD_Command(CLR_DISP);
  LCD_BusyWait();
  LCD_Locate( 0, 0 );
}


// Turns the cursor off or on.
//
void LCD_Cursor( char cursor_state )
{
  LCD_Command( 
              DISP_CTRL | DISP | ( cursor_state ? BLINK : 0 )
             );
}


// Initialises the LCD.
//
// The user must call this at the start or the display won't work.
//
void LCD_Init(void)
{
  LCD_Input();
  LCD_SetE(DISABLE);
  LCD_SetRS(INSTR_REG);
  LCD_SetRNW(READ);

  Delay_ms( 15 );

  LCD_SetRNW(WRITE);
  LCD_Output( FUNC_SET | BUSWIDTH | NUMLINES );
  LCD_SetE(ENABLE);
  LCD_SetE(DISABLE);
  LCD_Output( 0 );
  LCD_SetE(ENABLE);
  LCD_SetE(DISABLE);
  Delay_ms( 5 );

  LCD_SetRNW(WRITE);
  LCD_Output( FUNC_SET | BUSWIDTH | NUMLINES );
  LCD_SetE(ENABLE);
  LCD_SetE(DISABLE);
  LCD_Output( 0 );
  LCD_SetE(ENABLE);
  LCD_SetE(DISABLE);
  Delay_ms( 5 );

  LCD_SetRNW(WRITE);
  LCD_Output( FUNC_SET | BUSWIDTH | NUMLINES );
  LCD_SetE(ENABLE);
  LCD_SetE(DISABLE);
  LCD_Output( 0 );
  LCD_SetE(ENABLE);
  LCD_SetE(DISABLE);
  Delay_ms( 5 );

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
