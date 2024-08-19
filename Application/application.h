/*
 * application.h
 *
 *  Created on: Aug 14, 2024
 *      Author: Dmitry
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#define BitVal(data,y) ( (data>>y) & 1)      /** Return Data.Y value   **/
#define SetBit(data,y)    data |= (1 << y)    /** Set Data.Y   to 1    **/
#define ClearBit(data,y)  data &= ~(1 << y)   /** Clear Data.Y to 0    **/
#define TogleBit(data,y)     (data ^=BitVal(y))     /** Togle Data.Y  value  **/
#define Togle(data)   (data =~data )         /** Togle Data value     **/


#define LEDON	LED_GPIO_Port->BSRR = (uint32_t) LED_Pin << 16U
#define LEDOFF	LED_GPIO_Port->BSRR = LED_Pin
#define OUT_LOW	OUT_GPIO_Port->BSRR = (uint32_t) OUT_Pin << 16U
#define OUT_HI	OUT_GPIO_Port->BSRR = OUT_Pin

#define KEY_UP		(KEY_GPIO_Port->IDR & KEY_Pin) != 0
#define KEY_DOWN	(KEY_GPIO_Port->IDR & KEY_Pin) == 0

void app_init();

void app_loop();

void app_tim1_update();

#endif /* APPLICATION_H_ */
