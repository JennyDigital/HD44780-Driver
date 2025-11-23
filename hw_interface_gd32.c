#include "hw_interface_gd32.h"
//#include "gd32f10x.h"

volatile uint32_t TickCounter = 0;

void SysTick_Handler( void )
{
  if( TickCounter )
  {
    TickCounter--;
  }
}

void delay_millis( uint32_t wait_time )
{
  TickCounter = wait_time;
  while( TickCounter );
}


/** Sets up a pin at a specifc output state.
  *
  * @brief. A simple pin output function to make low demand projects quicker to get
  *         up and running.  Not as full control as some solutions but a bit
  *         Arduino-like.
  *
  * @param pin_to_wr, &port, bit value
  * @retval none
  */
void Output_Pin( uint32_t pin_to_wr, uint32_t port, char bit )
{
  gpio_init( port, GPIO_MODE_OUT_PP, PIN_SPEED_DEFAULT, pin_to_wr );
  gpio_bit_write( port, pin_to_wr, bit );
}


/** Outputs assuming state is already set-up.
  *
  * @brief. A simple pin output function to make low demand projects quicker to get
  *         up and running.  Not as full control as some solutions but a bit
  *         Arduino-like.
  *
  * @param pin_to_wr, &port, bit value
  * @retval none
  */
void Output_Pin_NoDDR( uint32_t pin_to_wr, uint32_t port, char bit )
{
  gpio_bit_write( port, pin_to_wr, bit );
}


/** Initialise a pin as input and read it's status
  * ...a slow but sure comfort feature.
  *
  * @params:  pin_to_hiz input pin.
  *           port the pin is on.
  * @reval:   none
  */
void Set_Input_Pin
  ( uint32_t pin_to_hiz, uint32_t port )
{
    gpio_init( port, GPIO_MODE_IN_FLOATING, PIN_SPEED_DEFAULT, pin_to_hiz );
}


/** Read a pins state and return the result.
  *
  * @param pin
  * @param port
  * @retval pinstate
  */
uint8_t Read_Pin( uint32_t GPIO_Pin, uint32_t port )
{
  #ifdef READ_SETS_INPUT
    Set_Input_Pin( GPIO_Pin, GPIOx );
  #endif

  return gpio_input_bit_get( port, GPIO_Pin );
}
