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

/* POSIX Header files */
#include <semaphore.h>

/* User defined Header files */
#include <protocol/evtdata_protocol.h>
#include <attr/attrTbl.h>

/*********************************************************************
 *  MACROS
 */
// Define the slave address of cc1310 on NanoEEG
#define CC1310_ADDR     0x33
// For math
#define INT_MAX         4294967296
#define RAT_1SCNT       4000000
#define RAT_SYNCNT      4000000
#define CC3235_1SCNT    100000


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
uint32_t SyncTimerBase;

/*********************************************************************
 *  EXTERNAL VARIABLES
 */
extern sem_t UDPEvtDataReady;
extern sem_t EvtDataRecv;
extern SampleTime_t *pSampleTime;
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
    // 获取当前时间作为Tsoc @ref task/README.md
    pSampleTime->LastSyncTime_10us = pSampleTime->BaseTime_10us + \
            Timer_getCount(pSampleTime->SampleTimer)/800;

    GPIO_toggle(CC1310_Sync_PWM); // CC1310 RAT should set both edge as input
}

/*!
    \brief  EventRecvHandle

    Callback from GPIO ISR

    \param  None

    \return void

*/
static void EventRecvHandle(uint_least8_t index)
{
    GPIO_toggle(LED_RED_GPIO); // RED_LED to indicate working well
    /* 释放信号量 */
    sem_post(&EvtDataRecv);
}

/*!
    \brief  cc1310_EventGet

    I2C transfer to get event from cc1310

    \param  i2cHandle - I2C object
            pdata - write the event data to this point

    \return void

*/
static bool cc1310_EventGet(I2C_Handle i2cHandle, uint8_t* pdata, uint8_t num)
{
    bool ret = true;

    uint32_t errstate; //TODO
    uint8_t datanum = num;
    uint8_t dataget = 0;
    uint32_t key;

    I2C_HWAttrs const *hwAttrs = i2cHandle->hwAttrs;
    uint32_t I2C_BASE = hwAttrs->baseAddr;

    /* Master RECEIVE of Multiple Data Bytes */
     key = HwiP_disable();
     /* Write Slave Address to I2CMSA, receive mode */
     I2CMasterSlaveAddrSet( I2C_BASE,                \
                            CC1310_ADDR,             \
                            true);  ///< true - I2C read
     /* Write ---01011 to I2C_O_MCS */
     I2CMasterControl(I2C_BASE,I2C_MASTER_CMD_BURST_RECEIVE_START);

     /* wait until no busy & no error */
     while((HWREG(I2C_BASE + I2C_O_MCS) & I2C_MCS_ADRACK));
     while(I2CMasterBusy(I2C_BASE) == true);
     errstate = I2CMasterErr(I2C_BASE);

     if(errstate == I2C_MASTER_ERR_NONE){
         /* read one byte from I2CMDR */
         *pdata = I2CMasterDataGet(I2C_BASE);
         dataget++;
     }
     while((HWREG(I2C_BASE + I2C_O_MCS) & I2C_MCS_ACK));

     while( dataget!= (datanum-1) && datanum!=1 ){

         /* Write ---01001 to I2CMCS */
         I2CMasterControl(I2C_BASE,I2C_MASTER_CMD_BURST_RECEIVE_CONT);
         /* wait until no busy & no error */
         while((HWREG(I2C_BASE + I2C_O_MCS) & I2C_MCS_ACK));
         while(I2CMasterBusy(I2C_BASE) == true);
         errstate = I2CMasterErr(I2C_BASE);

         if(errstate == I2C_MASTER_ERR_NONE){
             /* read one byte from I2CMDR */
             *(pdata+dataget) = I2CMasterDataGet(I2C_BASE);
             dataget++;
         }
     }

     while((HWREG(I2C_BASE + I2C_O_MCS) & I2C_MCS_ACK));
     /* Write ---00101 to I2CMCS */
     I2CMasterControl(I2C_BASE,I2C_MASTER_CMD_BURST_SEND_FINISH);

     /* wait until no busy & no error */
     while((HWREG(I2C_BASE + I2C_O_MCS) & I2C_MCS_ERROR));
     while(I2CMasterBusy(I2C_BASE) == true);
     if(errstate == I2C_MASTER_ERR_NONE){
         /* read one byte from I2CMDR */
         *(pdata+dataget) = I2CMasterDataGet(I2C_BASE);
     }

     HwiP_restore(key);

     return ret;
}

/*!
    \brief  Eventbacktracking

    事件标签的时间回溯 @ref task/README.md

    \param  Tror Tsor Tsoc @ref task/README.md

    \return Troc @ref task/README.md

*/
static uint32_t Eventbacktracking(SampleTime_t* pSampleTime, uint32_t Tror, uint32_t Tsor)
{
    uint32_t ret = 0;
    uint32_t t = 0;

    if(Tror>Tsor || Tror==Tsor){
        t = (Tror-Tsor)/(RAT_1SCNT);
    }else
        t = (Tsor + RAT_SYNCNT - INT_MAX - Tror)/(RAT_1SCNT);


    ret = pSampleTime->LastSyncTime_10us + t*CC3235_1SCNT;

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

    // 获取时钟基地址
    Timer_HWAttrs const *hwAttrs = pSyncTime->hwAttrs;
    SyncTimerBase = hwAttrs->baseAddress;

    /* Register interrupt for the CC1310_WAKEUP (cc1310 trigger) */
    GPIO_setCallback(CC1310_WAKEUP, EventRecvHandle);
    GPIO_enableInt(CC1310_WAKEUP);

    while(1){

        /* 等待信号量 */
        sem_wait(&EvtDataRecv);

        /* I2C 读取事件标签 */
        cc1310_EventGet(i2cHandle,I2C_BUFF,10);

        memcpy(&Tror, &I2C_BUFF[1],4);
        memcpy(&Tsor, &I2C_BUFF[5],4);
        type = I2C_BUFF[9];

        /* 事件标签时间戳回溯 */
        Troc = Eventbacktracking(pSampleTime,Tror,Tsor);
        /* 协议封包 */
        App_GetAttr(TRIGDELAY, &delay);
        UDP_DataProcess(Troc, delay, type);

        Display_printf(display, 0, 0,"Tror %u. Tsor %u. type %x Troc %u.\r\n",Tror,Tsor,type,Troc);

        /* 释放信号量，发送事件标签给上位机 */
        sem_post(&UDPEvtDataReady);
    }

}



