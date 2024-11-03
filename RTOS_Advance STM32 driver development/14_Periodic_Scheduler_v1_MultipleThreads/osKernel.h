#ifndef __OS_KERNEL_H__
#define __OS_KERNEL_H__

#include <stdint.h>

#include "stm32f4xx.h"                  // Device header

void osKernelLaunch(uint32_t quanta);
void osKernelInit(void);
uint8_t osKernelAddThreads(void(*task0)(void), void(*task1)(void),void(*task2)(void));
void osThreadYield(void);


void periodicTask1(void);

void periodicTask2(void);

#endif