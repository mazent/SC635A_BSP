#include "uspc.h"
#include "esp_log.h"

#include "driver/uart.h"
#include "cirbu.h"

#define TAG		"uspc"

#define BAUD		115200

#define USPC_TXD  	(GPIO_NUM_1)
#define USPC_RXD  	(GPIO_NUM_3)
#define USPC_RTS  	(UART_PIN_NO_CHANGE)
#define USPC_CTS  	(UART_PIN_NO_CHANGE)
#define USPC_UART	UART_NUM_0

static bool opened = false ;

static S_USPC_CFG uCfg ;

#define NUM_EVN		20
static QueueHandle_t evnQ = NULL ;

static osThreadId tid = NULL ;

// requests from api

#define S_QUIT		1

static const uart_event_t req_quit = {
	.type = UART_EVENT_MAX,
	.size = S_QUIT
} ;

// response to api

osMessageQDef(respR, 1, bool) ;

typedef struct {
	osMessageQId waitHere ;

	void * buf ;
	uint16_t dim ;
} RESP ;

static RESP resp = {
	.waitHere = NULL
} ;

osMessageQDef(respQ, 1, RESP *) ;
static osMessageQId respQ = NULL ;

static void uspcThd(void * v)
{
	UN_BUFFER * msg = (UN_BUFFER *) osPoolAlloc(uCfg.mp) ;
	bool cont = true ;
    uart_event_t event;
//    size_t buffered_size;

    esp_log_level_set(TAG, ESP_LOG_NONE) ;

    // Ok, i'm ready
    CHECK_IT( osOK == osMessagePut(respQ, (uint32_t) &resp, 0) ) ;

    while (cont) {
        //Waiting for UART event.
        if (xQueueReceive(evnQ, (void * )&event, (portTickType)portMAX_DELAY)) {
            switch (event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                	do {
    					if (NULL == msg) {
    						// Riprovo
    						msg = (UN_BUFFER *) osPoolAlloc(uCfg.mp) ;

    						ESP_LOGE(TAG, "buffer esauriti") ;
    						if (NULL == msg)
    							break ;
    					}
    					msg->orig = UART ;

                        ESP_LOGI(TAG, "[UART DATA]: %d", event.size);

                        msg->dim = uart_read_bytes(USPC_UART, msg->mem, event.size, portMAX_DELAY) ;

                        uCfg.msg(msg) ;
                        msg = (UN_BUFFER *) osPoolAlloc(uCfg.mp) ;
                	} while (false) ;
                    break;
				//Event of UART RX break detected
				case UART_BREAK:
					ESP_LOGI(TAG, "uart rx break");
					break;
				//Event of UART ring buffer full
				case UART_BUFFER_FULL:
					ESP_LOGI(TAG, "ring buffer full");
					// If buffer full happened, you should consider encreasing your buffer size
					// As an example, we directly flush the rx buffer here in order to read more data.
					uart_flush_input(USPC_UART);
					xQueueReset(evnQ);
					break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(USPC_UART);
                    xQueueReset(evnQ);
                    break;
				//Event of UART frame error
				case UART_FRAME_ERR:
					ESP_LOGI(TAG, "uart frame error");
					break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(TAG, "uart parity error");
                    break;
                case UART_DATA_BREAK:
                	break ;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
//                    uart_get_buffered_data_len(USPC_UART, &buffered_size);
//                    int pos = uart_pattern_pop_pos(USPC_UART);
//                    ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
//                    if (pos == -1) {
//                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
//                        // record the position. We should set a larger queue size.
//                        // As an example, we directly flush the rx buffer here.
//                        uart_flush_input(USPC_UART);
//                    } else {
//                        uart_read_bytes(USPC_UART, dtmp, pos, 100 / portTICK_PERIOD_MS);
//                        uint8_t pat[PATTERN_CHR_NUM + 1];
//                        memset(pat, 0, sizeof(pat));
//                        uart_read_bytes(USPC_UART, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
//                        ESP_LOGI(TAG, "read data: %s", dtmp);
//                        ESP_LOGI(TAG, "read pat : %s", pat);
//                    }
                    break;
                case UART_EVENT_MAX:
                	switch (event.size) {
                	case S_QUIT:
                		cont = false ;
                		break ;
                	}
                	break ;
                //Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }

	(void) osThreadTerminate(NULL) ;
	tid = NULL ;

	CHECK_IT( osOK == osMessagePut(resp.waitHere, true, 0) ) ;
}


#define STACK_SIZE		2000
osThreadDef(uspcThd, osPriorityNormal, 0, STACK_SIZE) ;


static const uart_config_t uart_config = {
	.baud_rate = BAUD,
	.data_bits = UART_DATA_8_BITS,
	.parity    = UART_PARITY_DISABLE,
	.stop_bits = UART_STOP_BITS_1,
	.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
};


bool USPC_open(S_USPC_CFG * cfg)
{
	do {
		if (opened)
			break ;

		uCfg = *cfg ;

		esp_err_t err = uart_param_config(USPC_UART, &uart_config);
		if (err != ESP_OK)
			break ;

		err = uart_set_pin(USPC_UART, USPC_TXD, USPC_RXD, USPC_RTS, USPC_CTS);
		if (err != ESP_OK)
			break ;

		err = uart_driver_install(USPC_UART, DIM_BUFFER, 0, NUM_EVN, &evnQ, 0);
		if (err != ESP_OK)
			break ;

		if (NULL == respQ) {
			respQ = osMessageCreate(osMessageQ(respQ), NULL) ;
			assert(respQ) ;
			if (NULL == respQ)
				break ;
		}

		if (NULL == resp.waitHere) {
			resp.waitHere = osMessageCreate(osMessageQ(respR), NULL) ;
			assert(resp.waitHere) ;
			if (NULL == resp.waitHere)
				break ;
		}

		if (NULL == tid) {
			tid = osThreadCreate(osThread(uspcThd), NULL) ;
			assert(tid) ;
			if (NULL == tid)
				break ;
		}

		opened = true ;

	} while (false) ;

	return opened ;
}

void USPC_close(void)
{
	if (opened) {
		osEvent evn = osMessageGet(respQ, osWaitForever) ;
		assert(osEventMessage == evn.status) ;

		if (osEventMessage == evn.status) {
			CHECK_IT(pdTRUE == xQueueSend(evnQ, &req_quit, portMAX_DELAY)) ;

			evn = osMessageGet(resp.waitHere, osWaitForever) ;
			assert(osEventMessage == evn.status) ;

			(void) uart_driver_delete(USPC_UART) ;
			evnQ = NULL ;

			opened = false ;
		}
	}
}

bool USPC_tx(const void * v, uint16_t dim)
{
	bool esito = false ;

	if (!opened) {
	}
	else if (NULL == v) {
	}
	else if (0 == dim) {
	}
	else
		esito = dim == uart_write_bytes(USPC_UART, (const char *) v, dim) ;

	return esito ;
}
