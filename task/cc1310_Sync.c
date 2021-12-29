/**
 * @file    cc1310_Sync.c
 * @author  gjmsilly
 * @brief   System Timer of CC3235S synchronize with CC1310 RAT
 * @version 1.0.0
 * @date    2021-12-24
 *
 * @copyright (c) 2021 gjmsilly
 *
 */

/********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* TI-DRIVERS Header files */
#include "ti_drivers_config.h"

#include <ti/devices/cc32xx/driverlib/i2c.h>
#include <ti/devices/cc32xx/inc/hw_i2c.h>

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Timer.h>
#include <ti/display/Display.h>

/* POSIX Header files */
#include <semaphore.h>

// Define the slave address of device on the SENSORS bus
#define CC1310_ADDR     0x33

/*********************************************************************
 *  GLOBAL VARIABLES
 */

static uint32_t         Tror;
static uint32_t         Tsor;
static uint32_t         Troc;
static uint8_t          type;
static uint32_t         delay;
static uint8_t          I2C_BUFF[16];

Timer_Handle pSyncTime = NULL;      //!< 同步时钟

/*********************************************************************
 *  EXTERNAL VARIABLES
 */
extern sem_t EvtDataRecv;
extern I2C_Handle i2cHandle;
extern Display_Handle display;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*!
    \brief  Sync_Init

    This function is called when Sync_Timer overflows.
    In this design, we just toggle the CC1310_Sync_PWM. // TODO

    \param  None

    \return None

*/
static void SyncOutputHandle(Timer_Handle handle, int_fast16_t status)
{
  //  GPIO_toggle(CC1310_Sync_PWM); // CC1310 RAT should set both edge as input

}

/*!
    \brief  EventRecvHandle

    Callback from GPIO ISR

    \param  None

    \return void

*/
void EventRecvHandle(uint_least8_t index)
{
    GPIO_toggle(Status_LED_2_GPIO); // CC1310 RAT should set both edge as input
    /* 释放信号量 */
    sem_post(&EvtDataRecv);
}

/*!
    \brief  cc1310_EventGet

    I2C transfer to get event from cc1310

    \param  i2cHandle - I2C object
            pdata - write the event data to this point
            num - num of data to get

    \return void

*/
static void cc1310_EventGet(I2C_Handle i2cHandle, uint8_t* pdata, uint8_t num)
{
    bool ret = true;
    uint32_t errstate;
    uint8_t datanum = num;
    uint8_t dataget = 0;

    I2C_HWAttrs const *hwAttrs = i2cHandle->hwAttrs;
    uint32_t I2C_BASE = 0x40020000;// = hwAttrs->baseAddr;

    /* Master RECEIVE of Multiple Data Bytes */

    /* Write Slave Address to I2CMSA, receive mode */
    I2CMasterSlaveAddrSet( I2C_BASE,                \
                           CC1310_ADDR,             \
                           true);  ///< true - I2C read

    /* Write ---01011 to I2C_O_MCS */
    I2CMasterControl(I2C_BASE,I2C_MASTER_CMD_BURST_RECEIVE_START);

    /* wait until no busy & no error */
    while(I2CMasterBusy(I2C_BASE) == true);
    errstate = I2CMasterErr(I2C_BASE);

    if(errstate == I2C_MASTER_ERR_NONE){
        /* read one byte from I2CMDR */
        *(pdata+dataget) = I2CMasterDataGet(I2C_BASE);
        dataget++;
    }

    while( dataget!= (datanum-1) && datanum!=1 ){
        /* Write ---01001 to I2CMCS */
        I2CMasterControl(I2C_BASE,I2C_MASTER_CMD_BURST_RECEIVE_CONT);

        /* wait until no busy & no error */
        while(I2CMasterBusy(I2C_BASE) == true);
        errstate = I2CMasterErr(I2C_BASE);

        if(errstate == I2C_MASTER_ERR_NONE){
            /* read one byte from I2CMDR */
            *(pdata+dataget) = I2CMasterDataGet(I2C_BASE);
            dataget++;
        }else
            goto errorServ;
    }

    /* Write ---00101 to I2CMCS */
    I2CMasterControl(I2C_BASE,I2C_MASTER_CMD_BURST_SEND_FINISH);
    /* wait until no busy & no error */
    while(I2CMasterBusy(I2C_BASE) == true);
    errstate = I2CMasterErr(I2C_BASE);

    if(errstate == I2C_MASTER_ERR_NONE){
        /* read one byte from I2CMDR */
        *(pdata+dataget) = I2CMasterDataGet(I2C_BASE);
    }else
        goto errorServ;

errorServ:
    if( errstate&I2C_MCS_ARBLST ){
        /* Write ---0-100 to I2CMCS */
        I2CMasterControl(I2C_BASE,I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
        ret = false;
    }
    else
        ret = false;


     return ret;
}

/*!
    \brief  Eventbacktracking

    事件标签的时间回溯 @ref task/README.md

    \param  Tror Tsor Tsoc @ref task/README.md

    \return Troc @ref task/README.md

*/
static uint32_t Eventbacktracking(uint32_t Tror, uint32_t Tsor)
{
    uint32_t ret = 0;

    return ret;
}


/*********************************************************************
 *  FUNCTIONS
 */

void SyncTask(uint32_t arg0, uint32_t arg1)
{

    Timer_Params    params;

    // Initialize Timer parameters
    Timer_Params_init(&params);
    params.periodUnits = Timer_PERIOD_US;
    params.period = 1000000; // 1s
    params.timerMode  = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = SyncOutputHandle;
    // Open Timer instance
    pSyncTime = Timer_open(Sync_Timer, &params);

    /* Register interrupt for the CC1310_WAKEUP (cc1310 trigger) */
    GPIO_setCallback(CC1310_WAKEUP, EventRecvHandle);
    GPIO_enableInt(CC1310_WAKEUP);

    Display_printf(display, 0, 0, "SyncTask ready\r\n");

    while(1){

        /* 等待信号量 */
        sem_wait(&EvtDataRecv);

        /* I2C 读取事件标签 */
        cc1310_EventGet(i2cHandle,I2C_BUFF,3);
        //memcpy(&Tror, (uint8_t*)&I2C_BUFF[1],4);
        //memcpy(&Tsor, (uint8_t*)&I2C_BUFF[5],4);
        //type = I2C_BUFF[9];

        /* 事件标签时间戳回溯 */
        //Troc = Eventbacktracking(Tror,Tsor);

        Display_printf(display, 0, 0, "Receive %x,%x,%x \n\r",I2C_BUFF[0],I2C_BUFF[1],I2C_BUFF[2]);


    }

}
