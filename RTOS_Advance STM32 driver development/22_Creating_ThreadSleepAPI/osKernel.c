/*
   -schedular
	 -Thread Control Block
	 -Stack
*/

#include "osKernel.h"

#define NUM_OF_THREADS 3
#define STACKSIZE 100
#define NUM_PERIODIC_TASKS 5

typedef void(*taskT)(void);
#define NULL (void*)0
	

#define BUS_FREQ		16000000

uint32_t MILLIS_PRESCALER;
void osSchedularLaunch(void);

/*Systick priority register*/
#define SYSPRI3         (*((volatile uint32_t *)0xE000ED20))
#define INTCTRL         (*((volatile uint32_t *)0xE000ED04))


void osPeriodicTask_Init(void(*task)(void), uint32_t freq, uint8_t priority);


typedef struct{
  taskT task;
	uint32_t period;
	uint32_t  timeLeft;

}periodicTaskT;

static periodicTaskT	PeriodicTasks[NUM_PERIODIC_TASKS];
static uint32_t TimeMsec;
static uint32_t MaxPeriod;
static int32_t NumPeriodicThreads =0;


struct tcb{
	
  int32_t *stackPt;
	struct tcb *nextPt;
	uint32_t sleepTime;
	
};

typedef struct tcb tcbType;
tcbType tcbs[NUM_OF_THREADS];
tcbType *currentPt;

int32_t TCB_STACK[NUM_OF_THREADS][STACKSIZE];

void osKernelStackInit(int i){
   tcbs[i].stackPt =  &TCB_STACK[i][STACKSIZE -16];//StackPointer
	 TCB_STACK[i][STACKSIZE -1] =  0x01000000;
	
	TCB_STACK[i][STACKSIZE-3] = 0x14141414;  //R14
	TCB_STACK[i][STACKSIZE-4] = 0x12121212;  //R12
	TCB_STACK[i][STACKSIZE-5] = 0x03030303;  //R3
	TCB_STACK[i][STACKSIZE-6] = 0x02020202;  //R2
	TCB_STACK[i][STACKSIZE-7] = 0x01010101;  //R1
	TCB_STACK[i][STACKSIZE-8] = 0x00000000;  //R0
	  /*We have to save manually*/	
	TCB_STACK[i][STACKSIZE-9] = 0x11111111;  //R11
	TCB_STACK[i][STACKSIZE-10] = 0x10101010; //R10
	TCB_STACK[i][STACKSIZE-11] = 0x09090909; //R9
	TCB_STACK[i][STACKSIZE-12] = 0x08080808; //R8
	TCB_STACK[i][STACKSIZE-13] = 0x07070707; //R7
	TCB_STACK[i][STACKSIZE-14] = 0x06060606; //R6
	TCB_STACK[i][STACKSIZE-15] = 0x05050505; //R5
	TCB_STACK[i][STACKSIZE-16] = 0x04040404; //R4
}


uint8_t osKernelAddPeriod_Thread(void (*task)(void), uint32_t period)
{
  if(NumPeriodicThreads ==  NUM_PERIODIC_TASKS || period == 0){
	  return 0;
	}
	
	PeriodicTasks[NumPeriodicThreads].task = task;
	PeriodicTasks[NumPeriodicThreads].period =  period;
	PeriodicTasks[NumPeriodicThreads].timeLeft =  period -1;
	
	NumPeriodicThreads++;
	return 1;
	
}

void periodic_events_execute(void){
 
	int i;
	 for(i=0;i<NumPeriodicThreads;i++){
	   if(PeriodicTasks[i].timeLeft ==0){
			 PeriodicTasks[i].task();
			 PeriodicTasks[i].timeLeft = PeriodicTasks[i].period -1;
		 }
		 else{
		   PeriodicTasks[i].timeLeft--;
		 }
	 }
	 for(int i =0;i<NUM_OF_THREADS;i++){
	   if(tcbs[i].sleepTime >0){
		  tcbs[i].sleepTime--;
		 }
	 }
	 
}

uint8_t osKernelAddThreads(void(*task0)(void), void(*task1)(void),void(*task2)(void))
{
	__disable_irq();
	
	tcbs[0].nextPt = &tcbs[1];
	tcbs[1].nextPt = &tcbs[2];
	tcbs[2].nextPt = &tcbs[0];
	
	
	osKernelStackInit(0);
	TCB_STACK[0][STACKSIZE-2] = (int32_t)(task0);
	
  osKernelStackInit(1);
	TCB_STACK[1][STACKSIZE-2] = (int32_t)(task1);
	
  osKernelStackInit(2);
	TCB_STACK[2][STACKSIZE-2] = (int32_t)(task2);
	
	currentPt =&tcbs[0];
	
	for(int i = 0;i<NUM_OF_THREADS;i++){
	   tcbs[i].sleepTime = 0;
	}
	__enable_irq();
	
		return 1;

	
}



void osKernelInit(void)
{
	MILLIS_PRESCALER =  (BUS_FREQ/1000);
	
	/*Frequency options :  1hz, 10hz, 100hz, 1KHz,10KHz, 100KHz*/
	osPeriodicTask_Init(periodic_events_execute,1000,6);
}


void osKernelLaunch(uint32_t quanta)
{
	 SysTick->CTRL =0;
	 SysTick->VAL  =0;
	 SysTick->LOAD =  (quanta*MILLIS_PRESCALER)-1;
	
	 SYSPRI3 =(SYSPRI3&0x00FFFFFF)|0xE0000000; // priority 7
	 SysTick->CTRL =  0x00000007;
	
	osSchedularLaunch();
	

}

void osThreadYield(void){

 SysTick->VAL =0;
 INTCTRL = 0x04000000; // trigger SysTick

}

void  SysTick_Handler(void){
  //Trigger PendSV
	INTCTRL  =  0x10000000;
}

uint32_t period_tick;

void osSchedulerPeriodicRR(void)
{

		 		 currentPt  =  currentPt->nextPt;

}

void osSchedulerRRWithSleep(void){
 
	 currentPt =  currentPt->nextPt;
	 
	while(currentPt->sleepTime >0){
	  currentPt = currentPt->nextPt;
	}
}


void (*PeriodicTask)(void);

void osPeriodicTask_Init(void(*task)(void), uint32_t freq, uint8_t priority){

	__disable_irq();
	PeriodicTask = task;

   RCC->APB1ENR |=0x4;
   TIM4->PSC =  16-1;    /* 16 000 000 / 16 = 1 000 000*/
	 TIM4->ARR =  (1000000/freq)-1;
   TIM4->CR1 =1;
	 
	 TIM4->DIER |=1;
	 NVIC_SetPriority(TIM4_IRQn,priority);
	 NVIC_EnableIRQ(TIM4_IRQn);
}


void TIM4_IRQHandler(void){
	
	TIM4->SR =0;
	(*PeriodicTask)();

}


void osSemaphoreInit(int32_t *semaphore,int32_t value){

	 *semaphore = value;
	
}


void osSignalSet(int *semaphore)
{
	 __disable_irq();
	  *semaphore +=1;
	 __enable_irq();
}

void osSignalWait(int32_t *semaphore)
{
	__disable_irq();
	
	while(*semaphore <=0)
	{
		__disable_irq();
		__enable_irq();
	}

	 *semaphore -=1;
	__enable_irq();
}

void osSignalCooperativeWait(int32_t *semaphore)
{
	__disable_irq();
	
	while(*semaphore<=0){
	 __disable_irq();
		osThreadYield();
		__enable_irq();
	}
	*semaphore -=1;
	 __enable_irq();
}

void osThreadSleep(uint32_t sleep_time){
   __disable_irq();
	currentPt->sleepTime =  sleep_time;
	 __enable_irq();
 	osThreadYield();
}