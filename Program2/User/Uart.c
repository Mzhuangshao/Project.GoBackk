#include "Uart.h"

void Uart1_Init(void) // 115200bps@11.0592MHz
{
    SCON = 0x50;  // 8位数据,可变波特率
    AUXR |= 0x40; // 定时器时钟1T模式
    AUXR &= 0xFE; // 串口1选择定时器1为波特率发生器
    TMOD &= 0x0F; // 设置定时器模式
    TL1 = 0xE8;   // 设置定时初始值
    TH1 = 0xFF;   // 设置定时初始值
    ET1 = 0;      // 禁止定时器中断
    TR1 = 1;      // 定时器1开始计时
    ES = 1;       // 使能串口1中断
}

void SendData(unsigned char dat)
{
    while (UartBusy)
        ; // 等待前面的数据发送完成
    UartBusy = 1;
    SBUF = dat; // 写数据到UART数据寄存器
}

void Uart1_Isr(void) interrupt 4
{
    if (TI) // 检测串口1发送中断
    {
        TI = 0; // 清除串口1发送中断请求位
    }
    if (RI) // 检测串口1接收中断
    {
        RI = 0; // 清除串口1接收中断请求位
    }
    UartBusy = 0;
}