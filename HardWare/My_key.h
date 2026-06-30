#ifndef __MY_KEY_H
#define __MY_KEY_H

#include "main.h"

/* 按键引脚 */
#define KEY1_PIN    GPIO_PIN_12
#define KEY2_PIN    GPIO_PIN_14

/* 按键电平定义：上拉输入，按下为低电平 */
#define KEY_DOWN_LEVEL    GPIO_PIN_RESET
#define KEY_UP_LEVEL      GPIO_PIN_SET

/* 时间阈值，单位 ms */
#define KEY_DEBOUNCE_TIME     20      // 消抖时间
#define KEY_PRESS_TIME        500     // 小于这个时间，算单击
#define KEY_LONG_TIME         1500    // 超过这个时间，算长按
#define KEY_DOUBLE_GAP        300     // 两次点击间隔小于这个时间，算双击

/* 最多支持几个按键 */
#define KEY_MAX_NUM           4

typedef enum
{
    KEY_NONE = 0,      // 无按键事件
    KEY_PRESS,         // 单击，快速按下松开
    KEY_SHORT,         // 短按，按下一段时间后松开
    KEY_LONG,          // 长按，按住超过长按时间，触发一次
    KEY_DOUBLE         // 双击
} KeyType;



KeyType Key_Scan(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

#endif /* __MY_KEY_H */
