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

#include <ti/display/Display.h>

#include "eegdata_protocol.h"
#include <utility/stateMachine.h>
#include <drivers/ads1299.h>

/*********************************************************************
 *  LOCAL VARIABLES
 */
/* 脑电数据通道变量 */
static uint32_t UDPNum;                 //!< 打包帧数
static uint8_t  AttrLen;                //!< 属性长度 - 按字节 （for debug）
static uint8_t* len=&AttrLen;           //!< for debug

/*********************************************************************
 *  GLOBAL VARIABLES
 */
UDPFrame_t UDPD_TX_Buff;                //!< UDP发送缓冲区

/*********************************************************************
 *  FUNCTIONS
 */

/*!
    \brief  UDP_DataFrameHeaderGet

    脑电数据帧协议 帧头数据获取
    对UDP帧头封包需要获取属性表中的属性值，只在UDP第一次帧头封包时调用本函数，将属性值存到静态变量中方便后续封包。
 */
static void UDP_DataFrameHeaderGet()
{

    extern SlDeviceVersion_t ver;

    /* 数据源 */
    UDP_TX_Buff.sampleheader.DevID = ver.DevID; //TODO check if right

    /* 本UDP包总样数 */
    UDP_TX_Buff.sampleheader.UDPSampleNum = UDP_SAMPLENUM;

    /* 本UDP包有效通道总数 - 目前用总通道数代替*/
    UDP_TX_Buff.sampleheader.UDP_ChannelNum = CHANNEL_NUM;

    /* 保留数 */
    memset((uint8_t*)&(UDP_TX_Buff.sampleheader.ReservedNum),0xFF,4);

}

/*!
    \brief UDP_DataGet 脑电数据通道读取AD数据

    \param  SampleIndex - AD当前样本序号
            Procesflag - EEG数据采集状态标志

    \return true - 读取AD数据成功
 *          false - 异常
 */
bool UDP_DataGet(uint8_t SampleIndex,uint8_t Procesflag)
{
    if( Procesflag&EEG_DATA_ACQ_EVT )
    {
        ADS1299_ReadResult((uint8_t *)&(UDP_TX_Buff.sampledata[SampleIndex].ChannelVal[0]));  //!< 样本每通道量化值
        //ADS1299_ReadResult((uint8_t *)&(UDP_TX_Buff1[28*SampleIndex])); //for debug
        return true;
    }
    else
        return false;

}

/*!
    \brief  UDP_DataProcess 脑电数据通道 帧协议服务处理函数(EEG数据封包)

    \param  pSampleTime - AD时间戳指针（时间戳服务提供的每样时间信息）
            Procesflag - EEG数据采集状态标志

    \return success - UDP打包数据完毕
            false - 异常
 */
bool UDP_DataProcess(SampleTime_t *pSampleTime,uint8_t Procesflag)
{
    uint8_t Index;

    /* 数据域封包 */
    for(Index=0;Index<UDP_SAMPLENUM;Index++)
    {
        UDP_TX_Buff.sampledata[Index].FrameHeader = UDP_SAMPLE_FH;      //!< 样本起始分隔符
        UDP_TX_Buff.sampledata[Index].Index[0] = Index;                 //!< 样本序号 - 低8位，序数从0开始
        memcpy((uint8_t*)&(UDP_TX_Buff.sampledata[Index].Timestamp[0]),\
               (uint8_t*)&(pSampleTime->CurTimeStamp[Index]),4);        //!< 每样增量时间戳
    }

     /*帧头部封包 */

     //!< 发生过EEG暂停采集事件或第一次UDP帧头封包
     if((( Procesflag & EEG_STOP_EVT )!=0)  ||  (UDPNum==0))
     {
        UDP_DataFrameHeaderGet(); //!< 重新获取UDP帧头数据
        UDPNum=0; //!< UDP包头重新计数
     }

     /* UDP包累加滚动码 */
     memcpy((uint8_t *)&(UDP_TX_Buff.sampleheader.UDPNum),(uint8_t *)&UDPNum,2); //!< UDP包累加滚动码,也即UDP帧头封包执行次数
     UDPNum++;

     return true;
}



