/*
   -schedular
	 -Thread Control Block
	 -Stack
*/

#include "osKernel.h"

#define NUM_OF_THREADS 3
#define STACKSIZE 100
#define NUM_PERIODIC_TASKS 2

typedef void(*taskT)(void);
#define NULL (void*)0
	

#define BUS_FREQ		16000000

uint32_t MILLIS_PRESCALER;
void osSchedularLaunch(void);

/*Systick priority register*/
#define SYSPRI3         (*((volatile uint32_t *)0xE000ED20))
#define INTCTRL         (*((volatile uint32_t *)0xE000ED04))



typedef struct{
  taskT task;
	uint32_t period;
}periodicTaskT;

static periodicTaskT	PeriodicTasks[NUM_PERIODIC_TASKS];
static uint32_t TimeMsec;
static uint32_t MaxPeriod;

struct tcb{
	
  int32_t *stackPt;
	struct tcb *nextPt;
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

uint8_t osKernelAddPeriodThreads( void(*thread1)(void),
																	uint32_t period1,
																	void(*thread2)(void),
																	uint32_t period2
																 )
{
	MaxPeriod  = period1 > period2 ? period1 : period2;
	uint32_t minPeriod =  period1 < period2 ? period1 :  period2;
	
	if(MaxPeriod %minPeriod !=0) return 0;
	
	PeriodicTasks[0].task  =  thread1;
	PeriodicTasks[0].period =  period1;
	PeriodicTasks[1].task   =  thread2;
  PeriodicTasks[1].period =  period2;

	return 1;
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
	
	__enable_irq();
	
		return 1;

	
}



void osKernelInit(void)
{
	MILLIS_PRESCALER =  (BUS_FREQ/1000);
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
	 if(TimeMsec < MaxPeriod) TimeMsec++;
	  else
			TimeMsec = 1;
		
		int i;
		 for(i=0;i<NUM_OF_THREADS;i++){
		   if((TimeMsec % PeriodicTasks[i].period)==0 && PeriodicTasks[i].task !=NULL){
			   PeriodicTasks[i].task();
			 }
		 }
		 
		 currentPt  =  currentPt->nextPt;

}

