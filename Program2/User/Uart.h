#ifndef __UART_H__
#define __UART_H__

#include "C51.h"
static bit UartBusy = 0;

void Uart1_Init(void);
void SendData(unsigned char dat);

#endif