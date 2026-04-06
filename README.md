# HD44780-Driver
A driver for the HD44780 based displays.

It is a simple easily portable affair with support for vacuum fluorescent displays, and different arrangements too.

## API overview

Include `hd44780.h`, configure the panel and feature macros to suit the target, then call `LCD_Init()` before using any other display function.

## Basic example

```c
#include "hd44780.h"

void App_LCD_Startup( void )
{
	LCD_Init();
	LCD_Clear();
	LCD_Locate( 0, 0 );
	LCD_Puts( "Hello" );
	LCD_Locate( 0, 1 );
	LCD_Printf( "Count: %u", 42u );
}
```

If `LCD_PRINTF_SUPPORT` is disabled, replace `LCD_Printf()` with `LCD_Puts()` or `LCD_Putchar()`.

## Read and recovery example

```c
#include "hd44780.h"

void App_LCD_Check_First_Char( void )
{
	uint8_t ch;

	if( LCD_Readchar( 0, 0, &ch ) == LCD_RESULT_OK )
	{
		LCD_Locate( 0, 1 );
		LCD_Printf( "First: %c", ch );
		return;
	}

	if( LCD_GetStatus() & LCD_STATUS_TIMEOUT )
	{
		Set_Fault_LED( 1 );
		LCD_ClearStatus();
		LCD_Init();
		return;
	}

	Set_Fault_LED( 1 );
}
```

### Core control

- `void LCD_Init( void )`
	Initializes the display and controller state. This must be called before any other LCD API.
- `void LCD_Clear( void )`
	Clears the display and returns the cursor to the home position.
- `void LCD_Locate( uint8_t x, uint8_t y )`
	Moves the cursor to the specified display coordinates.
- `void LCD_Cursor( uint8_t cursor_state )`
	Sets the cursor mode using `LCD_CURSOR_OFF`, `LCD_CURSOR_UNDERLINE`, `LCD_CURSOR_BLINK`, or `LCD_CURSOR_UNDERLINE_BLINK`.
- `uint8_t LCD_GetStatus( void )`
	Returns the current driver status bitmask.
- `void LCD_ClearStatus( void )`
	Clears the current driver status so operations can be retried after an error.

### Character and string output

- `void LCD_PutData( uint8_t dat )`
	Writes a raw data byte to the display at the current cursor position.
- `uint8_t LCD_Putchar( uint8_t ch )`
	Writes one character using the driver's cursor, newline, wrap, and scroll handling.
- `void LCD_Puts( const char * string )`
	Writes a null-terminated string directly to the display.
- `int LCD_Printf( const char * format, ... )`
	Formats text with `printf`-style arguments and writes the result to the display.
	Available only when `LCD_PRINTF_SUPPORT` is defined.

#### Write behaviour

- Writing a printable character into the last visible column does not immediately wrap or scroll.
- After that last-column write, the next printable character wraps to the next line.
- If wrapping from the last line and `LCD_SCROLL_SUPPORT` is enabled, the display scrolls up before the next printable character is written.
- If wrapping from the last line and `LCD_SCROLL_SUPPORT` is disabled, output wraps back to the top line instead.
- `\r` moves to column `0` of the current row and clears any pending wrap from a previous last-column write.
- `\n` advances to the next row. It does not force column `0` unless `HD_NL_DOES_CR` is defined.
- A `\n` issued on the last row scrolls when `LCD_SCROLL_SUPPORT` is enabled, otherwise it wraps to the top of the display.
- This means the bottom-right character is preserved on screen, and any wrap or scroll happens on the following output action rather than immediately.

### Addressing and display reads

- `uint8_t LCD_DDRAM_Addr( uint8_t dd_x, uint8_t dd_y )`
	Returns the controller DDRAM address for a display coordinate.
- `uint8_t LCD_Read_DDRAM( uint8_t dd_read_addr, uint8_t * dd_data )`
	Reads a raw DDRAM byte from a controller address.
	Returns `LCD_RESULT_OK` on success and writes the value to `dd_data`.
	Available when `LCD_READ_DD_SUPPORT` is enabled.
- `uint8_t LCD_Readchar( uint8_t rc_x, uint8_t rc_y, uint8_t * rc_data )`
	Reads the character currently shown at a display coordinate.
	Returns `LCD_RESULT_OK` on success and writes the value to `rc_data`.
	Available when `LCD_READCHAR_SUPPORT` is enabled.

### Custom characters and scrolling

- `void LCD_Defchar( uint16_t ChToSet, const uint8_t * ChDataset )`
	Defines one user character in CGRAM.
	In `LCD_CHAR_FONT_5X8` mode, `ChDataset` supplies 8 rows and `ChToSet` may be `0` to `7`.
	In `LCD_CHAR_FONT_5X10` mode, `ChDataset` supplies 11 rows and `ChToSet` may be `0` to `3`.
	Available when `LCD_UDG_SUPPORT` is enabled.
- `void LCD_ScrollUp( void )`
	Scrolls the display contents up by one line.
	Available when `LCD_SCROLL_SUPPORT` is enabled.

#### Custom character example

```c
#include "hd44780.h"

static const uint8_t heart_5x8[ 8 ] =
{
	0x00, 0x0A, 0x1F, 0x1F, 0x1F, 0x0E, 0x04, 0x00
};

static const uint8_t tall_bar_5x10[ 11 ] =
{
	0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00
};

void App_LCD_Load_Custom_Chars( void )
{
	LCD_Defchar( 0, heart_5x8 );

	/* In LCD_CHAR_FONT_5X10 mode, use 11 rows and slots 0..3. */
	LCD_Defchar( 1, tall_bar_5x10 );
}
```

Use the dataset that matches the selected `LCD_CHAR_FONT` mode.

### VFD-specific support

- `void LCD_VFD_Intensity( char intensity )`
	Sets VFD brightness using the provided intensity constant.
	Available only when `HD_ISVFD` is defined.

### Support and integration helpers

- `void delay_cycles( uint8_t cycles_to_waste )`
	Busy-wait helper used by the driver timing layer. This remains visible for toolchain portability.
- `int __putchar( int ch, __printf_tag_ptr ptr )`
	CrossWorks retarget hook when building with Rowley CrossWorks.
- `int __io_putchar( int ch )`
	GCC retarget hook when building with GNU toolchains.

#### Timeout behaviour

- LCD operations poll the controller busy flag before continuing.
- If the busy flag does not clear within `LCD_BUSY_WAIT_TIMEOUT` polls, the driver sets `LCD_STATUS_TIMEOUT`.
- Once timed out, later LCD operations fail closed rather than blocking forever.
- Call `LCD_GetStatus()` to inspect the current status and `LCD_ClearStatus()` to allow retries.
- Set `LCD_BUSY_WAIT_TIMEOUT` to `0` if you explicitly want the old wait-forever behaviour.

## Configuration notes

- Select the panel geometry in `hd44780.h` with one `HD_PANEL_*` define.
- The built-in geometry options now include `HD_PANEL_24X1_T1`, `HD_PANEL_24X1_T2`, and `HD_PANEL_24X2` in addition to the existing 8, 16, 20, and 40 column layouts.
- For 24x1 modules, choose the `T1` or `T2` mapping that matches the panel datasheet, just as with the existing 16x1 support.
- Select either `LCD_BUS4BIT` or `LCD_BUS8BIT`.
- Select `LCD_CHAR_FONT_5X8` or `LCD_CHAR_FONT_5X10` with `LCD_CHAR_FONT`.
- Enable `LCD_SCROLL_SUPPORT` if automatic scrolling is wanted instead of wraparound.
- Enable `LCD_PRINTF_SUPPORT` to include `LCD_Printf`.
- `LCD_PRINTF_BUFFER_SIZE` controls the temporary format buffer used by `LCD_Printf`.
- `LCD_BUSY_WAIT_TIMEOUT` controls how many busy-flag polls are allowed before a timeout is reported.
- `LCD_CHAR_FONT_5X10` is intended for supported one-line modules; most common multi-line modules use 5x8 characters.
- `LCD_LITE` disables several optional features to reduce memory usage.
- `LCD_UDG_SUPPORT`, `LCD_READCHAR_SUPPORT`, and `LCD_READ_DD_SUPPORT` are enabled automatically when `LCD_LITE` is not defined.

## Now supporting putchar retargetting for Rowley CrossWorks and GCC compilers!

I hope you all like it.

Kind regards,


JennyDigital.
