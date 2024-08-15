/*
 * application.h
 *
 *  Created on: Aug 14, 2024
 *      Author: Dmitry
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_


#define LEDON	LED_GPIO_Port->BSRR = (uint32_t) LED_Pin << 16U
#define LEDOFF	LED_GPIO_Port->BSRR = LED_Pin
#define KEY_UP		(KEY_GPIO_Port->IDR & KEY_Pin) != 0
#define KEY_DOWN	(KEY_GPIO_Port->IDR & KEY_Pin) == 0

void app_init();

void app_loop();

#endif /* APPLICATION_H_ */
