#ifndef _HW_INTERFACE_H
	#define _HW_INTERFACE_H
#endif

#include <stdint.h>
#include "main.h"     // Used with CubeMX projects.

#define PIN_SPEED_DEFAULT GPIO_SPEED_FREQ_LOW

/** Exported Function Declarations
  *
  */
void Output_Pin         ( uint16_t pin_to_wr, GPIO_TypeDef* port, char bit );
void Output_Pin_NoDDR   ( uint16_t pin_to_wr, GPIO_TypeDef* port, char bit );
void Set_Input_Pin      ( int pin_to_hiz, GPIO_TypeDef* port );
uint8_t Read_Pin        ( uint16_t GPIO_Pin, GPIO_TypeDef *GPIOx );
