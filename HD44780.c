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

#case
#include "hardware.h"

/** HD44780 system variables
  *
  */
int hd_xpos = 0 , hd_ypos = 0;
int dd_addr;
const char hd_map[] = HD_ADDR_MAP;


// The below functions are designed for CCS C, but will need customizing
// to suit whatever hardware you are using, eg STM32, AVR etc.
//
void LCD_SetRS(char state)
{
  output_bit(LCD_RS, state);
}


void LCD_SetRNW(char state)
{
  output_bit(LCD_RNW, state);
}


void LCD_SetE( char state )
{
  output_bit( LCD_E, state );
}


char LCD_Input( void )
{
  char ch;
  
  ch = input( LCD_D7 ); ch <<= 1;
  ch |= input( LCD_D6 ); ch <<= 1;
  ch |= input( LCD_D5 ); ch <<= 1;
  ch |= input( LCD_D4 );
#ifdef LCD_BUS8BIT
  ch <<= 1;
  ch |= input( LCD_D3 ); ch <<= 1;
  ch |= input( LCD_D2 ); ch <<= 1;
  ch |= input( LCD_D1 ); ch <<= 1;
  ch |= input( LCD_D0 );
#endif

  return ch;
}


void LCD_Output( char ch )
{
  output_bit( LCD_D7, ch & 0x80 );
  output_bit( LCD_D6, ch & 0x40 );
  output_bit( LCD_D5, ch & 0x20 );
  output_bit( LCD_D4, ch & 0x10 );
#ifdef LCD_BUS8BIT
  output_bit( LCD_D3, ch & 0x8 );
  output_bit( LCD_D2, ch & 0x4 );
  output_bit( LCD_D1, ch & 0x2 );
  output_bit( LCD_D0, ch & 0x1 );
#endif
}


// The below functions will not need changing, except for the delay_cycles
// function which is a feature of CCS C.
//
char LCD_IsBusy( void )
{
  char busybit;

  LCD_SetRS( INST_REG );
  LCD_SetRNW( READ );
  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );

  busybit = input( LCD_D7 );

  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

  LCD_SetE( ENABLE );
  delay_cycles( E_CYCLES );
  
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

  if( ( busybit ) != 0 )
  {
    LCD_SetE( DISABLE );
    delay_cycles( E_CYCLES );

    return 1;
  }
  else
  {
    LCD_SetE( DISABLE );
    delay_cycles( E_CYCLES );
    return 0;
  }
}


#ifdef LCD_READ_DD_SUPPORT

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
char LCD_Readchar( char rc_x, char rc_y )
{
  int addr_to_sample;

  addr_to_sample = LCD_DDRAM_Addr( rc_x, rc_y );

  return ( LCD_Read_DDRAM( addr_to_sample ) );
}
#endif


void LCD_Command( char cmd )
{
  LCD_BusyWait();
  LCD_SetRS( INST_REG );
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
}


void LCD_BusyWait( void )
{
  while( LCD_IsBusy() );
}


#ifdef LCD_UDG_SUPPORT
void LCD_Defchar( char ChToSet, char * ChDataset )
{
  int ChAddress,
      ch_line,
      dd_addr;
   
  ChAddress = ChToSet << 3;
  
  LCD_SetRS( INST_REG );
  LCD_SetRNW( READ );
  LCD_SetE( ENABLE );
  dd_addr = LCD_Input() & 0b1111111;
  LCD_SetE( DISABLE );
  delay_cycles( E_CYCLES );

#ifdef LCD_BUS4BIT
  dd_addr <<= 4;
  LCD_SetE( ENABLE );
  dd_addr |= LCD_Input() & 0b1111111;
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
  
  LCD_Command( SET_DDRAM_ADD | dd_addr );
}
#endif


void LCD_Locate(char x, char y)
{
  int addr;
  addr = LCD_DDRAM_Addr( x, y );
  hd_xpos = x; hd_ypos = y;
  
  LCD_Command( SET_DDRAM_ADD | addr );
}


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


#define LCD_Putc( ch_to_put ) LCD_Putchar( ch_to_put );


char LCD_Putchar( char ch )
{
  if( hd_ypos > YMAX )
#ifdef LCD_SCROLL_SUPPORT
    {
      hd_ypos = YMAX;
      LCD_ScrollUp();
    }
#else   // The alternative is to wrap around.
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


void LCD_Printf( char * string )
{
  while( ( *string ) != 0)
  {
    LCD_Putchar( *string++ );
  }
}


void LCD_Clear(void)
{
  LCD_BusyWait();
  LCD_Command(CLR_DISP);
  LCD_BusyWait();
  LCD_Locate( 0, 0 );
}

void LCD_Cursor( char cur_state )
{
  LCD_Command( 
              DISP_CTRL | DISP | cur_state
             );
}


void LCD_Init(void)
{
  LCD_SetE(DISABLE);
  LCD_SetRS(INST_REG);
  LCD_SetRNW(READ);
  LCD_Input();
  
#ifdef HD_ISVFD
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES | vfd_intensity );
  delay_ms(10);
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES | vfd_intensity );
  delay_ms(110);
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES | vfd_intensity );
  delay_ms(110);
#else  
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES );
  delay_ms(10);
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES );
  delay_ms(110);
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES );
  delay_ms(110);
#endif
  
  LCD_Command(DISP_CTRL | DISP | CURSOR | BLINK);
  LCD_BusyWait();
  LCD_Command(CLR_DISP);
  LCD_BusyWait();
  LCD_Command(ENT_MODE | INC);
}


#ifdef HD_ISVFD
void LCD_VFD_Intensity( char intensity )
{
  LCD_Command( FUNC_SET | BUSWIDTH | NUMLINES | intensity );
}
#endif
