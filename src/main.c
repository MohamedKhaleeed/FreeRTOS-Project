/*
 * This file is part of the ÂµOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#define CCM_RAM __attribute__((section(".ccmram")))

// ----------------------------------------------------------------------------

#include "led.h"

#define BLINK_PORT_NUMBER         (3)
#define BLINK_PIN_NUMBER_GREEN    (12)
#define BLINK_PIN_NUMBER_ORANGE   (13)
#define BLINK_PIN_NUMBER_RED      (14)
#define BLINK_PIN_NUMBER_BLUE     (15)
#define BLINK_ACTIVE_LOW          (false)

struct led blinkLeds[4];

// ----------------------------------------------------------------------------
/*-----------------------------------------------------------*/

/*
 * The LED timer callback function.  This does nothing but switch the red LED
 * off.
 */
/*-----------------------------------------------------------*/

/* The LED software timer.  This uses vLEDTimerCallback() as its callback
 * function.
 */
BaseType_t xTimer1Started, xTimer2Started, xTimer3Started;

/*-----------------------------------------------------------*/
// ----------------------------------------------------------------------------
//
// Semihosting STM32F4 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace-impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"

QueueHandle_t Q = 0;
TimerHandle_t Sender1Timer = 0;
TimerHandle_t Sender2Timer = 0;
TimerHandle_t ReceiverTimer = 0;
SemaphoreHandle_t Sender1Semaphore = 0;
SemaphoreHandle_t Sender2Semaphore = 0;
SemaphoreHandle_t ReceiverSemaphore = 0;

int Tsender1;
int Tsender2;
int Treceiver = 100;
int i =0;
int count_received = 0;
int count_blocked = 0;
int count_sent = 0;
int upper[6]= {150, 200, 250, 300,350, 400};
int lower[6]={50, 80, 110, 140, 170, 200};


void Sender1(void *p)
{
	char message[50];
	while(1)
	{
		//Is the SendSemaphore available take it and enter the operations under if condition
		//if it is not available then don't enter
		if(xSemaphoreTake(Sender1Semaphore,1000)==pdTRUE)
		{
			//Put the message "Time is Number_of_Ticks_now" in the array of character
			sprintf(message," %u",xTaskGetTickCount());
			//Send the message in message array to the queue:
			//if it is sent successfully as it was available space in the queue
			//then increment the counter of the sent messages by 1
			//if it is not sent because the queue is full
			//then increment the counter of the blocked messages by 1
			if(xQueueSend(Q,message, 100))
				count_sent++;
			else
				count_blocked++;
		}
	}
}

void Sender2(void *p)
{
	char message[50];
	while(1)
	{
	  //Is the SendSemaphore available take it and enter the operations under if condition
	  //if it is not available then don't enter
		if(xSemaphoreTake(Sender2Semaphore,1000)==pdTRUE)
		{
			//Put the message "Time is Number_of_Ticks_now" in the array of character
			sprintf(message," %u",xTaskGetTickCount());
			//Send the message in message array to the queue:
			//if it is sent successfully as it was available space in the queue
			//then increment the counter of the sent messages by 1
			//if it is not sent because the queue is full
			//then increment the counter of the blocked messages by 1
			if(xQueueSend(Q,(void*) message, 100==pdPASS))
				count_sent++;
			else
				count_blocked++;
		}
	}
}

void Receiver(void *p)
{
	char received_message[50];
	while(1)
	{
		//Is the RecieveSemaphore available take it and enter the operations under if condition
		//if it is not available then don't enter
		if(xSemaphoreTake(ReceiverSemaphore,1000)==pdTRUE)
		{
			//If queue is not empty then receive the message in the queue
			//and if it is received successfully then increment the counter of received messages
			//otherwise, don't do anything
		if(Q != 0)
				if(xQueueReceive(Q,received_message,100)==pdPASS)
				{trace_puts(received_message);
					count_received++;}
		}
	}
}

void sender1CallBack ()
{
	//Give SendSemaphore when this function called
	//after passing Tsender, the timer resets and call the sender1CallBack function again
	Tsender1=(rand() %(upper[i] - lower[i] + 1)) + lower[i];
	xTimerChangePeriod(Sender1Timer,Tsender1,100);
	xSemaphoreGive(Sender1Semaphore);

}

void sender2CallBack ()
{
	//Give SendSemaphore when this function called
	//after passing Tsender, the timer resets and call the sender2CallBack function again
	Tsender2=(rand() %(upper[i] - lower[i] + 1)) + lower[i];
	xTimerChangePeriod(Sender2Timer,Tsender2,100);
	xSemaphoreGive(Sender2Semaphore);
}

void receiverCallBack ()
{

	//If number of received messages reached 500 then call reset function
	//but if it still <500 then give the ReceiverSemaphore
	//after passing 200ms, the timer resets and call the receiverCallBack function again
	if (count_received==500)
		{i++;
		reset();
		}
	xSemaphoreGive(ReceiverSemaphore);
}

void reset()
{   if (!i==0){


	trace_printf("Total number of successfully sent messages: %d\n",count_sent);
		trace_printf("Total number of successfully blocked messages: %d\n",count_blocked);
		trace_printf("****************************************************************\n");}
        //Reset the counters
		count_sent=0;
		count_blocked=0;
		count_received=0;
        //Reset the queue and the ReceiverTimer
		//Update the period of SendTimer to the new value in the Tsender variable and reset SendTimer
		xQueueReset(Q);
		//If we accessed every element in the Tsender array and finished them all then enter the
		//operations under if condition
		//and delete all the tasks and timers
		//otherwise don't do anything
		if(i>5)
			{
			trace_printf("Game Over!!!!!");
			xTimerDelete(Sender1Timer,(TickType_t) 0);
			xTimerDelete(Sender2Timer,(TickType_t) 0);
			xTimerDelete(ReceiverTimer,(TickType_t) 0);
			vTaskEndScheduler();
			return ;
			}
}


int main(int argc, char* argv[])
{
	// Add your code here.
//	reset();
	Tsender1=(rand() % (upper[i] - lower[i] +1)) + upper[i];
	Tsender2=(rand() % (upper[i] - lower[i] +1)) + upper[i];
	char arr[50];
xTaskCreate(Sender1,"Sender",1000,(void*) 0,1,0);
xTaskCreate(Sender2,"Sender",1000,(void*) 0,1,0);
xTaskCreate(Receiver,"Receiver",1000,(void*) 0,2,0);

Sender1Timer = xTimerCreate("Timer1",pdMS_TO_TICKS(Tsender1),pdTRUE,1, sender1CallBack);
Sender2Timer = xTimerCreate("Timer2",pdMS_TO_TICKS(Tsender2),pdTRUE,1, sender2CallBack);
ReceiverTimer = xTimerCreate("Timer3",pdMS_TO_TICKS(Treceiver),pdTRUE,1, receiverCallBack);
Sender1Semaphore = xSemaphoreCreateBinary();
Sender2Semaphore = xSemaphoreCreateBinary();
ReceiverSemaphore = xSemaphoreCreateBinary();
Q = xQueueCreate(2,50*sizeof(char));

if( ( Sender1Timer != NULL ) && ( Sender2Timer != NULL ) && ( ReceiverTimer !=NULL) )
	{
		xTimer1Started = xTimerStart( Sender1Timer, 0 );
		xTimer2Started = xTimerStart( Sender2Timer, 0 );
		xTimer3Started = xTimerStart( ReceiverTimer, 0 );
}

	if( xTimer1Started == pdPASS && xTimer2Started == pdPASS && xTimer3Started == pdPASS)
	{
		vTaskStartScheduler();
	}

	return 0;
}


#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------



void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}

void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;

/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
