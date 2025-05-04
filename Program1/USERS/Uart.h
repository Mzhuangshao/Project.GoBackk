#ifndef __UART_H__
#define __UART_H__

#include "C51.h"

static bit UartBusy = 0, Uartwork = 0; // 串口工作标志
static unsigned char UartColdDown = 0;                         // UART冷却计数变量

void Uart1_Init(void);
void SendData(unsigned char dat);
void Uart1_Isr(void);

#endif