#include "C51.h"
#include "Key.h"
#include "Uart.h"
#include <string.h>
#include <stdio.h>
void SendData(unsigned char dat);

// sbit sensorGo = P0 ^ 1;   // 光电传感器检测触发，视为出发，触发标志
// sbit sensorBack = P0 ^ 2; // 光电传感器检测触发，视为返回，触发标志

static bit trigger_1ms = 0;  // 1ms触发标志
static bit trigger_10ms = 0; // 10ms触发标志

static unsigned char RunningState = 0; // 运行状态检查
static unsigned char KeyColdDown = 0;  // 按键冷却状态
// static unsigned char UartColdDown = 0;                       // 串口冷却状态
static unsigned int minute = 0, second = 0, millisecond = 0; // 时间计数

void Timer0_Init(void) // 1毫秒@11.0592MHz
{
    AUXR |= 0x80; // 定时器时钟1T模式
    TMOD &= 0xF0; // 设置定时器模式
    TL0 = 0xCD;   // 设置定时初始值
    TH0 = 0xD4;   // 设置定时初始值
    TF0 = 0;      // 清除TF0标志
    TR0 = 1;      // 定时器0开始计时
    ET0 = 1;      // 使能定时器0中断
}

void Timer1_Init(void) // 10毫秒@11.0592MHz
{
    AUXR &= 0xBF; // 定时器时钟12T模式
    TMOD &= 0x0F; // 设置定时器模式
    TL1 = 0x00;   // 设置定时初始值
    TH1 = 0xDC;   // 设置定时初始值
    TF1 = 0;      // 清除TF1标志
    TR1 = 1;      // 定时器1开始计时
    ET1 = 1;      // 使能定时器1中断
}

void StateCheck() // 按键[复位],复位时间计数
{
    if (KeyResetState == 1) // 按键[复位]
    {
        KeyResetState = 0;
        RunningState += 1;
    }
}

void TimeCount() // 时间计数
{
    millisecond++;           // 时间计数
    if (millisecond >= 1000) // 秒数+1
    {
        second++;
        millisecond = 0;
        if (second >= 60) // 分钟+1
        {
            minute++;
            second = 0;
        }
    }
}

void LED_Show(void)
{
    unsigned char tableHead[] = {
        // 数据包头 | 卡地址| 字节长度
        0x55, 0xAA, 0x01, 0x00};
    unsigned char tableDisplaySettting[] = {
        0xC0, // 指令码 - C0 = 发送内容到显示屏
        0x00, // 编号 - 存储在0号位置
        0x04, // 类型 - 显示数值
        0x00, // 显示模式 - 立即显示
        0x00, // 字移动速度 - 不移动
        0x00, // 字停留秒数 - 不停留
        0x03, // 颜色 - 01红 02绿 03黄 // 但屏幕是单色，所以本项实际上无效
    };
    unsigned char tableTimeData[16];
    unsigned char tableEnd = 0x00;
    unsigned char length = 0, i = 0, sum = 0;

    sprintf((char *)tableTimeData, "%02d:%02d.%02d", minute, second, (int)millisecond / 10);
    length = strlen((char *)tableTimeData); // 计算tableTimeData的实际长度
    // 计算tableDisplaySettting和tableTimeData的累加和
    for (i = 0; i < sizeof(tableDisplaySettting); i++)
        sum += tableDisplaySettting[i];
    for (i = 0; i < length; i++)
        sum += tableTimeData[i];
    // 提取最低的8位，作为校验码
    tableEnd = (unsigned char)(sum & 0xFF);

    // 字节长度 = tableDisplaySettting + tableTimeData + tableEnd 的长度
    tableHead[3] = sizeof(tableDisplaySettting) + length + 1;
    // 发送数据
    for (i = 0; i < sizeof(tableHead); i++)
        SendData(tableHead[i]);
    for (i = 0; i < sizeof(tableDisplaySettting); i++)
        SendData(tableDisplaySettting[i]);
    for (i = 0; i < length; i++)
        SendData(tableTimeData[i]);
    SendData(tableEnd); // 发送校验码
}

void main()
{
    Timer0_Init();
    Timer1_Init();
    Uart1_Init();
    P2PU |= 0x40; // P26 设为上拉电阻
    P0PU |= 0x08; // P03 设为上拉电阻
    EA = 1;
    while (1)
    {
        if (trigger_1ms == 1) // 1ms触发
        {
            trigger_1ms = 0;
            KeyCheck();   // 按键检测
            StateCheck(); // 按键[复位],复位时间计数
        }
        if (trigger_10ms == 1) // 10ms触发
        {
            trigger_10ms = 0;
            LED_Show(); // LED点阵显示时间
        }
    }
}

void Timer0_Isr(void) interrupt 1
{
    trigger_1ms = 1; // 1ms触发标志
    // if (RunningState == 1)
        TimeCount(); // 时间计数
}