/**
 * @file    hw_bq27441-g1.c
 * @author  gjmsilly
 * @brief   hardware Init for bq27441-g1 (based on CC3235S driverlib)
 * @version 0.0.1
 * @date    2022-3-3
 *
 * @copyright (c) 2022 gjmsilly
 *
 */
/********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

/* TI-DRIVERS Header files */
#include "ti_drivers_config.h"
#include <ti/devices/cc32xx/driverlib/i2c.h>
#include <ti/devices/cc32xx/inc/hw_i2c.h>
#include <ti/devices/cc32xx/driverlib/utils.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Timer.h>
#include <ti/display/Display.h>

/*********************************************************************
 * Callback
 */
void I2CTransfer(uint16_t deviceAddr, uint8_t cmdArray[], uint8_t dataArray[], uint16_t cmdLen, uint16_t dataLen, uint16_t flag)
{
    I2C_TransferSeq_TypeDef i2cTransfer; // transfer structure

    I2C_TransferReturn_TypeDef result;  //transfer return enum

    i2cTransfer.addr        = deviceAddr;
    i2cTransfer.flags       = flag;
    i2cTransfer.buf[0].data = cmdArray;
    i2cTransfer.buf[0].len  = cmdLen;

    i2cTransfer.buf[1].data = dataArray;
    i2cTransfer.buf[1].len  = dataLen;

    result = I2C_TransferInit(I2C1, &i2cTransfer);

    while (result != i2cTransferDone)
    {
        if (result != i2cTransferInProgress)
        {
            DEBUG_BREAK;
        }
        result = I2C_Transfer(I2C1);    // continue an initiated I2C transfer
    }
}

uint16_t i2cReadRegister(uint16_t addr,uint8_t regOffset)
{
    uint16_t result = 0x00;

    uint8_t cmdArray[1];
    uint8_t dataArray[2];

    cmdArray[0] = regOffset;
    I2CTransfer(addr << 1, cmdArray, dataArray, 1, 2, I2C_FLAG_WRITE_READ);

    result = (dataArray[1] << 8) | (dataArray[0]);
    return result;
    return dataArray[0];
}


void i2cWriteRegister(uint16_t addr,uint8_t regOffset, uint8_t writeData)
{
    uint8_t cmdArray[1];
    uint8_t dataArray[2];

    cmdArray[0] = regOffset;
    dataArray[0] = writeData;
    I2CTransfer(addr << 1, cmdArray, dataArray, 1, 2, I2C_FLAG_WRITE_WRITE);
}

/*********************************************************************
 *  FUNCTIONS
 */


