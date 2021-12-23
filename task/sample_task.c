/**
 * @file    sample_task.c
 * @author  gjmsilly
 * @brief   NanoEEG 处理AD采样的线程
 * @version 1.0.0
 * @date    2021-12-23
 *
 * @copyright (c) 2021 gjmsilly
 *
 */

/********************************************************************
 * INCLUDES
 */
#include <ti/drivers/GPIO.h>
#include <service/timestamp.h>
#include <protocol/eegdata_protocol.h>
#include <ti/display/Display.h>

/* POSIX Header files */
#include <semaphore.h>

#include "sample_task.h"
#include "ti_drivers_config.h"

/*********************************************************************
 *  GLOBAL VARIABLES
 */
uint8_t eegSamplingState;               //!< AD采样状态标志位

/*********************************************************************
 *  EXTERNAL VARIABLES
 */
extern SampleTime_t *pSampleTime;
extern Display_Handle display;
extern sem_t UDPDataReady;
extern sem_t SampleReady;

/*********************************************************************
 * FUNCTIONS
 */

/*!
    \brief  ADS1299nDRDYHandle

    Callback from GPIO ISR

    \param  None

    \return void

*/
void ADS1299nDRDYHandle(uint_least8_t index)
{
    /* 释放信号量 */
    sem_post(&SampleReady);

}


/*!
    \brief  Sample task

    This task handles the Sample related event

    \param  None

    \return void

*/
void SampleTask(uint32_t arg0, uint32_t arg1)
{
    static uint8_t SampleIndex = 0; //!< 样本序号

    /* Register interrupt for the Mod_nDRDY (EEG trigger) */
    GPIO_setCallback(Mod_nDRDY, ADS1299nDRDYHandle);

    Display_printf(display, 0, 0, "Sample task ready\r\n");

    while(1)
    {
        /* 等待信号量,由Mod_nDRDY中断释放，等不到则阻塞 */
        sem_wait(&SampleReady);

        /* 信号量释放，开始运行 */

        /* 每次开始采样时对样本序号清零 */
        if( eegSamplingState & EEG_DATA_START_EVT )
        {
            SampleIndex=0;
        }

        /* 一包数据 采样中 */
        pSampleTime->CurTimeStamp[SampleIndex] = pSampleTime->Time_40s + \
                Timer_getCount(pSampleTime->SampleTimer)/800; //!< 获取当前时间

        eegSamplingState |= EEG_DATA_ACQ_EVT; //!< 更新事件：一包AD数据采集中
        eegSamplingState &= ~EEG_DATA_START_EVT; //!< 清除前序事件 - 一包ad数据开始采集

        if(UDP_DataGet(SampleIndex)) //!< 获取AD数据
        {
            SampleIndex++; //!< 样本序号+1
        }

        /* 一包数据最后一个样本采样完毕 */
        if(SampleIndex == UDP_SAMPLENUM )
        {
            SampleIndex=0; //!< 样本序号归零

            eegSamplingState |= EEG_DATA_CPL_EVT; //!< 更新事件：一包ad数据采集完成
            eegSamplingState &= ~EEG_DATA_ACQ_EVT; //!< 清除前序事件 - 一包AD数据采集中

            if(UDP_DataProcess(pSampleTime,eegSamplingState&EEG_STOP_EVT)) //!< 完成最后的封包工作
                sem_post(&UDPDataReady); //!< 释放信号量给UDP线程 将一包数据发送
        }

    }

}

