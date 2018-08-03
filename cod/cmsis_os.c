/*
 * Tutorial: https://os.mbed.com/handbook/CMSIS-RTOS
 * API: https://arm-software.github.io/CMSIS_5/RTOS/html/group__CMSIS__RTOS.html
 */

#include "bsp.h"

#include "cmsis_os.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static TickType_t ms_in_tick(uint32_t millisec)
{
	TickType_t ticks = 1 ;

	assert(portTICK_PERIOD_MS) ;

	if (millisec == osWaitForever)
	    ticks = portMAX_DELAY ;
	else if (0 == millisec)
		return 0 ;
	else {
		// Rounding
	    ticks = 1 + (millisec - 1) / portTICK_PERIOD_MS ;
	}

	return ticks ;
}

static unsigned portBASE_TYPE freeRtos_pri(osPriority priority)
{
	assert(configMAX_PRIORITIES >= 7) ;

	switch (priority) {
	case osPriorityIdle:		return tskIDLE_PRIORITY ;
	case osPriorityLow:			return tskIDLE_PRIORITY + 1 ;
	case osPriorityBelowNormal: return tskIDLE_PRIORITY + 2 ;
	case osPriorityNormal:      return tskIDLE_PRIORITY + 3 ;
	case osPriorityAboveNormal: return tskIDLE_PRIORITY + 4 ;
	case osPriorityHigh:        return tskIDLE_PRIORITY + 5 ;
	case osPriorityRealtime:    return tskIDLE_PRIORITY + 6 ;
	default:					return tskIDLE_PRIORITY ;
	}
}

static QueueHandle_t mzQueueCreate(const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize, const char * nome)
{
	QueueHandle_t q = xQueueCreate(uxQueueLength, uxItemSize) ;
#if ( configQUEUE_REGISTRY_SIZE > 0 )
    vQueueAddToRegistry(q, nome) ;
#else
    UNUSED(nome) ;
#endif
	return q ;
}


//  ==== Kernel Control Functions ====

///// Initialize the RTOS Kernel for creating objects.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osKernelInitialize shall be consistent in every CMSIS-RTOS.
//osStatus osKernelInitialize (void);
//
///// Start the RTOS Kernel.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osKernelStart shall be consistent in every CMSIS-RTOS.
//osStatus osKernelStart (void);
//
///// Check if the RTOS kernel is already started.
///// \note MUST REMAIN UNCHANGED: \b osKernelRunning shall be consistent in every CMSIS-RTOS.
///// \return 0 RTOS is not started, 1 RTOS is started.
//int32_t osKernelRunning(void);
//
//#if (defined (osFeature_SysTick)  &&  (osFeature_SysTick != 0))     // System Timer available
//
///// Get the RTOS kernel system timer counter
///// \note MUST REMAIN UNCHANGED: \b osKernelSysTick shall be consistent in every CMSIS-RTOS.
///// \return RTOS kernel system timer as 32-bit value
//uint32_t osKernelSysTick (void);
//
///// The RTOS kernel system timer frequency in Hz
///// \note Reflects the system timer setting and is typically defined in a configuration file.
//#define osKernelSysTickFrequency 100000000
//
///// Convert a microseconds value to a RTOS kernel system timer value.
///// \param         microsec     time value in microseconds.
///// \return time value normalized to the \ref osKernelSysTickFrequency
//#define osKernelSysTickMicroSec(microsec) (((uint64_t)microsec * (osKernelSysTickFrequency)) / 1000000)
//
//#endif    // System Timer available

//  ==== Estensioni ====

void * os_malloc(size_t dim)
{
	if (dim)
		return pvPortMalloc(dim) ;
	else
		return NULL ;
}

void os_free(void * v)
{
	if (v)
		vPortFree(v) ;
}


//  ==== Thread Management ====

osThreadId osThreadCreate(const osThreadDef_t * td, void * argument)
{
	TaskHandle_t handle ;

	if (xTaskCreatePinnedToCore(td->pthread,
					td->nome,
	                (td->stacksize + sizeof(portSTACK_TYPE) - 1) / sizeof(portSTACK_TYPE),
	                argument,
	                freeRtos_pri(td->tpriority),
	                &handle,
					portNUM_PROCESSORS - 1) != pdPASS) {
	    return NULL ;
	}

	return handle ;
}

///// Return the thread ID of the current running thread.
///// \return thread ID for reference by other functions or NULL in case of error.
///// \note MUST REMAIN UNCHANGED: \b osThreadGetId shall be consistent in every CMSIS-RTOS.
//osThreadId osThreadGetId (void);

#if (INCLUDE_vTaskDelete == 1)
osStatus osThreadTerminate(osThreadId thread_id)
{
    vTaskDelete(thread_id) ;

    return osOK ;
}
#endif

///// Pass control to next thread that is in state \b READY.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osThreadYield shall be consistent in every CMSIS-RTOS.
//osStatus osThreadYield (void);
//
///// Change priority of an active thread.
///// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
///// \param[in]     priority      new priority value for the thread function.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osThreadSetPriority shall be consistent in every CMSIS-RTOS.
//osStatus osThreadSetPriority (osThreadId thread_id, osPriority priority);
//
///// Get current priority of an active thread.
///// \param[in]     thread_id     thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
///// \return current priority value of the thread function.
///// \note MUST REMAIN UNCHANGED: \b osThreadGetPriority shall be consistent in every CMSIS-RTOS.
//osPriority osThreadGetPriority (osThreadId thread_id);


//  ==== Generic Wait Functions ====

osStatus osDelay(uint32_t millisec)
{
	if (0 == millisec)
		millisec = 1 ;

	vTaskDelay( ms_in_tick(millisec) ) ;

	return osOK ;
}

#if (defined (osFeature_Wait)  &&  (osFeature_Wait != 0))     // Generic Wait available

///// Wait for Signal, Message, Mail, or Timeout.
///// \param[in] millisec          \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out
///// \return event that contains signal, message, or mail information or error code.
///// \note MUST REMAIN UNCHANGED: \b osWait shall be consistent in every CMSIS-RTOS.
//osEvent osWait (uint32_t millisec);

#endif  // Generic Wait available


//  ==== Timer Management Functions ====

///// Create a timer.
///// \param[in]     timer_def     timer object referenced with \ref osTimer.
///// \param[in]     type          osTimerOnce for one-shot or osTimerPeriodic for periodic behavior.
///// \param[in]     argument      argument to the timer call back function.
///// \return timer ID for reference by other functions or NULL in case of error.
///// \note MUST REMAIN UNCHANGED: \b osTimerCreate shall be consistent in every CMSIS-RTOS.
//osTimerId osTimerCreate (const osTimerDef_t *timer_def, os_timer_type type, void *argument);
//
///// Start or restart a timer.
///// \param[in]     timer_id      timer ID obtained by \ref osTimerCreate.
///// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue "time delay" value of the timer.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osTimerStart shall be consistent in every CMSIS-RTOS.
//osStatus osTimerStart (osTimerId timer_id, uint32_t millisec);
//
///// Stop the timer.
///// \param[in]     timer_id      timer ID obtained by \ref osTimerCreate.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osTimerStop shall be consistent in every CMSIS-RTOS.
//osStatus osTimerStop (osTimerId timer_id);
//
///// Delete a timer that was created by \ref osTimerCreate.
///// \param[in]     timer_id      timer ID obtained by \ref osTimerCreate.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osTimerDelete shall be consistent in every CMSIS-RTOS.
//osStatus osTimerDelete (osTimerId timer_id);


//  ==== Signal Management ====

osStatus osSignalSet(osThreadId thread_id, int32_t signal)
{
	assert(signal >= 0) ;

    if ( xPortInIsrContext() ) {
        if (xTaskNotifyFromISR( thread_id, (uint32_t)signal, eSetBits, NULL ) != pdPASS )
            return osErrorOS ;

        portYIELD_FROM_ISR() ;
    }
    else if (xTaskNotify( thread_id, (uint32_t)signal, eSetBits) != pdPASS )
        return osErrorOS ;

    return osOK ;
}

osEvent osSignalWait(int32_t signals, uint32_t millisec)
{
	osEvent ret = { 0 } ;
	TickType_t ticks = ms_in_tick(millisec) ;

	assert(signals >= 0) ;
	assert( !xPortInIsrContext() ) ;

	if (0 == signals)
		signals = NOT(0x80000000) ;

	if (xTaskNotifyWait(
			0, // ulBitsToClearOnEntry: non cancello niente quando mi metto in attesa
			(uint32_t) signals,
			(uint32_t *) &ret.value.signals,
			ticks) != pdTRUE)
		ret.status = osEventTimeout ;
	else if (ret.value.signals < 0)
		ret.status = osErrorValue ;
	else
		ret.status = osEventSignal ;

	return ret ;
}


//  ==== Mutex Management ====

///// Create and Initialize a Mutex object.
///// \param[in]     mutex_def     mutex definition referenced with \ref osMutex.
///// \return mutex ID for reference by other functions or NULL in case of error.
///// \note MUST REMAIN UNCHANGED: \b osMutexCreate shall be consistent in every CMSIS-RTOS.
//osMutexId osMutexCreate (const osMutexDef_t *mutex_def);
//
///// Wait until a Mutex becomes available.
///// \param[in]     mutex_id      mutex ID obtained by \ref osMutexCreate.
///// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osMutexWait shall be consistent in every CMSIS-RTOS.
//osStatus osMutexWait (osMutexId mutex_id, uint32_t millisec);
//
///// Release a Mutex that was obtained by \ref osMutexWait.
///// \param[in]     mutex_id      mutex ID obtained by \ref osMutexCreate.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osMutexRelease shall be consistent in every CMSIS-RTOS.
//osStatus osMutexRelease (osMutexId mutex_id);
//
///// Delete a Mutex that was created by \ref osMutexCreate.
///// \param[in]     mutex_id      mutex ID obtained by \ref osMutexCreate.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osMutexDelete shall be consistent in every CMSIS-RTOS.
//osStatus osMutexDelete (osMutexId mutex_id);


//  ==== Semaphore Management Functions ====

#if (defined (osFeature_Semaphore)  &&  (osFeature_Semaphore != 0))     // Semaphore available

///// Create and Initialize a Semaphore object used for managing resources.
///// \param[in]     semaphore_def semaphore definition referenced with \ref osSemaphore.
///// \param[in]     count         number of available resources.
///// \return semaphore ID for reference by other functions or NULL in case of error.
///// \note MUST REMAIN UNCHANGED: \b osSemaphoreCreate shall be consistent in every CMSIS-RTOS.
//osSemaphoreId osSemaphoreCreate (const osSemaphoreDef_t *semaphore_def, int32_t count);
//
///// Wait until a Semaphore token becomes available.
///// \param[in]     semaphore_id  semaphore object referenced with \ref osSemaphoreCreate.
///// \param[in]     millisec      \ref CMSIS_RTOS_TimeOutValue or 0 in case of no time-out.
///// \return number of available tokens, or -1 in case of incorrect parameters.
///// \note MUST REMAIN UNCHANGED: \b osSemaphoreWait shall be consistent in every CMSIS-RTOS.
//int32_t osSemaphoreWait (osSemaphoreId semaphore_id, uint32_t millisec);
//
///// Release a Semaphore token.
///// \param[in]     semaphore_id  semaphore object referenced with \ref osSemaphoreCreate.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osSemaphoreRelease shall be consistent in every CMSIS-RTOS.
//osStatus osSemaphoreRelease (osSemaphoreId semaphore_id);
//
///// Delete a Semaphore that was created by \ref osSemaphoreCreate.
///// \param[in]     semaphore_id  semaphore object referenced with \ref osSemaphoreCreate.
///// \return status code that indicates the execution status of the function.
///// \note MUST REMAIN UNCHANGED: \b osSemaphoreDelete shall be consistent in every CMSIS-RTOS.
//osStatus osSemaphoreDelete (osSemaphoreId semaphore_id);

#endif     // Semaphore available


//  ==== Memory Pool Management Functions ====

#if (defined (osFeature_Pool)  &&  (osFeature_Pool != 0))  // Memory Pool Management available

/*
	+-----+  free  +--------+
	|     +-------->        |
	| USR |  alloc | liberi |
	|     <--------+        |
	+-----+        +--------+
*/

typedef struct os_pool_cb {
    QueueHandle_t liberi ;
    size_t dim_elem ;
    uint8_t * mem ;
} os_pool_cb_t ;

osPoolId osPoolCreate(const osPoolDef_t * pool_def)
{
	osPoolId pid = NULL ;
	os_pool_cb_t tmp = { .dim_elem = pool_def->item_sz } ;
	uint8_t * mem = NULL ;

	assert( !xPortInIsrContext() ) ;

	tmp.liberi = mzQueueCreate(pool_def->pool_sz, sizeof(void *), pool_def->nome) ;
	assert(tmp.liberi) ;
	if (NULL == tmp.liberi)
		goto err1 ;

	tmp.mem = pvPortMalloc(pool_def->pool_sz * pool_def->item_sz) ;
	assert(tmp.mem) ;
	if (NULL == tmp.mem)
		goto err2 ;

	// All'inizio tutti i buffer sono liberi
	mem = tmp.mem ;
	for (uint32_t i=0 ; i<pool_def->pool_sz ; i++, mem += pool_def->item_sz)
		(void) xQueueSend(tmp.liberi, &mem, 0) ;

	pid = pvPortMalloc(sizeof(os_pool_cb_t)) ;
	assert(pid) ;
	if (NULL == pid)
		goto err3 ;

	*pid = tmp ;
	goto err1 ;

err3:
	vPortFree(tmp.mem) ;
err2:
	vQueueDelete(tmp.liberi) ;
err1:
    return pid ;
}

void * osPoolAlloc(osPoolId pool_id)
{
	void * buf = NULL ;

	assert(pool_id) ;

	if (NULL == pool_id) {
	}
	else if (xPortInIsrContext()) {
		(void) xQueueReceiveFromISR(pool_id->liberi, &buf, NULL) ;

		portYIELD_FROM_ISR() ;
	}
	else
		(void) xQueueReceive(pool_id->liberi, &buf, 0) ;

	return buf ;
}

void * osPoolCAlloc(osPoolId pool_id)
{
	void * buf = osPoolAlloc(pool_id) ;
	if (buf)
		memset(buf, 0, pool_id->dim_elem) ;

	return buf ;
}

osStatus osPoolFree(osPoolId pool_id, void * buf)
{
	assert(pool_id) ;
	assert(buf) ;

	if (NULL == pool_id)
		return osErrorParameter ;
	else if (NULL == buf)
		return osErrorParameter ;
	else if (xPortInIsrContext()) {
		osStatus esito = osOK ;

		if (pdTRUE != xQueueSendFromISR(pool_id->liberi, &buf, NULL))
			esito = osErrorOS ;

		portYIELD_FROM_ISR() ;

		return esito ;
	}
	else if (pdTRUE == xQueueSend(pool_id->liberi, &buf, 0))
		return osOK ;
	else
		return osErrorOS ;
}

#endif   // Memory Pool Management available


//  ==== Message Queue Management Functions ====

#if (defined (osFeature_MessageQ)  &&  (osFeature_MessageQ != 0))     // Message Queues available

osMessageQId osMessageCreate(const osMessageQDef_t *queue_def, osThreadId thread_id)
{
	UNUSED(thread_id) ;

	assert( !xPortInIsrContext() ) ;

	return mzQueueCreate(queue_def->queue_sz, queue_def->item_sz, queue_def->nome) ;
}

osStatus osMessagePut(osMessageQId queue_id, uint32_t info, uint32_t millisec)
{
    TickType_t ticks = ms_in_tick(millisec) ;

    assert(queue_id) ;

    if (queue_id == NULL)
        return osErrorParameter ;
    else if ( xPortInIsrContext() ) {
        if (xQueueSendFromISR(queue_id, &info, NULL) != pdTRUE)
            return osErrorResource ;

        portYIELD_FROM_ISR() ;
    }
    else {
        if (xQueueSend(queue_id, &info, ticks) != pdTRUE)
            return osErrorResource ;
    }

    return osOK ;
}

osEvent osMessageGet(osMessageQId queue_id, uint32_t millisec)
{
    TickType_t ticks = ms_in_tick(millisec) ;
    osEvent event ;

    assert(queue_id) ;

    event.def.message_id = queue_id ;
    event.value.v = 0 ;

    if (queue_id == NULL) {
        event.status = osErrorParameter ;
        return event ;
    }

    if ( xPortInIsrContext() ) {
        if (xQueueReceiveFromISR(queue_id, &event.value.v, NULL) == pdTRUE) {
            /* We have mail */
            event.status = osEventMessage ;
        }
        else {
            event.status = osOK ;
        }
        portYIELD_FROM_ISR() ;
    }
    else {
        if (xQueueReceive(queue_id, &event.value.v, ticks) == pdTRUE) {
            /* We have mail */
            event.status = osEventMessage ;
        }
        else {
            event.status = (ticks == 0) ? osOK : osEventTimeout ;
        }
    }

    return event ;
}

#endif     // Message Queues available


//  ==== Mail Queue Management Functions ====

#if (defined (osFeature_MailQ)  &&  (osFeature_MailQ != 0))     // Mail Queues available

/*
    +-----+                       	  +-----+
	|     |   put  +---------+  get   |     |
	|     +--------> spedite +-------->     |
	|     |        +---------+        |     |
	| MIT |                           | DST |
	|     |  alloc +---------+  free  |     |
	|     <--------+  libere <--------+     |
	|     |        +---------+        |     |
	+-----+                           +-----+
*/

typedef struct os_mailQ_cb {
    const osMailQDef_t * queue_def ;

    QueueHandle_t libere ;
    QueueHandle_t spedite ;
    uint8_t * mem ;
} os_mailQ_cb_t ;


osMailQId osMailCreate(const osMailQDef_t * queue_def, osThreadId thread_id)
{
	osMailQId mq = NULL ;
	os_mailQ_cb_t tmp = { .queue_def = queue_def } ;
	uint8_t * mail = NULL ;

    assert( !xPortInIsrContext() ) ;

	tmp.spedite = mzQueueCreate(queue_def->queue_sz, sizeof(void *), queue_def->nome_o) ;
	assert(tmp.spedite) ;
	if (NULL == tmp.spedite)
		goto err1 ;

	tmp.libere = mzQueueCreate(queue_def->queue_sz, sizeof(void *), queue_def->nome_l) ;
	assert(tmp.libere) ;
	if (NULL == tmp.libere)
		goto err2 ;

	tmp.mem = pvPortMalloc(queue_def->queue_sz * queue_def->item_sz) ;
	assert(tmp.mem) ;
	if (NULL == tmp.mem)
		goto err3 ;

	// All'inizio tutte le mail sono disponibili
	mail = tmp.mem ;
	for (uint32_t i=0 ; i<queue_def->queue_sz ; i++, mail += queue_def->item_sz)
		(void) xQueueSend(tmp.libere, &mail, 0) ;

	mq = pvPortMalloc(sizeof(os_mailQ_cb_t)) ;
	assert(mq) ;
	if (NULL == mq)
		goto err4 ;

	*(queue_def->pId) = mq ;
	*mq = tmp ;
	goto err1 ;

err4:
	vPortFree(tmp.mem) ;
err3:
	vQueueDelete(tmp.libere) ;
err2:
	vQueueDelete(tmp.spedite) ;
err1:
    return mq ;
}

void os_MailDelete(osMailQId mq)
{
	assert( !xPortInIsrContext() ) ;
	
	if (mq) {
		if (mq->libere) {
			vQueueDelete(mq->libere) ;
			mq->libere = NULL ;
		}

		if (mq->spedite) {
			vQueueDelete(mq->spedite) ;
			mq->spedite = NULL ;
		}

		if (mq->mem) {
			vPortFree(mq->mem) ;
			mq->mem = NULL ;
		}

		vPortFree(mq) ;
	}
}

void * osMailAlloc(osMailQId queue_id, uint32_t millisec)
{
	void * mail = NULL ;

	assert(queue_id) ;

	if (NULL == queue_id) {
	}
	else if (xPortInIsrContext()) {
		(void) xQueueReceiveFromISR(queue_id->libere, &mail, NULL) ;

		portYIELD_FROM_ISR() ;
	}
	else {
		(void) xQueueReceive(queue_id->libere, &mail, millisec) ;
	}

	return mail ;
}

void * osMailCAlloc(osMailQId queue_id, uint32_t millisec)
{
	void * mail = osMailAlloc(queue_id, millisec) ;
	if (mail)
		memset(mail, 0, queue_id->queue_def->item_sz) ;

	return mail ;
}

osStatus osMailFree(osMailQId queue_id, void *mail)
{
	assert(queue_id) ;
	assert(mail) ;

	if (NULL == queue_id)
		return osErrorParameter ;
	else if (NULL == mail)
		return osErrorParameter ;
	else if (xPortInIsrContext()) {
		osStatus esito = osOK ;

		if (pdTRUE != xQueueSendFromISR(queue_id->libere, &mail, NULL))
			esito = osErrorOS ;

		portYIELD_FROM_ISR() ;

		return esito ;
	}
	else if (pdTRUE == xQueueSend(queue_id->libere, &mail, 0))
		return osOK ;
	else
		return osErrorOS ;
}

osStatus osMailPut(osMailQId queue_id, void * mail)
{
	assert(queue_id) ;
	assert(mail) ;

	if (NULL == queue_id)
		return osErrorParameter ;
	else if (NULL == mail)
		return osErrorParameter ;
	else if (xPortInIsrContext()) {
		osStatus esito = osOK ;

		if (pdTRUE != xQueueSendFromISR(queue_id->spedite, &mail, NULL))
			esito = osErrorOS ;

		portYIELD_FROM_ISR() ;

		return esito ;
	}
	else if (pdTRUE == xQueueSend(queue_id->spedite, &mail, 0))
		return osOK ;
	else
		return osErrorOS ;
}

osEvent osMailGet(osMailQId queue_id, uint32_t millisec)
{
	osEvent event ;

	assert(queue_id) ;

	if (NULL == queue_id)
		event.status = osErrorParameter ;
	else if (xPortInIsrContext()) {
#if 1
		// Dentro una interruzione gestite le mail?
		assert(false) ;
		event.status = osErrorISR ;
#else
		if (xQueueReceiveFromISR(queue_id->spedite, &event.value.p, NULL) == pdTRUE)
			event.status = osEventMail ;
		else
			event.status = osOK ;

		portYIELD_FROM_ISR() ;
#endif
	}
	else if (pdTRUE == xQueueReceive(queue_id->spedite, &event.value.p, millisec))
		event.status = osEventMail ;
	else
		event.status = osOK ;

	return event ;
}

#endif  // Mail Queues available

