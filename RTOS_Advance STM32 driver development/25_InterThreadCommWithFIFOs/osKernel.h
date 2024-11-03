#ifndef __OS_KERNEL_H__
#define __OS_KERNEL_H__

#include <stdint.h>

#include "stm32f4xx.h"                  // Device header

void osKernelLaunch(uint32_t quanta);
void osKernelInit(void);
uint8_t osKernelAddThreads(void(*task0)(void), void(*task1)(void),void(*task2)(void));
void osThreadYield(void);


uint8_t osKernelAddPeriod_Thread(void (*task)(void), uint32_t period);


void periodicTask1(void);

void periodicTask2(void);

void osSemaphoreInit(int32_t *semaphore,int32_t value);
void osSignalSet(int *semaphore);
void osSignalWait(int32_t *semaphore);
void osSignalCooperativeWait(int32_t *semaphore);

void osThreadSleep(uint32_t sleep_time);


void osMailboxInit(void);
void osMailboxSend(uint32_t data);
uint32_t osMailboxRecv(void);


void osFifoInit(void);
int8_t osFifoPut(uint32_t data);
uint32_t osFifoGet(void);

#endif