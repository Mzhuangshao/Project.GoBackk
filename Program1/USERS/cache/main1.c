#include "C51.h"
#include <string.h>
#include <stdio.h>

sbit sensorGo = P0 ^ 1;   // 光电传感器检测触发，视为出发，触发标志
sbit sensorBack = P0 ^ 2; // 光电传感器检测触发，视为返回，触发标志
sbit KeyReset = P0 ^ 3;   // 按键[复位]

static bit trigger_1ms = 0;   // 1ms触发标志
static bit KeyResetState = 0; // 按键[复位]的状态
static bit UartBusy = 0;
void SendData(unsigned char dat);
void SendMsg(char *s);

static unsigned char RunningState = 0;            // 运行状态检查
static unsigned char KeyColdDown = 0;             // 按键消抖用 冷却计数变量
static unsigned char UartColdDown = 0;            // UART冷却计数变量
unsigned char minute = 45, second = 40; // 时间计数
unsigned int millisecond = 0;                     // 时间计数

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

// void KeyCheck() // 按键检测
// {
//     if (KeyReset == 1)
//     {
//         KeyResetState = 1; // 按键[复位]状态
//         KeyReset = 0;      // 复位按键
//     }
// }

// void StateCheck() // 按键[复位],复位时间计数
// {
//     if (KeyResetState == 1) // 复位按键动作
//     {
//         KeyResetState = 0; // 复位按键[复位]状态
//         RunningState = 0;  // 停止计时
//         millisecond = 0;   // 时间计数清零
//         second = 0;
//         minute = 0;
//         hour = 0;
//     }
//     switch (RunningState) // 运行状态检查
//     {
//     case 0:                // 起跑检测
//         if (sensorGo == 0) // 触发出发
//         {
//             RunningState = 1; // 开始计时
//             sensorGo = 0;     // 复位触发标志
//         }
//         break;
//     case 1:                  // 回程检测
//         if (sensorBack == 0) // 触发返回
//         {
//             RunningState = 2; // 停止计时
//             sensorBack = 0;   // 复位触发标志
//         }
//     case 2:
//         if (KeyResetState == 1)
//         {
//             KeyResetState = 0; // 复位按键[复位]状态
//             RunningState = 0;  // 停止计时
//             millisecond = 0;   // 时间计数清零
//             second = 0;
//             minute = 0;
//             hour = 0;
//         }
//         break;
//     default:
//         break;
//     }
// }

// void TimeCount() // 时间计数
// {
//     millisecond++;           // 时间计数
//     if (millisecond >= 1000) // 秒数+1
//     {
//         second++;
//         millisecond = 0;
//         if (second >= 60) // 分钟+1
//         {
//             minute++;
//             second = 0;
//         }
//     }
// }

void LED_Show(void) // LED点阵显示时间
{
    /*
    字节长度 = tableDisplaySettting + tableTimeData + tableEnd 的长度
    数据包尾校验码 = tableDisplaySettting + tableTimeData 的累加和
    */

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
    unsigned char tableTimeData[32];
    unsigned char tableEnd = 0x00;
    unsigned char len = 0, i = 0, sum = 0;

    // 构造tableTimeData - 时间字符串 // 格式：分:秒.两位毫秒
    sprintf(tableTimeData, "%d:%d\r\n", (short)minute,(char)second);
    len = strlen(tableTimeData);
    for (i = 0; i < len; i++)
        SendData(tableTimeData[i]);
    // 计算tableDisplaySettting和tableTimeData的累加和
    // for (i = 0; i < sizeof(tableDisplaySettting); i++)
    //     sum += tableDisplaySettting[i];
    // for (i = 0; i < len; i++)
    //     sum += tableTimeData[i];
    // // 提取最低的8位，作为校验码
    // tableEnd = (unsigned char)(sum & 0xFF);

    // 字节长度 = tableDisplaySettting + tableTimeData + tableEnd 的长度
    // tableHead[3] = sizeof(tableDisplaySettting) + len + 1;
    // 发送数据
    // for (i = 0; i < sizeof(tableHead); i++)
    //     SendData(tableHead[i]);
    // for (i = 0; i < sizeof(tableDisplaySettting); i++)
    //     SendData(tableDisplaySettting[i]);

    // SendData(tableEnd); // 发送校验码
}

void SendData(unsigned char dat)
{
    while (UartBusy)
        ; // 等待前面的数据发送完成
    UartBusy = 1;
    SBUF = dat; // 写数据到UART数据寄存器
}

/*----------------------------
发送字符串
----------------------------*/
void SendMsg(char *s)
{
    while (*s) // 检测字符串结束标志
    {
        SendData(*s++); // 发送当前字符
    }
}

void main()
{
    Timer0_Init();
    Uart1_Init();
    EA = 1;
    while (1)
    {
        if (UartColdDown > 90)
            LED_Show(); // LED点阵显示时间
        // KeyCheck();   // 按键检测
        // StateCheck(); // 按键[复位],复位时间计数
    }
}

void Timer0_Isr(void) interrupt 1
{
    TF0 = 0;                 // 清除TF0标志
    trigger_1ms = 1;         // 1ms触发标志
    // TimeCount();             // 时间计数
    if (++KeyColdDown == 80) // 按键消抖
        KeyColdDown = 0;
    if (++UartColdDown == 100)
        UartColdDown = 0;
    // if (RunningState == 1)
}

void Uart1_Isr(void) interrupt 4
{
    if (TI) // 检测串口1发送中断
    {
        TI = 0;       // 清除串口1发送中断请求位
        UartBusy = 0; // 清忙标志
    }
    if (RI) // 检测串口1接收中断
    {
        RI = 0; // 清除串口1接收中断请求位
    }
}