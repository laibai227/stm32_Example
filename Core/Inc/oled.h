/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    oled.h
 * @brief   OLED display driver for 0.96 inch SSD1306 (4-pin I2C, software I2C)
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026
 * All rights reserved.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>

#define OLED_8X16       8
#define OLED_6X8        6
#define OLED_UNFILLED   0
#define OLED_FILLED     1

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief  初始化OLED，上电后调用一次
 * @param  无
 * @note   会自动清屏，之后就可以用 Show/Draw 函数往缓冲区写内容
 * @usage  OLED_Init();
 */
void OLED_Init(void);

/**
 * @brief  清屏（清空缓冲区并刷新到屏幕）
 * @param  无
 * @note   调用后屏幕变空白，内部已包含 Refresh_Gram
 * @usage  OLED_Clear();
 */
void OLED_Clear(void);

/**
 * @brief  把缓冲区的数据刷新到OLED屏幕
 * @param  无
 * @note   所有 Show/Draw/Fill 操作只写缓冲区，必须调这个才能显示
 * @usage  OLED_Show_String(0, 0, "Hello", 8);
 *         OLED_Refresh_Gram();  // 这时屏幕才显示
 */
void OLED_Refresh_Gram(void);

/**
 * @brief  开启OLED显示
 * @param  无
 * @note   一般不需要手动调用，OLED_Init 已经开启了
 * @usage  OLED_Display_On();
 */
void OLED_Display_On(void);

/**
 * @brief  关闭OLED显示（省电，屏幕变黑）
 * @param  无
 * @note   不会清空缓冲区，重新开显示后内容还在
 * @usage  OLED_Display_Off();
 */
void OLED_Display_Off(void);

/**
 * @brief  全屏填充同一个值
 * @param  fill_Data: 填充值，0x00=全灭，0xFF=全亮
 * @note   内部已包含 Refresh_Gram
 * @usage  OLED_Fill(0xFF);  // 全屏点亮
 */
void OLED_Fill(uint8_t fill_Data);

/**
 * @brief  画一个像素点（写入缓冲区）
 * @param  x:    横坐标，0~127（0=最左边）
 * @param  y:    纵坐标，0~63（0=最上边）
 * @param  bit:  1=点亮，0=熄灭
 * @note   只写缓冲区，需要调 OLED_Refresh_Gram() 才显示
 * @usage  OLED_Draw_Point(64, 32, 1);  // 在屏幕中心画一个点
 *         OLED_Refresh_Gram();
 */
void OLED_Draw_Point(uint8_t x, uint8_t y, uint8_t bit);

/**
 * @brief  在指定位置显示字符串（写入缓冲区）
 * @param  x:    起始横坐标，0~127
 * @param  y:    起始纵坐标（像素行），0~63
 * @param  str:  要显示的字符串，如 "Hello"
 * @param  size: 字体大小，8=6x8小字体，16=8x16大字体
 * @note   只写缓冲区，需要调 OLED_Refresh_Gram() 才显示
 * @usage  OLED_Show_String(0, 0, "Hello", 8);    // 左上角，小字体
 *         OLED_Show_String(0, 16, "World", 16);   // 往下移16像素，大字体
 *         OLED_Refresh_Gram();
 */
void OLED_Show_String(uint8_t x, uint8_t y, char *str, uint8_t size);

/**
 * @brief  向SSD1306发送命令（底层函数，一般不直接用）
 * @param  cmd: 命令字节
 * @usage  OLED_Write_Command(0xAE);  // 关显示
 */
void OLED_Write_Command(uint8_t cmd);

/**
 * @brief  向SSD1306发送数据（底层函数，一般不直接用）
 * @param  data: 数据缓冲区指针
 * @param  len:  数据长度
 * @usage  OLED_Write_Data(buf, 128);
 */
void OLED_Write_Data(uint8_t *data, uint8_t len);

/**
 * @brief  设置光标位置（底层函数，一般不直接用）
 * @param  page: 页地址，0~7（每页8像素高，共64像素）
 * @param  col:  列地址，0~127
 * @usage  OLED_Set_Pos(0, 0);  // 定位到左上角
 */
void OLED_Set_Pos(uint8_t page, uint8_t col);

void OLED_ClearArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void OLED_Reverse(void);
void OLED_ReverseArea(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size);
void OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowFloatNum(uint8_t x, uint8_t y, double num, uint8_t intLen, uint8_t fraLen, uint8_t size);
void OLED_Printf(uint8_t x, uint8_t y, uint8_t size, char *format, ...);
void OLED_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void OLED_DrawRectangle(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t filled);
void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t radius, uint8_t filled);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H__ */
