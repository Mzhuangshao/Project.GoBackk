#include "C51.h"
#include <string.h>

static bit UartBusy = 0;
static bit trigger_1ms = 0;
static bit trigger_10ms = 0;
static unsigned char KeyState = 0;

static unsigned char ColdDown = 0;
static unsigned char KeyColdDown = 0;
static unsigned char trigger_10ms_count = 0;
static unsigned char RunningState = 0;
static unsigned int minutes = 0, seconds = 0, milliseconds = 0; // 时间变量
static unsigned char tableState[16];
unsigned char i = 0;

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

void SendData(unsigned char dat)
{
    while (UartBusy)
        ; // 等待前面的数据发送完成
    UartBusy = 1;
    SBUF = dat; // 写数据到UART数据寄存器
}
void SendMsg(char *s)
{
    while (*s) // 检测字符串结束标志
    {
        SendData(*s++); // 发送当前字符
    }
}

void KeyCheck() // 按键检测
{
    if (P03 == 0)
    {
        NOP5();
        if (P03 == 0)
            RunningState++;
    }
}

void StateCheck() // 状态检测
{
    if (RunningState >= 10) // 运行状态检查
        RunningState = 0;
}

void TimeCal()
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
            // if (minutes >= 60)
            // {
            //     minutes = 0;
            // }
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
    P0PU |= 0x08; 
    EA = 1;
    while (1)
    {
        KeyCheck();
        StateCheck();
        if (trigger_1ms)
        {
            trigger_1ms = 0;
            if (RunningState == 11) // 暂时禁用
                TimeCal();          // 暂时禁用
        }
        if (trigger_10ms)
        {
            trigger_10ms = 0;
            if (RunningState == 11) // 暂时禁用
                LED_Show();         // 暂时禁用

            sprintf((char *)tableState, "State:%d", (int)RunningState);
            if (++i >= 10)
            {
                SendMsg((char *)tableState);
                i = 0;
            }
        }
    }
}

// 1ms定时器中断服务程序
void Timer0_Isr(void) interrupt 1
{
    trigger_1ms = 1;
    if (++trigger_10ms_count >= 10) // 10ms触发计数
    {
        trigger_10ms_count = 0;
        trigger_10ms = 1;
    }
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