/**
 * @file    timestamp.h
 * @author  gjmsilly
 * @brief   NanoEEG 时间戳服务
 * @version 1.0.0
 * @date    2020-09-01
 *
 * @copyright (c) 2020 gjmsilly
 *
 */

#ifndef SERVICE_TIMESTAMP_H_
#define SERVICE_TIMESTAMP_H_
/*******************************************************************
 * INCLUDES
 */
#include <ti/drivers/Timer.h>

/*******************************************************************
 * TYPEDEFS
 */

/*!
    \brief  SampleTime_t

    每样本精密时间戳 结构体
 */
typedef struct
{
    Timer_Handle    SampleTimer;        //!< 系统定时器
    uint32_t        BaseTime_10us;      //!< 基础时间/10us = 计时器溢出次数*4000000
    uint32_t        CurTimeStamp[10];   //!< 每样本当前时间
    uint32_t        LastSyncTime_10us;  //!< 最近一次同步时间/10us Tsoc

}SampleTime_t;


/*********************************************************************
 * FUNCTIONS
 */
/* 时间戳服务 */
SampleTime_t* SampleTimestamp_Service_Init(Timer_Params *params);


#endif /* SERVICE_TIMESTAMP_H_ */
