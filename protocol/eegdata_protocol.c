/**
 * @file    eegdata_protocol.c
 * @author  gjmsilly
 * @brief   NanoEEG UDP1脑电数据通道协议
 * @version 1.0.0
 * @date    2020-12-27
 *
 * @copyright (c) 2020 gjmsilly
 *
 */

/*********************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

#include <ti/drivers/net/wifi/slnetifwifi.h>

#include "eegdata_protocol.h"
#include <service/ads1299.h>


/*********************************************************************
 *  LOCAL VARIABLES
 */
static uint32_t UDPNum;                 //!< UDP包累加滚动码

/*********************************************************************
 *  GLOBAL VARIABLES
 */
UDPDtFrame_t UDP_DTX_Buff;                //!< UDP发送缓冲区

/*********************************************************************
 *  LOCAL FUNCTIONS
 */

/*!
    \brief  UDP_DataFrameHeaderGet

    脑电数据通道 数据帧头部静态变量获取
    只在第一次帧头封包时调用本函数,帧头部的静态变量在采集过程中不需要更新。
 */
static void UDP_DataFrameHeaderGet()
{

    extern SlDeviceVersion_t ver;

    /* 设备ID */
    UDP_DTX_Buff.sampleheader.DevID = ver.ChipId;

    /* 本UDP包总样本数 */
    UDP_DTX_Buff.sampleheader.UDPSampleNum[0] = UDP_SAMPLENUM;

    /* 本UDO包有效通道总数 */
    UDP_DTX_Buff.sampleheader.UDP_ChannelNum = CHANNEL_NUM;

    /* 保留数 */
    memset((uint8_t*)(UDP_DTX_Buff.sampleheader.ReservedNum),0xFF,4);

}

/*********************************************************************
 *  FUNCTIONS
 */

/*!
    \brief UDP_DataGet 
    
    脑电数据通道 数据帧数据域 量化通道值获取，调用本函数获取一个样本，
    并按照指定样本序号将量化通道值填充至UDP数据帧数据域指定位置。

    \param  SampleIndex - 样本序号
            Processflag - EEG数据采集状态标志

    \return true - 读取AD数据成功
 *          false - 异常
 */
bool UDP_EEGDataGet(uint8_t SampleIndex)
{
    ADS1299_ReadResult((uint8_t *)&(UDP_DTX_Buff.sampledata[SampleIndex].ChannelVal[0]));  //!< 样本每通道量化值
    //ADS1299_ReadResult((uint8_t *)&(UDP_DTx_Buff[28*SampleIndex])); //for debug
    return true;
}

/*!
    \brief  UDP_DataProcess 
    
    脑电数据通道 数据帧封包处理，本函数在一包EEG样本获取完毕后调用，
    本函数负责处理数据域封包和帧头部封包。

    \param  pSampleTime  -  EEG样本时间戳定时器对象，该对象应提供一包
                            数据帧中各个样本的精密时间戳
            reSampleFlag -   本次采样前发生过采样停止

    \return success - UDP打包数据完毕
            false - 异常
 */
bool UDP_EEGDataProcess(SampleTime_t *pSampleTime,bool reSampleFlag)
{
    uint8_t Index;

    /* 数据域封包 */
    for(Index=0; Index<UDP_SAMPLENUM; Index++)
    {
        UDP_DTX_Buff.sampledata[Index].FrameHeader = UDP_SAMPLE_FH;      //!< 样本起始分隔符
        UDP_DTX_Buff.sampledata[Index].Index[0] = Index;                 //!< 样本序号 - 低8位，序数从0开始
        memcpy((uint8_t*)&(UDP_DTX_Buff.sampledata[Index].Timestamp[0]),\
               (uint8_t*)&(pSampleTime->CurTimeStamp[Index]),4);        //!< 每样增量时间戳
    }

     /* 帧头部封包 */

     //!< 发生过EEG暂停采集或第一次UDP帧头封包
     if( reSampleFlag ||  (UDPNum==0) )
     {
        UDP_DataFrameHeaderGet(); //!< 重新获取UDP帧头数据
        UDPNum = 0; //!< UDP包累加滚动码重新计数
     }

     /* UDP包累加滚动码 */
     memcpy((uint8_t *)&(UDP_DTX_Buff.sampleheader.UDPNum),(uint8_t *)&UDPNum,4); //!< UDP包累加滚动码,也即UDP帧头封包执行次数
     UDPNum++;

     return true;
}



