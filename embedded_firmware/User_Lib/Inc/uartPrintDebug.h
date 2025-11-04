/*
 * uartPrintDebug.h
 *
 *  Created on: Dec 25, 2024
 *      Author: LHGiang
 */

#ifndef UARTPRINTDEBUG_H_
#define UARTPRINTDEBUG_H_
#include "usb_device.h"
#include "main.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
//int _write(int file, char *ptr, int len) {
//    CDC_Transmit_FS((uint8_t*) ptr, len); return len;
//}
#include <stdio.h>

// Bật/tắt debug (1: bật, 0: tắt)
#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
    #define DEBUG_PRINT(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(fmt, ...) // Không làm gì cả
#endif

#endif /* UARTPRINTDEBUG_H_ */
