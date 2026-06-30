# LED 学习项目

基于 STM32F103C8T6 的嵌入式学习项目，使用 FreeRTOS 操作系统。

## 硬件配置

- **芯片**: STM32F103C8T6 (Blue Pill)
- **OLED**: 0.96寸 SSD1306 I2C 接口 128x64
- **Flash**: W25Q16 SPI 接口 2MB
- **按键**: PB12/PB14 外部中断模式

## 软件架构

- **操作系统**: FreeRTOS
- **驱动**: HAL 库
- **调试**: USART1 (115200 8N1)

## 项目结构

```
LED/
├── Core/Inc/          # 头文件
│   ├── main.h
│   ├── FreeRTOSConfig.h
│   ├── gpio.h / i2c.h / spi.h / usart.h
│   └── oled.h / My_key.h / W25Q16.h
├── Core/Src/           # 源文件
│   ├── freertos.c      # FreeRTOS 任务定义
│   ├── main.c           # 入口
│   ├── gpio.c / i2c.c / spi.c / usart.c
│   ├── oled.c           # OLED 显示驱动
│   ├── My_key.c         # 多功能按键驱动（单击/短按/长按/双击）
│   └── W25Q16.c         # Flash 存储驱动
├── HardWare/           # 手动编写的外设驱动
│   ├── oled.c / oled.h
│   ├── My_key.c / My_key.h
│   └── W25Q16.c / W25Q16.h
└── MDK-ARM/
    └── LED.uvprojx      # Keil 项目文件
```

## 功能说明

### 按键驱动 (My_key.c)

支持多种按键模式，调用 `Key_Scan()` 周期检测（建议 10ms 调用一次）：

| 返回值 | 含义 |
|--------|------|
| `KEY_PRESS` | 单击，快速按下松开 |
| `KEY_SHORT` | 短按，按下一段时间后松开 |
| `KEY_LONG` | 长按，按住超过 1 秒，触发一次 |
| `KEY_DOUBLE` | 双击，两次按下间隔 < 300ms |
| `KEY_NONE` | 无按键 |

使用示例：
```c
KeyType key = Key_Scan(GPIOB, KEY1_PIN);
if (key != KEY_NONE) {
    switch (key) {
        case KEY_PRESS:  /* 处理单击 */  break;
        case KEY_SHORT:   /* 处理短按 */  break;
        case KEY_LONG:    /* 处理长按 */  break;
        case KEY_DOUBLE:  /* 处理双击 */ break;
    }
}
```

### OLED 驱动 (oled.c)

| 函数 | 说明 |
|------|------|
| `OLED_Init()` | 初始化，上电调用一次 |
| `OLED_Show_String(x, y, str, size)` | 显示字符串，size=8(6x8)或16(8x16) |
| `OLED_Printf(x, y, size, fmt, ...)` | 格式化显示，类似 printf |
| `OLED_ShowNum(x, y, num, len, size)` | 显示数字 |
| `OLED_ShowHexNum(x, y, num, len, size)` | 显示十六进制 |
| `OLED_DrawPoint(x, y, bit)` | 画点 |
| `OLED_DrawLine / Rectangle / Circle` | 画图形 |
| `OLED_Refresh_Gram()` | 刷新到屏幕（必须调用） |
| `OLED_Clear()` | 清屏 |

### W25Q16 Flash 驱动 (W25Q16.c)

| 函数 | 说明 |
|------|------|
| `W25Q16_Init()` | 初始化 |
| `W25Q16_Read_JEDEC_ID()` | 读取芯片 ID，返回 3 字节如 0xEF4015 |
| `W25Q16_Read_Data(addr, buf, len)` | 读取数据 |
| `W25Q16_Write_Data(addr, buf, len)` | 写入数据（自动跨页/跨扇区） |
| `W25Q16_Sector_Erase(addr)` | 擦除 4KB 扇区 |

注意：写入前必须先擦除。

## 编译

使用 Keil MDK 打开 `MDK-ARM/LED.uvprojx`，编译下载。

## 开发环境

- Keil MDK 5
- STM32CubeMX 生成配置
- FreeRTOS 10
- STM32F1 HAL 库
