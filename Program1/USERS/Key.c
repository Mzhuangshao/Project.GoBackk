#include "Key.h"

void KeyCheck() // 按键检测
{
    static unsigned char Key_Status_Check = 0; // 定义按键状态判断，0为默认状态，1为去抖检测，2表示按键松开
    switch (Key_Status_Check)
    {
    case 0:                       // 默认状态
        if (P26 == 0)             // 检测按键电位为0
            Key_Status_Check = 1; // 视按键为按下，进入去抖检测
        break;

    case 1:           // 判断按键是否真的按下
        if (P26 == 0) // 检测按键电位为低，若这一步仍为0，则判定这次按下通过
        {
            KeyResetState = 1;    // 返回按键被按下的状态
            Key_Status_Check = 2; // 进入下一个状态：等待按键松开
        }
        else // 按键电位变高，说明仍有干扰，返回默认状态
        {
            Key_Status_Check = 0; // 检测状态重置为默认
        }
        break;

    case 2:                       // 等待按键松开
        if (P26 == 1)             // 按键电位为高，视为松开
            Key_Status_Check = 0; // 检测状态重置为默认
        break;
    default:
        Key_Status_Check = 0; // 其它情况均重置为默认
        break;
    }
}