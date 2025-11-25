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


#ifndef _HD44780_H
#define _HD44780_H
#include <gd32f10x.h>
#include <stdio.h>
//#include "hw_interface_stm32.h"
#include "hw_interface_gd32.h"

/** ----------------------------------------------------------+ 
  * User configuration section.                               !
  *                                                           !
  *                                                           !
  *  LCD_D7..LCD_D4 must be defined for 4 bit mode            !
  *  LCD_D3..LCD_D0 must also be defined for 8 bit mode.      !
  *  LCD_BUSxBIT must be uncommented as required.             !
  *  LCD_RS, LCD_RNW and LCD_E must also be defined.          !
  *  LCD_xx_BANK defines must also be defined.                !
  *  This is a work in progress...                            !
  *                                                           !
  *  If you don't require scrolling and a few other features  !
  *  you might consider using the Lite mode.                  !
  *                                                           !
  *  Written by Jennifer Gunn for everyone to use.            !
  *                                                           !
  *  jennifer.a.gunn@outlook.com                              !
  *                                                           !
  *-----------------------------------------------------------+
  */
#define LCD_BUS4BIT
//#define LCD_BUS8BIT


#ifndef LCD_BUS4BIT
  #ifndef LCD_BUS8BIT
    #error LCD Bus width must be defined.
  #endif
#endif


// These pins are always used.
//
#define LCD_D7        GPIO_PIN_9
#define LCD_D6        GPIO_PIN_8
#define LCD_D5        GPIO_PIN_7
#define LCD_D4        GPIO_PIN_6

#define LCD_D7_BANK    GPIOB
#define LCD_D6_BANK    GPIOB
#define LCD_D5_BANK    GPIOB
#define LCD_D4_BANK    GPIOB

// These pins are only used for 8-bit mode.
//
#ifdef LCD_BUS8BIT

  #define LCD_D3        GPIO_Pin_xx
  #define LCD_D2        GPIO_Pin_xx
  #define LCD_D1        GPIO_Pin_xx
  #define LCD_D0        GPIO_Pin_xx

  #define LCD_D7_BANK    GPIOA
  #define LCD_D6_BANK    GPIOA
  #define LCD_D5_BANK    GPIOA
  #define LCD_D4_BANK    GPIOA
 
#endif


/** LCD Bus control pins
  *
  * These also must be defined
  */
#define LCD_RS        GPIO_PIN_5  
#define LCD_RS_BANK   GPIOB

#define LCD_RNW       GPIO_PIN_0
#define LCD_RNW_BANK  GPIOB

#define LCD_E         GPIO_PIN_3
#define LCD_E_BANK    GPIOB

/** LCD Timing
  * 
  * This sets the number of cycles hold time for the bus.
 */
#define E_CYCLES      8 // How many cycles to delay for bus settling and controller response.

/** Panel mapping selection defines.
  *
  * Un-comment whichever suits your panel.
  */  
// #define HD_PANEL_8X1
// #define HD_PANEL_16X1_T1     // This is for Type one LCDs
// #define HD_PANEL_16X1_T2     // ..and this if for type two. Why 2?!
// #define HD_PANEL_16X2
// #define HD_PANEL_16X4
// #define HD_PANEL_20X2
#define HD_PANEL_20X4
// #define HD_PANEL_40X2


/** Vacuum Flourescent Display support.
  *
  * Some VFDs support intensity control which
  * this driver supports when this define is 
  * uncommented.  Not required unless you
  * are using a VFD.
  */
// #define HD_ISVFD


/** Set this define to compile in Lite mode.
  *
  * Lite mode reduces the driver features therefore reducing
  * memory usage.
  */
// #define LCD_LITE


/** Set this define to make \n also do a \r
  *
  */
  #define HD_NL_DOES_CR

/** =================================
  * End of user configurable section
  * =================================
  */


/** Adjust compilation support depending on
  * whether Lite has been selected
  */
#ifndef LCD_LITE
  #define LCD_UDG_SUPPORT
  #define LCD_READCHAR_SUPPORT
  #define LCD_SCROLL_SUPPORT
  #define LCD_READ_DD_SUPPORT
#endif


/** LCD Bus Bit defines
  */
#define BF          0b10000000
#define WRITE       0
#define READ        1
#define DATA_REG    1
#define INSTR_REG   0
#define ENABLE      1
#define DISABLE     0


/** LCD Commands
  */
#define CLR_DISP      0b00000001
#define RET_HOME      0b00000010
#define ENT_MODE      0b00000100
#define DISP_CTRL     0b00001000
#define CURS_DISP_SH  0b00010000
#define FUNC_SET      0b00100000
#define SET_CGRAM_ADD 0b01000000
#define SET_DDRAM_ADD 0b10000000


/** LCD Command Parameters
  */
#define INC           0b00000010
#define DEC           0b00000000
#define SHIFT         0b00000001
#define DISP          0b00000100
#define CURSOR        0b00000010
#define NO_CURSOR     0b00000000
#define BLINK         0b00000001
#define DIS_SHIFT     0b00001000
#define RIGHT         0b00000100
#define DL_EIGHT      0b00010000
#define TWOLINES      0b00001000
#define ONELINE       0b00000000
#define FIVETEN       0b00000100
//
// These Last values are only for VFD displays.
//
#define VFD100        0b00000000
#define VFD75         0b00000001
#define VFD50         0b00000010
#define VFD25         0b00000011


/** Bus width signalling to registers
  */
#ifdef LCD_BUS8BIT
#define BUSWIDTH DL_EIGHT
#endif

#ifdef LCD_BUS4BIT
#define BUSWIDTH 0
#endif


/** Panel Mapping. Refer to your LCDs Datasheet!
  *
  * Different panels have their own special
  * characteristics and awkward dd memory
  * layouts.  These following defines set
  * out how to drive each panel type.
  */

/** Mapping tables for different display specs
  *
  */
#ifdef HD_PANEL_8X1     //  8 by 1

#define YMAX      0
#define XMAX      7
#define HD_ADDR_MAP { 0, 1, 2, 3, 4, 5, 6, 7 }
#define NUMLINES  ONELINE

#endif


#ifdef HD_PANEL_16X1_T1     //  16 by 1 type 1

#define YMAX      0
#define XMAX      15
#define HD_ADDR_MAP { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,\
                      0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47 }
// Notice the f**ked up system HD44780 Controllers use!
//
#define NUMLINES  TWOLINES

#endif


#ifdef HD_PANEL_16X1_T2     //  16 by 1 type 2

#define YMAX      0
#define XMAX      15
#define HD_ADDR_MAP { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,\
                      0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF }
#define NUMLINES  TWOLINES

#endif


#ifdef HD_PANEL_16X2     //  16 by 2

#define YMAX      1
#define XMAX      15
#define HD_ADDR_MAP { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,\
                      0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF,\
                      0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,\
                      0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F }
#define NUMLINES  TWOLINES

#endif


#ifdef HD_PANEL_16X4     //  16 by 4

#define YMAX      3
#define XMAX      15
#define HD_ADDR_MAP { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,\
                      0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF,\
                      0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,\
                      0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,\
                      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,\
                      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,\
                      0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,\
                      0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F }
#define NUMLINES  TWOLINES

#endif


#ifdef HD_PANEL_20X2     //  20 by 2

#define YMAX      1
#define XMAX      19
#define HD_ADDR_MAP { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9,\
                      0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13,\
                      0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,\
                      0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53 }
#define NUMLINES  TWOLINES

#endif


#ifdef HD_PANEL_20X4     //  20 by 4

#define YMAX      3
#define XMAX      19
#define HD_ADDR_MAP { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9,\
                      0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13,\
                      0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,\
                      0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53,\
                      0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,\
                      0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,\
                      0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D,\
                      0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67 }
#define NUMLINES  TWOLINES

#endif

#ifdef HD_PANEL_40X2     //  20 by 4

#define YMAX      1
#define XMAX      39
#define HD_ADDR_MAP { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9,\
                      0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13,\
                      0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,\
                      0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,\
                      0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,\
                      0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53,\
                      0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D,\
                      0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67 }
#define NUMLINES  TWOLINES

#endif


/** Function Declarations
  *
  * Porting this driver to other platforms will require
  * that the hardware functions be modified.
  * It may also require that you adjust types to suit
  * your compiler as this was written in CCS C.
  */

  
// Hardware Abstraction Layer Functions
//
void LCD_SetRS        ( char state );
void LCD_SetRNW       ( char state );
void LCD_SetE         ( char state );
void LCD_SetBusInput  ( void );
char LCD_Input        ( void );
void LCD_Output       ( char ch );
char LCD_IsBusy       ( void );

// System Control
//
void LCD_Command      ( char cmd );
void LCD_BusyWait     ( void );
void LCD_Init         ( void );
void LCD_Locate       ( char x, char y );
void LCD_Cursor       ( char cursor_state );

#ifdef __CROSSWORKS_ARM
int __putchar(int ch, __printf_tag_ptr ptr);
#endif

// HACK: Further iffy bit, also covering CCS C's inbuilt functions
//       but not accurate.  Its just to get things working.
//
void delay_cycles( char cycles_to_waste );

// IO Functions.
//
char LCD_Read_DDRAM   ( char dd_read_addr );
void LCD_PutData      ( char dat );
char LCD_DDRAM_Addr   ( char dd_x, char dd_y );
char LCD_Putchar      ( char ch );

#ifdef LCD_READCHAR_SUPPORT
char LCD_Readchar     ( char rc_x, char rc_y );
#endif

#ifndef __CROSSWORKS_ARM
void LCD_Printf       ( char * string );
#endif
void LCD_Clear        ( void );

#ifdef LCD_UDG_SUPPORT
void LCD_Defchar      ( char ChToSet, uint8_t * ChDataset );
#endif

#ifdef LCD_SCROLL_SUPPORT
  void LCD_ScrollUp   ( void );
#endif

// VFD Specific function for display intensity.
//
#ifdef HD_ISVFD
  void LCD_VFD_Intensity( char intensity );
#endif

#endif // _HD44780_H
