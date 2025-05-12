#include "C51.h"
#include <string.h>
#include "Uart.h"
void TimeReset();

static bit trigger_1ms = 0, trigger_10ms = 0;
static bit KeyReset_State = 0;

static unsigned char trigger_10ms_count = 0;
static unsigned char RunningState = 0;
static unsigned int minutes = 0, seconds = 0, milliseconds = 0; // 时间变量
static unsigned char tableDebug[16];

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

void KeyCheck(void)
{
    static unsigned char Key_Status_Check = 0; // 定义按键状态判断，0为默认状态，1为去抖检测，2表示按键松开
    switch (Key_Status_Check)
    {
    case 0:                       // 默认状态
        if (P32 == 0)             // 检测按键电位为0
            Key_Status_Check = 1; // 视按键为按下，进入去抖检测
        break;

    case 1:           // 判断按键是否真的按下
        if (P32 == 0) // 检测按键电位为低，若这一步仍为0，则判定这次按下通过
        {
            KeyReset_State = 1;   // 按键按下状态标记
            Key_Status_Check = 2; // 进入下一个状态：等待按键松开
        }
        else                      // 按键电位变高，说明仍有干扰，返回默认状态
            Key_Status_Check = 0; // 检测状态重置为默认
        break;

    case 2:                       // 等待按键松开
        if (P32 == 1)             // 按键电位为高，视为松开
            Key_Status_Check = 0; // 检测状态重置为默认
        break;
    default:
        Key_Status_Check = 0; // 其它情况均重置为默认
        break;
    }
}

void StateCheck(void) // 状态检测
{
    if (KeyReset_State)
    {
        KeyReset_State = 0;
        RunningState = 0;
        TimeReset();
    }
    switch (RunningState)
    {
    case 0:
        if (P35 == 0)
            RunningState = 1;
        break;
    case 1:
        if (P36 == 0)
            RunningState = 2;
        break;
    default:
        break;
    }
    if (RunningState == 3) // 运行状态检查
        RunningState = 0;
}

void TimeReset(void)
{
    minutes = 0;
    seconds = 0;
    milliseconds = 0;
}
void TimeCal(void)
{
    milliseconds++;
    if (milliseconds >= 1000)
    {
        milliseconds = 0;
        seconds++;
        if (seconds >= 60)
        {
            seconds = 0;
            minutes++;
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

    sprintf((char *)tableTimeData, "%02d:%02d.%02d", minutes, seconds, (int)milliseconds / 10);
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

void main(void)
{
    Timer0_Init();
    Uart1_Init();
    P3M0 &= ~0x74;
    P3M1 &= ~0x74; // P32 P34 P35 P36 准双向口
    EA = 1;
    while (1)
    {
        StateCheck();
        if (trigger_1ms)
        {
            trigger_1ms = 0;
            KeyCheck();
        }
        if (trigger_10ms)
        {
            trigger_10ms = 0;
            LED_Show();
            // sprintf((char *)tableDebug, "State:%d", (int)RunningState);
            // if (++i >= 10)
            // {
            //     LED_Show(); // 暂时禁用
            //     SendMsg((char *)tableDebug);
            //     i = 0;
            // }
        }
    }
}

// 1ms定时器中断服务程序
void Timer0_Isr(void) interrupt 1
{
    TF0 = 0; // 清除定时器0中断标志
    trigger_1ms = 1;
    if (++trigger_10ms_count == 10) // 10ms触发计数
    {
        trigger_10ms_count = 0;
        trigger_10ms = 1;
    }
    if (RunningState == 1)
        TimeCal();
}
