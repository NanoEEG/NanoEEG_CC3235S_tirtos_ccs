/**
 * @file    bq25895.c
 * @author  gjmsilly
 * @brief   Driver for bq25895 (Based on CC3235S driverlib).
 * @version 0.0.1
 * @date    2021-12-6
 *
 * @copyright (c) 2021 gjmsilly
 *
 */

/********************************************************************************
 *  INCLUDES
 */
#include <stdbool.h>
#include <stddef.h>
#include <service/bq25895.h>

/* Driver configuration */
#include "ti_drivers_config.h"
#include <ti/devices/cc32xx/driverlib/i2c.h>

/*********************************************************************
 *  LOCAL FUNCTIONS
 */

/*!
    \brief  Sync_Init

    Read register value from BQ25895

    \param  bq25895handle       the handle of BQ25895
            regAddr             the register to be read
            regData             the register value to be returned

    \return true      succeed to read
            false     fail to read

*/
static bool BQ25895_ReadReg(I2C_Handle bq25895handle, uint8_t regAddr, uint8_t* regData)
{
    bool ret = true;

    I2C_HWAttrs const *hwAttrs = bq25895handle->hwAttrs;
    uint32_t I2C_BASE = hwAttrs->baseAddr;

    /* Write Slave Address to I2CMSA Transmit mode */
    I2CMasterSlaveAddrSet ( I2C_BASE,                \
                            BQ25895_Addr,            \
                            false);  ///< false - I2C write
    /* Write data to I2CMDR */
    I2CMasterDataPut(I2C_BASE, regAddr);

    /* start to transmit but stop condition is not generated */
    I2CMasterControl(I2C_BASE,I2C_MASTER_CMD_BURST_SEND_START);

    while(I2CMasterBusy(I2C_BASE) == true); // wait util Transmit is ok

    /* Write Slave Address to I2CMSA Receive mode */
    I2CMasterSlaveAddrSet ( I2C_BASE,                 \
                            BQ25895_Addr,             \
                            true);  ///< true - I2C read

    /* start to receive with NCK and stop conditon */
    I2CMasterControl(I2C_BASE,I2C_MASTER_CMD_SINGLE_RECEIVE);

    while(I2CMasterBusy(I2C_BASE) == true); ///< wait util data Received

    if ( I2CMasterErr(I2C_BASE) == I2C_MASTER_ERR_NONE ) ///< no error
    {
        /* get the reg data */
        *regData=I2CMasterDataGet(I2C_BASE);
        ret = true;
    }

    else
    {
        /* finish the transfer due to error */
        I2CMasterControl(I2C_BASE,I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
        ret = false;
    }


    return ret;

}

/*********************************************************************
 *  FUNCTIONS
 */

/*!
    \brief  Sync_Init

    Initializate the hardware for BQ25895

    \param  I2Chandle - the I2C handle for bq25895

    \return true - bq25895 driver handle is ready
            false -

*/

bool BQ25895_init(I2C_Handle I2Chandle)
{
    bool ret = true;

    if(I2Chandle == NULL){

        ///< One-time init of I2C driver
        I2C_init();

        ///< initialize optional I2C bus parameters
        I2C_Params      i2cParams;
        I2C_Params_init(&i2cParams);
        i2cParams.bitRate = I2C_400kHz;

        ///< Open I2C bus for usage
        I2C_Handle i2cHandle = I2C_open(COMMON_I2C, &i2cParams);
        if (i2cHandle == NULL) {

            ret = false;
        }
    }


    return ret;
}
