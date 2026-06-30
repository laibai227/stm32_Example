#include "My_key.h"

typedef enum
{
    KEY_STATE_IDLE = 0,              // 空闲
    KEY_STATE_DEBOUNCE_PRESS,        // 按下消抖
    KEY_STATE_PRESSED,               // 已确认按下
    KEY_STATE_HELD,                  // 长按已触发，等待松手
    KEY_STATE_DEBOUNCE_RELEASE,      // 松手消抖
    KEY_STATE_WAIT_DOUBLE,           // 等待第二次按下
    KEY_STATE_SECOND_DEBOUNCE_PRESS, // 第二次按下消抖
    KEY_STATE_SECOND_PRESSED,        // 第二次已确认按下
    KEY_STATE_SECOND_RELEASE         // 第二次等待松手
} KeyState_t;

/**
 * @brief 按键控制结构体
 * @note  每个按键对应一个实例，存放该按键的引脚、状态、时间戳等信息
 */
typedef struct
{
    GPIO_TypeDef *GPIOx;      // 按键所在的GPIO端口，如GPIOB
    uint16_t GPIO_Pin;         // 按键对应的引脚号，如GPIO_PIN_12

    uint8_t used;              // 该结构体是否已被占用（0=空闲，1=已用）
    KeyState_t state;          // 当前状态机状态（空闲/消抖/已按下/长按/等待双击等）

    uint32_t press_tick;        // 本次按下的时刻（毫秒tick），用于计算按下持续时间
    uint32_t release_tick;     // 本次松开的时刻（毫秒tick），用于计算松开间隔、判断双击窗口
    uint32_t first_press_time; // 第一次按下持续的时间（毫秒），用于区分KEY_PRESS和KEY_SHORT
} KeyControl_t;

static KeyControl_t key_ctrl[KEY_MAX_NUM];

/**
 * @brief  获取当前按键对应的状态控制块
 */
static KeyControl_t *Key_GetControl(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    uint8_t i;

    // 如果按键已经注册过，直接返回对应的控制块
    for(i = 0; i < KEY_MAX_NUM; i++)
    {
        if(key_ctrl[i].used)
        {
            if(key_ctrl[i].GPIOx == GPIOx && key_ctrl[i].GPIO_Pin == GPIO_Pin)
            {
                return &key_ctrl[i];
            }
        }
    }

    // 如果按键没有注册过，找一个空闲的控制块进行注册
    for(i = 0; i < KEY_MAX_NUM; i++)
    {
        if(key_ctrl[i].used == 0)
        {
            key_ctrl[i].used = 1;
            key_ctrl[i].GPIOx = GPIOx;
            key_ctrl[i].GPIO_Pin = GPIO_Pin;
            key_ctrl[i].state = KEY_STATE_IDLE;
            key_ctrl[i].press_tick = 0;
            key_ctrl[i].release_tick = 0;
            key_ctrl[i].first_press_time = 0;

            return &key_ctrl[i];
        }
    }

    return 0;
}

/**
 * @brief  判断按键是否按下
 */
static uint8_t Key_IsDown(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    if(HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == KEY_DOWN_LEVEL)
    {
        return 1;
    }

    return 0;
}

/**
 * @brief  多功能按键扫描
 * @note   需要周期调用，比如 10ms 调用一次
 */
KeyType Key_Scan(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    KeyControl_t *key;
    uint32_t now_tick;
    uint32_t press_time;

    key = Key_GetControl(GPIOx, GPIO_Pin);

    if(key == 0)
    {
        return KEY_NONE;
    }

    now_tick = HAL_GetTick();

    switch(key->state)
    {
        case KEY_STATE_IDLE:
        {
            if(Key_IsDown(GPIOx, GPIO_Pin))
            {
                key->press_tick = now_tick;
                key->state = KEY_STATE_DEBOUNCE_PRESS;
            }
            break;
        }

        case KEY_STATE_DEBOUNCE_PRESS:
        {
            if((now_tick - key->press_tick) >= KEY_DEBOUNCE_TIME)
            {
                if(Key_IsDown(GPIOx, GPIO_Pin))
                {
                    key->press_tick = now_tick;
                    key->state = KEY_STATE_PRESSED;
                }
                else
                {
                    key->state = KEY_STATE_IDLE;
                }
            }
            break;
        }

        case KEY_STATE_PRESSED:
        {
            if(Key_IsDown(GPIOx, GPIO_Pin))
            {
                if((now_tick - key->press_tick) >= KEY_LONG_TIME)
                {
                    key->state = KEY_STATE_HELD;
                    return KEY_LONG;
                }
            }
            else
            {
                key->release_tick = now_tick;
                key->state = KEY_STATE_DEBOUNCE_RELEASE;
            }
            break;
        }

        case KEY_STATE_DEBOUNCE_RELEASE:
        {
            if((now_tick - key->release_tick) >= KEY_DEBOUNCE_TIME)
            {
                if(Key_IsDown(GPIOx, GPIO_Pin) == 0)
                {
                    press_time = key->release_tick - key->press_tick;

                    if(press_time < KEY_DEBOUNCE_TIME)
                    {
                        key->state = KEY_STATE_IDLE;
                    }
                    else
                    {
                        key->first_press_time = press_time;
                        key->state = KEY_STATE_WAIT_DOUBLE;
                    }
                }
                else
                {
                    key->state = KEY_STATE_PRESSED;
                }
            }
            break;
        }

        case KEY_STATE_WAIT_DOUBLE:
        {
            if(Key_IsDown(GPIOx, GPIO_Pin))
            {
                if((now_tick - key->release_tick) <= KEY_DOUBLE_GAP)
                {
                    key->press_tick = now_tick;
                    key->state = KEY_STATE_SECOND_DEBOUNCE_PRESS;
                }
                else
                {
                    key->press_tick = now_tick;
                    key->state = KEY_STATE_DEBOUNCE_PRESS;
                }
            }
            else
            {
                if((now_tick - key->release_tick) >= KEY_DOUBLE_GAP)
                {
                    press_time = key->first_press_time;
                    key->state = KEY_STATE_IDLE;

                    if(press_time < KEY_PRESS_TIME)
                    {
                        return KEY_PRESS;
                    }
                    else
                    {
                        return KEY_SHORT;
                    }
                }
            }
            break;
        }

        case KEY_STATE_SECOND_DEBOUNCE_PRESS:
        {
            if((now_tick - key->press_tick) >= KEY_DEBOUNCE_TIME)
            {
                if(Key_IsDown(GPIOx, GPIO_Pin))
                {
                    key->state = KEY_STATE_SECOND_PRESSED;
                }
                else
                {
                    key->state = KEY_STATE_WAIT_DOUBLE;
                }
            }
            break;
        }

        case KEY_STATE_SECOND_PRESSED:
        {
            if(Key_IsDown(GPIOx, GPIO_Pin) == 0)
            {
                key->release_tick = now_tick;
                key->state = KEY_STATE_SECOND_RELEASE;
            }
            break;
        }

        case KEY_STATE_SECOND_RELEASE:
        {
            if((now_tick - key->release_tick) >= KEY_DEBOUNCE_TIME)
            {
                if(Key_IsDown(GPIOx, GPIO_Pin) == 0)
                {
                    key->state = KEY_STATE_IDLE;
                    return KEY_DOUBLE;
                }
                else
                {
                    key->state = KEY_STATE_SECOND_PRESSED;
                }
            }
            break;
        }

        case KEY_STATE_HELD:
        {
            /*
             * 长按已经返回过一次 KEY_LONG
             * 后面一直按住不再重复返回
             * 松手后回到空闲
             */
            if(Key_IsDown(GPIOx, GPIO_Pin) == 0)
            {
                key->state = KEY_STATE_IDLE;
            }
            break;
        }

        default:
        {
            key->state = KEY_STATE_IDLE;
            break;
        }
    }

    return KEY_NONE;
}
