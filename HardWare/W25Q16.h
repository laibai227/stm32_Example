#ifndef __W25Q16_H
#define __W25Q16_H

#include "main.h"
#include "spi.h"
#include "gpio.h"
#include <stdint.h>
#include <stddef.h>

/* =========================================================
 * W25Q16 基本参数
 *用户可用区：0x000000 ~ 0x1FEFFF
 * ========================================================= */
#define W25Q16_PAGE_SIZE              256U
#define W25Q16_SECTOR_SIZE            4096U
#define W25Q16_FLASH_SIZE             2097152U

/* 最后一个 4KB 扇区作为内部交换区 */
#define W25Q16_SWAP_SECTOR_ADDR       (W25Q16_FLASH_SIZE - W25Q16_SECTOR_SIZE)

/* 用户实际可用容量，少最后一个扇区 */
#define W25Q16_USER_FLASH_SIZE        W25Q16_SWAP_SECTOR_ADDR

/* 用户可访问地址范围：0x000000 ~ 0x1FEFFF */
#define W25Q16_USER_ADDR_START        0x000000U
#define W25Q16_USER_ADDR_END          (W25Q16_USER_FLASH_SIZE - 1U)

/* W25Q16BV 正常 JEDEC ID */
#define W25Q16_JEDEC_ID               0xEF4015U

/* =========================================================
 * W25Q16 指令
 * ========================================================= */
#define W25Q16_CMD_WRITE_ENABLE       0x06
#define W25Q16_CMD_WRITE_DISABLE      0x04
#define W25Q16_CMD_READ_STATUS_REG1   0x05
#define W25Q16_CMD_READ_DATA          0x03
#define W25Q16_CMD_PAGE_PROGRAM       0x02
#define W25Q16_CMD_SECTOR_ERASE       0x20
#define W25Q16_CMD_JEDEC_ID           0x9F

/* =========================================================
 * 返回值类型
 * ========================================================= */
typedef enum
{
    W25Q16_OK = 0,
    W25Q16_ERROR,
    W25Q16_TIMEOUT
} W25Q16_StatusTypeDef;

/* =========================================================
 * 对外接口
 * ========================================================= */

/**
 * @brief  初始化 W25Q16，上电后调用一次
 */
void W25Q16_Init(void);

/**
 * @brief  读取芯片 JEDEC ID，验证 SPI 通信是否正常
 * @retval 3 字节 ID，W25Q16BV 正常应为 0xEF4015
 */
uint32_t W25Q16_Read_JEDEC_ID(void);

/**
 * @brief  从指定地址读取数据
 * @param  addr 起始地址，范围 0x000000 ~ 0x1FEFFF
 * @param  buf  读取缓冲区
 * @param  len  读取字节数
 *
 * @note   最后一个扇区 0x1FF000 ~ 0x1FFFFF 被驱动内部当交换区使用。
 */
void W25Q16_Read_Data(uint32_t addr, uint8_t *buf, uint32_t len);

/**
 * @brief  擦除指定地址所在的 4KB 扇区
 * @param  addr 扇区内任意地址，范围 0x000000 ~ 0x1FEFFF
 *
 * @note   擦除后该扇区全部变为 0xFF。
 * @note   不允许擦除最后一个交换扇区。
 */
W25Q16_StatusTypeDef W25Q16_Sector_Erase(uint32_t addr);

/**
 * @brief  页编程，最多 256 字节，不允许跨页
 * @param  addr 写入地址，范围 0x000000 ~ 0x1FEFFF
 * @param  buf  待写入数据
 * @param  len  写入长度，最大 256 字节
 *
 * @note   这个函数不会自动擦除。
 * @note   如果要安全写入，推荐直接用 W25Q16_Write_Data。
 */
W25Q16_StatusTypeDef W25Q16_Page_Program(uint32_t addr, const uint8_t *buf, uint16_t len);

/**
 * @brief  任意起始地址写入数据，推荐使用
 * @param  addr 起始地址，范围 0x000000 ~ 0x1FEFFF
 * @param  buf  待写入数据
 * @param  len  写入字节数
 *
 * @retval W25Q16_OK      写入成功
 * @retval W25Q16_ERROR   参数错误或地址越界
 * @retval W25Q16_TIMEOUT 等待 Flash 忙状态超时
 *
 * @note   这个函数内部会自动处理：
 *         1. 任意起始地址
 *         2. 自动跨页
 *         3. 自动跨扇区
 *         4. 自动判断是否需要擦除
 *         5. 保留同一扇区内其他数据
 *         6. 使用最后一个扇区作为交换区
 *
 * @note   Flash 擦除后数据是 0xFF。
 * @note   Flash 写入只能把 1 写成 0，不能直接把 0 写回 1。
 */
W25Q16_StatusTypeDef W25Q16_Write_Data(uint32_t addr, const uint8_t *buf, uint32_t len);

#endif /* __W25Q16_H */
