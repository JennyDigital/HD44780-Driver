#ifndef _HW_INTERFACE_GD32_H
	#define _HW_INTERFACE_GD32_H

#include <stdint.h>
#include "gd32f10x.h"

#define PIN_SPEED_DEFAULT GPIO_OSPEED_10MHZ

/** Exported Function Declarations
  *
  */
void delay_millis       ( uint32_t wait_time );
void Output_Pin         ( uint32_t pin_to_wr, uint32_t port, char bit );
void Output_Pin_NoDDR   ( uint32_t pin_to_wr, uint32_t port, char bit );
void Set_Input_Pin      ( uint32_t pin_to_hiz, uint32_t port );
uint8_t Read_Pin        ( uint32_t GPIO_Pin, uint32_t port );

#endif // _HW_INTERFACE_GD32_H