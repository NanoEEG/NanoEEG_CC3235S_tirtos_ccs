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

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/I2C.h>

/* POSIX Header files */
#include <semaphore.h>

/* User defined Header files */
#include <protocol/evtdata_protocol.h>
#include <attr/attrTbl.h>

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
extern sem_t UDPEvtDataReady;
extern sem_t EvtDataRecv;
extern SampleTime_t *pSampleTime;
extern I2C_Handle i2cHandle;

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

    GPIO_toggle(LED_RED_GPIO); // CC1310 RAT should set both edge as input
}

/*!
    \brief  EventRecvHandle

    Callback from GPIO ISR

    \param  None

    \return void

*/
static void EventRecvHandle(uint_least8_t index)
{
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
static void cc1310_EventGet(I2C_Handle i2cHandle, uint8_t* pdata)
{

    return;
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

    while(1){

        /* 等待信号量 */
        sem_wait(&EvtDataRecv);

        /* I2C 读取事件标签 */
        cc1310_EventGet(i2cHandle,I2C_BUFF);
        memcpy(&Tror, (uint8_t*)&I2C_BUFF[1],4);
        memcpy(&Tsor, (uint8_t*)&I2C_BUFF[5],4);
        type = I2C_BUFF[9];

        /* 事件标签时间戳回溯 */
        Troc = Eventbacktracking(pSampleTime,Tror,Tsor);
        /* 协议封包 */
        App_GetAttr(TRIGDELAY, &delay);
        UDP_DataProcess(Troc, delay, type);

        /* 释放信号量，发送事件标签给上位机 */
        sem_post(&UDPEvtDataReady);
    }

}



