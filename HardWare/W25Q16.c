#include "W25Q16.h"
#include <string.h>

/* 只占用 256 字节 RAM */
static uint8_t W25Q16_PageBuf[W25Q16_PAGE_SIZE];

/**
 * @brief  拉低 CS，选中 Flash
 */
static void W25Q16_CS_Low(void)
{
    HAL_GPIO_WritePin(Flash_SPI_CS_GPIO_Port, Flash_SPI_CS_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  拉高 CS，取消选中 Flash
 */
static void W25Q16_CS_High(void)
{
    HAL_GPIO_WritePin(Flash_SPI_CS_GPIO_Port, Flash_SPI_CS_Pin, GPIO_PIN_SET);
}

/**
 * @brief  SPI 发送 1 字节，同时接收 1 字节
 */
static uint8_t W25Q16_SPI_ReadWriteByte(uint8_t txData)
{
    uint8_t rxData = 0x00;

    HAL_SPI_TransmitReceive(&hspi1, &txData, &rxData, 1, 100);

    return rxData;
}

/**
 * @brief  初始化 W25Q16
 */
void W25Q16_Init(void)
{
    W25Q16_CS_High();
    HAL_Delay(10);
}

/**
 * @brief  读取 JEDEC ID
 */
uint32_t W25Q16_Read_JEDEC_ID(void)
{
    uint32_t jedecId = 0;

    W25Q16_CS_High();
    HAL_Delay(1);

    W25Q16_CS_Low();

    /* 发送 0x9F，发送这个字节时收到的是无效数据 */
    W25Q16_SPI_ReadWriteByte(W25Q16_CMD_JEDEC_ID);

    /* 后面 3 个字节才是有效 ID */
    jedecId  = ((uint32_t)W25Q16_SPI_ReadWriteByte(0xFF)) << 16;
    jedecId |= ((uint32_t)W25Q16_SPI_ReadWriteByte(0xFF)) << 8;
    jedecId |=  (uint32_t)W25Q16_SPI_ReadWriteByte(0xFF);

    W25Q16_CS_High();

    return jedecId;
}

/**
 * @brief  读取状态寄存器 1
 * @note   bit0 = BUSY，1 表示忙，0 表示空闲
 */
static uint8_t W25Q16_Read_StatusReg1(void)
{
    uint8_t status = 0x00;

    W25Q16_CS_Low();

    W25Q16_SPI_ReadWriteByte(W25Q16_CMD_READ_STATUS_REG1);
    status = W25Q16_SPI_ReadWriteByte(0xFF);

    W25Q16_CS_High();

    return status;
}

/**
 * @brief  写使能
 */
static void W25Q16_Write_Enable(void)
{
    W25Q16_CS_Low();

    W25Q16_SPI_ReadWriteByte(W25Q16_CMD_WRITE_ENABLE);

    W25Q16_CS_High();
}


/**
 * @brief  等待 Flash 空闲
 */
static W25Q16_StatusTypeDef W25Q16_Wait_Busy(uint32_t timeout_ms)
{
    uint32_t startTick = HAL_GetTick();

    while((W25Q16_Read_StatusReg1() & 0x01) == 0x01)
    {
        if((HAL_GetTick() - startTick) >= timeout_ms)
        {
            return W25Q16_TIMEOUT;
        }

        HAL_Delay(1);
    }

    return W25Q16_OK;
}

/**
 * @brief  内部原始读取函数
 * @note   可以读取整个 Flash，包括最后一个交换扇区
 */
static void W25Q16_Read_Data_Raw(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t i;

    if(buf == NULL || len == 0)
    {
        return;
    }

    if(addr >= W25Q16_FLASH_SIZE)
    {
        return;
    }

    if(len > (W25Q16_FLASH_SIZE - addr))
    {
        len = W25Q16_FLASH_SIZE - addr;
    }

    W25Q16_CS_Low();

    W25Q16_SPI_ReadWriteByte(W25Q16_CMD_READ_DATA);

    W25Q16_SPI_ReadWriteByte((addr >> 16) & 0xFF);
    W25Q16_SPI_ReadWriteByte((addr >> 8) & 0xFF);
    W25Q16_SPI_ReadWriteByte(addr & 0xFF);

    for(i = 0; i < len; i++)
    {
        buf[i] = W25Q16_SPI_ReadWriteByte(0xFF);
    }

    W25Q16_CS_High();
}

/**
 * @brief  对外读取函数
 * @note   不允许读取最后一个交换扇区
 */
void W25Q16_Read_Data(uint32_t addr, uint8_t *buf, uint32_t len)
{
    if(buf == NULL || len == 0)
    {
        return;
    }

    if(addr >= W25Q16_USER_FLASH_SIZE)
    {
        return;
    }

    if(len > (W25Q16_USER_FLASH_SIZE - addr))
    {
        len = W25Q16_USER_FLASH_SIZE - addr;
    }

    W25Q16_Read_Data_Raw(addr, buf, len);
}

/**
 * @brief  内部原始扇区擦除
 * @note   可以擦除整个 Flash，包括最后一个交换扇区
 */
static W25Q16_StatusTypeDef W25Q16_Sector_Erase_Raw(uint32_t addr)
{
    addr = addr & ~(W25Q16_SECTOR_SIZE - 1U);

    if(addr >= W25Q16_FLASH_SIZE)
    {
        return W25Q16_ERROR;
    }

    W25Q16_Write_Enable();

    W25Q16_CS_Low();

    W25Q16_SPI_ReadWriteByte(W25Q16_CMD_SECTOR_ERASE);

    W25Q16_SPI_ReadWriteByte((addr >> 16) & 0xFF);
    W25Q16_SPI_ReadWriteByte((addr >> 8) & 0xFF);
    W25Q16_SPI_ReadWriteByte(addr & 0xFF);

    W25Q16_CS_High();

    return W25Q16_Wait_Busy(1000);
}

/**
 * @brief  对外扇区擦除
 * @note   不允许擦除最后一个交换扇区
 */
W25Q16_StatusTypeDef W25Q16_Sector_Erase(uint32_t addr)
{
    if(addr >= W25Q16_USER_FLASH_SIZE)
    {
        return W25Q16_ERROR;
    }

    return W25Q16_Sector_Erase_Raw(addr);
}

/**
 * @brief  内部原始页编程
 * @note   可以写整个 Flash，包括最后一个交换扇区
 * @note   不允许跨页
 */
static W25Q16_StatusTypeDef W25Q16_Page_Program_Raw(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    uint16_t i;
    uint16_t pageRemain;

    if(buf == NULL || len == 0)
    {
        return W25Q16_ERROR;
    }

    if(addr >= W25Q16_FLASH_SIZE)
    {
        return W25Q16_ERROR;
    }

    if(len > (W25Q16_FLASH_SIZE - addr))
    {
        return W25Q16_ERROR;
    }

    pageRemain = W25Q16_PAGE_SIZE - (addr % W25Q16_PAGE_SIZE);

    if(len > pageRemain)
    {
        return W25Q16_ERROR;
    }

    W25Q16_Write_Enable();

    W25Q16_CS_Low();

    W25Q16_SPI_ReadWriteByte(W25Q16_CMD_PAGE_PROGRAM);

    W25Q16_SPI_ReadWriteByte((addr >> 16) & 0xFF);
    W25Q16_SPI_ReadWriteByte((addr >> 8) & 0xFF);
    W25Q16_SPI_ReadWriteByte(addr & 0xFF);

    for(i = 0; i < len; i++)
    {
        W25Q16_SPI_ReadWriteByte(buf[i]);
    }

    W25Q16_CS_High();

    return W25Q16_Wait_Busy(500);
}

/**
 * @brief  对外页编程
 * @note   不允许写最后一个交换扇区
 * @note   不自动擦除
 */
W25Q16_StatusTypeDef W25Q16_Page_Program(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    if(addr >= W25Q16_USER_FLASH_SIZE)
    {
        return W25Q16_ERROR;
    }

    if(len > (W25Q16_USER_FLASH_SIZE - addr))
    {
        return W25Q16_ERROR;
    }

    return W25Q16_Page_Program_Raw(addr, buf, len);
}

/**
 * @brief  内部不擦除连续写入
 * @note   只负责自动跨页
 * @note   要求目标区域已经擦除过，或者只发生 1 -> 0
 */
static W25Q16_StatusTypeDef W25Q16_Write_NoErase_Raw(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t pageRemain;
    uint32_t writeLen;
    W25Q16_StatusTypeDef ret;

    if(buf == NULL || len == 0)
    {
        return W25Q16_ERROR;
    }

    if(addr >= W25Q16_FLASH_SIZE)
    {
        return W25Q16_ERROR;
    }

    if(len > (W25Q16_FLASH_SIZE - addr))
    {
        return W25Q16_ERROR;
    }

    while(len > 0)
    {
        pageRemain = W25Q16_PAGE_SIZE - (addr % W25Q16_PAGE_SIZE);

        if(len < pageRemain)
        {
            writeLen = len;
        }
        else
        {
            writeLen = pageRemain;
        }

        ret = W25Q16_Page_Program_Raw(addr, buf, (uint16_t)writeLen);

        if(ret != W25Q16_OK)
        {
            return ret;
        }

        addr += writeLen;
        buf += writeLen;
        len -= writeLen;
    }

    return W25Q16_OK;
}

/**
 * @brief  任意起始地址写入数据
 *
 * 功能：
 * 1. 任意起始地址
 * 2. 任意长度
 * 3. 自动跨页
 * 4. 自动跨扇区
 * 5. 自动判断是否需要擦除
 * 6. 保留同一扇区内其他数据
 * 7. 最后一个扇区作为交换区
 */
W25Q16_StatusTypeDef W25Q16_Write_Data(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t sectorAddr;
    uint32_t sectorOffset;
    uint32_t currentWriteLen;

    uint32_t checkAddr;
    uint32_t checkRemain;
    uint32_t checkLen;
    uint32_t checkIndex;

    uint32_t pageOffset;
    uint32_t modifyStart;
    uint32_t modifyEnd;
    uint32_t copyLen;

    uint32_t i;
    uint8_t needErase;
    W25Q16_StatusTypeDef ret;

    if(buf == NULL || len == 0)
    {
        return W25Q16_ERROR;
    }

    /*
     * 用户可写范围少最后一个扇区
     * 最大只能写到 0x1FEFFF
     */
    if(addr >= W25Q16_USER_FLASH_SIZE)
    {
        return W25Q16_ERROR;
    }

    if(len > (W25Q16_USER_FLASH_SIZE - addr))
    {
        return W25Q16_ERROR;
    }

    while(len > 0)
    {
        /*
         * 当前写入地址所在的 4KB 扇区
         */
        sectorAddr = addr & ~(W25Q16_SECTOR_SIZE - 1U);
        sectorOffset = addr - sectorAddr;

        /*
         * 本轮只处理当前扇区内的数据
         */
        if(len < (W25Q16_SECTOR_SIZE - sectorOffset))
        {
            currentWriteLen = len;
        }
        else
        {
            currentWriteLen = W25Q16_SECTOR_SIZE - sectorOffset;
        }

        /*
         * 判断是否需要擦除
         *
         * Flash 可以 1 -> 0
         * 不能 0 -> 1
         *
         * 如果 old & new != new
         * 说明存在 0 要变 1，必须擦除
         */
        needErase = 0;
        checkAddr = addr;
        checkRemain = currentWriteLen;
        checkIndex = 0;

        while(checkRemain > 0)
        {
            if(checkRemain < W25Q16_PAGE_SIZE)
            {
                checkLen = checkRemain;
            }
            else
            {
                checkLen = W25Q16_PAGE_SIZE;
            }

            W25Q16_Read_Data_Raw(checkAddr, W25Q16_PageBuf, checkLen);

            for(i = 0; i < checkLen; i++)
            {
                if((W25Q16_PageBuf[i] & buf[checkIndex + i]) != buf[checkIndex + i])
                {
                    needErase = 1;
                    break;
                }
            }

            if(needErase)
            {
                break;
            }

            checkAddr += checkLen;
            checkIndex += checkLen;
            checkRemain -= checkLen;
        }

        if(needErase == 0)
        {
            /*
             * 不需要擦除，直接跨页写入
             */
            ret = W25Q16_Write_NoErase_Raw(addr, buf, currentWriteLen);

            if(ret != W25Q16_OK)
            {
                return ret;
            }
        }
        else
        {
            /*
             * 需要擦除时，使用最后一个扇区作为交换区
             */

            /*
             * 1. 擦除交换扇区
             */
            ret = W25Q16_Sector_Erase_Raw(W25Q16_SWAP_SECTOR_ADDR);

            if(ret != W25Q16_OK)
            {
                return ret;
            }

            /*
             * 2. 把原扇区逐页复制到交换扇区
             *    复制过程中修改目标数据
             */
            for(pageOffset = 0; pageOffset < W25Q16_SECTOR_SIZE; pageOffset += W25Q16_PAGE_SIZE)
            {
                W25Q16_Read_Data_Raw(sectorAddr + pageOffset,
                                     W25Q16_PageBuf,
                                     W25Q16_PAGE_SIZE);

                /*
                 * 当前页范围：
                 * pageOffset ~ pageOffset + 255
                 *
                 * 本轮修改范围：
                 * sectorOffset ~ sectorOffset + currentWriteLen - 1
                 */
                if(((pageOffset + W25Q16_PAGE_SIZE) > sectorOffset) &&
                   (pageOffset < (sectorOffset + currentWriteLen)))
                {
                    if(pageOffset > sectorOffset)
                    {
                        modifyStart = pageOffset;
                    }
                    else
                    {
                        modifyStart = sectorOffset;
                    }

                    if((pageOffset + W25Q16_PAGE_SIZE) < (sectorOffset + currentWriteLen))
                    {
                        modifyEnd = pageOffset + W25Q16_PAGE_SIZE;
                    }
                    else
                    {
                        modifyEnd = sectorOffset + currentWriteLen;
                    }

                    copyLen = modifyEnd - modifyStart;

                    memcpy(&W25Q16_PageBuf[modifyStart - pageOffset],
                           &buf[modifyStart - sectorOffset],
                           copyLen);
                }

                ret = W25Q16_Page_Program_Raw(W25Q16_SWAP_SECTOR_ADDR + pageOffset,
                                              W25Q16_PageBuf,
                                              W25Q16_PAGE_SIZE);

                if(ret != W25Q16_OK)
                {
                    return ret;
                }
            }

            /*
             * 3. 擦除原扇区
             */
            ret = W25Q16_Sector_Erase_Raw(sectorAddr);

            if(ret != W25Q16_OK)
            {
                return ret;
            }

            /*
             * 4. 把交换扇区逐页写回原扇区
             */
            for(pageOffset = 0; pageOffset < W25Q16_SECTOR_SIZE; pageOffset += W25Q16_PAGE_SIZE)
            {
                W25Q16_Read_Data_Raw(W25Q16_SWAP_SECTOR_ADDR + pageOffset,
                                     W25Q16_PageBuf,
                                     W25Q16_PAGE_SIZE);

                ret = W25Q16_Page_Program_Raw(sectorAddr + pageOffset,
                                              W25Q16_PageBuf,
                                              W25Q16_PAGE_SIZE);

                if(ret != W25Q16_OK)
                {
                    return ret;
                }
            }
        }

        /*
         * 进入下一个扇区
         */
        addr += currentWriteLen;
        buf += currentWriteLen;
        len -= currentWriteLen;
    }

    return W25Q16_OK;
}

