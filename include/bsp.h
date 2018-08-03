#ifndef BSP_H_
#define BSP_H_

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
// Per le PRIu32 ecc
#include <inttypes.h>

// Sistema operativo
#include "cmsis_os.h"

// Log
#include "esp_log.h"


// Questi mancano
// ==========================================

#define MIN(a, b)			((a) < (b) ? (a) : (b))
#define MAX(a, b)			((a) > (b) ? (a) : (b))

#define UNUSED(x)           (void)(sizeof(x))
#define NOT(x)              (~(unsigned int) (x))
#define ABS(x)				(x < 0 ? -(x) : x)

#define DIM_VECT(a)         sizeof(a) / sizeof(a[0])

// Debug
// ==========================================

#ifdef NDEBUG
#   define CHECK_IT(a)      (void)(a)

#   define BPOINT
#else
#   define CHECK_IT(a)      assert(a)

#   define BPOINT			__asm__("break 0,0")
#endif


// Print
// ==========================================

	// Every file can enable its DBG_xxx
#ifdef STAMPA_DBG
#	define DBG_ABIL					1

#	define DBG_ERR						ESP_LOGE("file", "%s[%d]", __FILE__, __LINE__) ;
#	define DBG_PRINTF(f, ...)			ESP_LOGD("dbg", f, ##__VA_ARGS__)
#	define DBG_PUTS(a)					ESP_LOGD("dbg", a)
#else
#	define DBG_ERR
#	define DBG_PRINTF(f, ...)
#	define DBG_PUTS(a)
#endif


#endif
