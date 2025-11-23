#include "hw_interface_stm32.h"
//#include "hw_interface_gd32.h"

/** Sets up a pin at a specifc output state.
  *
  * @brief. A simple pin output function to make low demand projects quicker to get
  *         up and running.  Not as full control as some solutions but a bit
  *         Arduino-like.
  *
  * @param pin_to_wr, &port, bit value
  * @retval none
  */
void Output_Pin( uint16_t pin_to_wr, GPIO_TypeDef* port, char bit )
{
    /* Check the parameters */
  assert_param( IS_GPIO_PIN( pin_to_wr ) );
  assert_param( IS_GPIO_PIN_ACTION( bit ) );
   
  // Setup struct
  //
  GPIO_InitTypeDef PinStruct = {0};

  PinStruct.Pin    = pin_to_wr;
  PinStruct.Speed  = PIN_SPEED_DEFAULT;
  PinStruct.Mode   = GPIO_MODE_OUTPUT_PP;

  HAL_GPIO_Init( port, &PinStruct );

  HAL_GPIO_WritePin( port, pin_to_wr , bit );
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
void Output_Pin_NoDDR( uint16_t pin_to_wr, GPIO_TypeDef* port, char bit )
{
    /* Check the parameters */
  assert_param( IS_GPIO_PIN( pin_to_wr ) );
  assert_param( IS_GPIO_PIN_ACTION( bit ) );

  HAL_GPIO_WritePin( port, pin_to_wr , bit );
}


/** Initialise a pin as input and read it's status
  * ...a slow but sure comfort feature.
  *
  * @params:  pin_to_hiz input pin.
  *           port the pin is on.
  * @reval:   none
  */
void Set_Input_Pin
  ( int pin_to_hiz, GPIO_TypeDef* port )
{
  GPIO_InitTypeDef PinStruct;
  PinStruct.Pin     = pin_to_hiz;
  PinStruct.Speed   = GPIO_SPEED_FREQ_LOW;
  PinStruct.Mode    = GPIO_MODE_INPUT;
  PinStruct.Pull    = GPIO_PULLUP;

  HAL_GPIO_Init( port, &PinStruct );
}


/** Read a pins state and return the result.
  *
  * @param pin
  * @param port
  * @retval pinstate
  */
uint8_t Read_Pin( uint16_t GPIO_Pin, GPIO_TypeDef *GPIOx )
{
  /* Check the parameters */
  assert_param( IS_GPIO_PIN( GPIO_Pin ) );

  #ifdef READ_SETS_INPUT
    Set_Input_Pin( GPIO_Pin, GPIOx );
  #endif

  return HAL_GPIO_ReadPin( GPIOx, GPIO_Pin );
}
