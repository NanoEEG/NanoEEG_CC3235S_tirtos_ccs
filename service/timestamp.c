/**
 * @file    timestamp.c
 * @author  gjmsilly
 * @brief   NanoEEG 时间戳服务
 * @version 1.0.0
 * @date    2020-09-01
 *
 * @copyright (c) 2020 gjmsilly
 *
 */

/*******************************************************************
 * INCLUDES
 */
#include "timestamp.h"
#include "ti_drivers_config.h"

/* Driverlib header files */
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_timer.h>
#include <ti/devices/cc32xx/driverlib/timer.h>


/*******************************************************************
 *  LOCAL VARIABLES
 */
static SampleTime_t SampleTime; //传递该对象地址到应用层

/*******************************************************************
 *  Callback
 */

/*!
    \brief SampleTimerCB

    每样本时间戳溢出中断回调,由Timer溢出中断触发的回调

    \param  handle - 定时器对象

    \return NULL
 */
static void SampleTimerCB(Timer_Handle handle, int_fast16_t status)
{
    SampleTime.BaseTime_10us += 4000000;
}


/*******************************************************************
 *  FUNCTIONS
 */

/*!
     \brief SampleTimestamp_Service_Init

     每样本增量时间戳初始化

     \return 每样本时间戳结构体指针
 */
SampleTime_t* SampleTimestamp_Service_Init(Timer_Params *params)
{
	
    /* CC3235S 每个32bit的timer由16bit的timerA和timerB串联构成，时钟为80MHz，最多计时约50s
     * 以us为单位计算，计数约每53687091次溢出中断，触发回调，此处设置40s触发一次中断进行软计数
     * 写入数据帧的时间戳时间精度为 10us,处理方式为: 4000000*计时器中断次数 + 计时器当前值/800 (单位:10us)
     * */

    Timer_Params_init(params);

    params->period = 40000000;
    params->periodUnits = Timer_PERIOD_US;
    params->timerMode = Timer_CONTINUOUS_CALLBACK;
    params->timerCallback = SampleTimerCB;

    SampleTime.SampleTimer = Timer_open(System_Timer, params);

    if (SampleTime.SampleTimer == NULL) {
        /* Failed to initialized timer */
        while (1) {}
    }

    return(&SampleTime);

}

/*!
     \brief SampleTimestamp_Reset

     样本时间戳清零

     \return None
 */
void SampleTimestamp_Reset(SampleTime_t* SampleTime){

    // 清空时钟的值，TI Driver不支持，用driverlib实现
    // pSampleTime->SampleTimer - Timer0
    Timer_HWAttrs const *hwAttrs = SampleTime->SampleTimer->hwAttrs;
    uint32_t TimerBase = hwAttrs->baseAddress;

    TimerValueSet(TimerBase,TIMER_A,0x00);
    TimerValueSet(TimerBase,TIMER_B,0x00);

    SampleTime->LastSyncTime_10us = 0x00;

}

