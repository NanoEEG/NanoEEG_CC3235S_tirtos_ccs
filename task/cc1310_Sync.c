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
/* TI-DRIVERS Header files */
#include "ti_drivers_config.h"

/* TI-RTOS Header files */
#include <ti/drivers/GPIO.h>

/* POSIX Header files */
#include <semaphore.h>

/* User defined Header files */
#include "cc1310_Sync.h"
#include <protocol/evtdata_protocol.h>

/*********************************************************************
 *  LOCAL VARIABLES
 */
static Timer_Handle SyncTimer;

/*********************************************************************
 *  EXTERNAL VARIABLES
 */
extern sem_t UDPEvtDataReady;
extern SampleTime_t *pSampleTime;
extern UDPEvtFrame_t UDP_EvtTX_Buff;

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
static void SyncOutput(Timer_Handle handle, int_fast16_t status)
{
    UDP_DataFrameHeaderGet();
    //测试版本
    // 测试版本 ： 每1s释放信号量
    UDP_EvtTX_Buff.Evtdata.Timestamp = pSampleTime->BaseTime_10us + Timer_getCount(pSampleTime->SampleTimer)/800; //!< 获取当前时间
    UDP_EvtTX_Buff.Evtdata.evtType = 0xaa;

    /* 等待信号量 */
    sem_post(&UDPEvtDataReady);

    GPIO_toggle(LED_RED_GPIO); // CC1310 RAT should set both edge as input
}

/*!
    \brief  Sync_Init

    Initializate the hardware, create 1s SyncTimer.

    \param  None

    \return Timer_Handle

*/
Timer_Handle Sync_Init()
{
    Timer_Params    params;

    // Initialize Timer parameters
    Timer_Params_init(&params);
    params.periodUnits = Timer_PERIOD_US;
    params.period = 1000000; // 1s
    params.timerMode  = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = SyncOutput;
    // Open Timer instance
    SyncTimer = Timer_open(Sync_Timer, &params);

    return SyncTimer;
}



