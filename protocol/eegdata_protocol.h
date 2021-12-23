#ifndef __EEGDATA_PROTOCOL_H
#define __EEGDATA_PROTOCOL_H

/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include <stdint.h>
#include <service/timestamp.h>

/*********************************************************************
 * Macros
 */
/* 脑电数据通道参数*/
#define UDP_SAMPLE_FH               0x23    //!< UDP帧数据域 样起始分隔符
#define UDP_SAMPLENUM               10      //!< UDP每包含样本数

// 发送缓冲区参数
#ifdef Dev_Ch32
#define UDP_DTx_Buff_Size           1173    //!< 数据帧头部23 + 样本数10 x（数据域头部7 + (本组通道状态3 + 八通道8 x 每通道量化字节数3）x 通道组数4)字节
#define UDP_SampleValSize           108     //!< 通道组数4 - 108字节状态+量化值
#define CHANNEL_NUM                 32      //!< 通道数量  （x8/x16/x24/x32）
#endif
#ifdef Dev_Ch24
#define UDP_DTx_Buff_Size           903     //!< 数据帧头部23 + 样本数10 x（数据域头部7 + (本组通道状态3 + 八通道8 x 每通道量化字节数3）x 通道组数3)字节
#define UDP_SampleValSize           81      //!< 通道组数3 - 81字节状态+量化值
#define CHANNEL_NUM                 24      //!< 通道数量  （x8/x16/x24/x32）
#endif
#ifdef Dev_Ch16
#define UDP_DTx_Buff_Size           633     //!< 数据帧头部23 + 样本数10 x（数据域头部7 + (本组通道状态3 + 八通道8 x 每通道量化字节数3）x 通道组数2)字节
#define UDP_SampleValSize           54      //!< 通道组数2 - 51字节状态+量化值
#define CHANNEL_NUM                 16      //!< 通道数量  （x8/x16/x24/x32）
#endif
#ifdef Dev_Ch8
#define UDP_DTx_Buff_Size           363     //!< 数据帧头部23 + 样本数10 x（数据域头部7 + (本组通道状态3 + 八通道8 x 每通道量化字节数3）x 通道组数1)字节
#define UDP_SampleValSize           27      //!< 通道组数1 - 27字节状态+量化值
#define CHANNEL_NUM                 8      //!< 通道数量  （x8/x16/x24/x32）
#endif

/*******************************************************************
 * TYPEDEFS
 */

/*!
    \brief    UDP帧头数据 结构体

    [DANFER]  本结构体内数据需通过属性表获取
 */
#pragma pack(push)
#pragma pack(1)

 typedef struct
{
     uint32_t DevID;                //!< 设备ID
     uint8_t  UDPNum[4];            //!< UDP包累加滚动码
     uint8_t  UDPSampleNum[2];      //!< 本UDP包总样数
     uint8_t  UDP_ChannelNum;       //!< 本UDP包有效通道总数
     uint8_t  UNIXTimeStamp[8];     //!< Unix时间戳（未用）
     uint8_t  ReservedNum[4];       //!< 保留数
} UDPHeader_t;

/*!
    \brief  UDP数据通道 数据帧数据域结构体 - 一个通道组（8通道）
            本结构体定义UDP数据通道 数据帧数据域格式
 */
#pragma pack(push)
#pragma pack(1)

typedef struct
{
    uint8_t     FrameHeader;                        //!< 起始分隔符
    uint8_t     Index[2];                           //!< 样本序号
    uint8_t     Timestamp[4];                       //!< 本样本时间戳
    uint8_t     ChannelVal[UDP_SampleValSize];      //!< 8通道状态+量化值
}UDPData_t;

/*!
    \brief    UDP数据通道发送缓冲区 联合体
              本联合体定义UDP脑电数据通道的发送缓冲区帧格式
 */
typedef //union
//{
   //uint8_t UDP_DTx_Buff[UDP_DTx_Buff_Size];
   struct
   {
       /* 数据帧头部     - 23字节*/
       UDPHeader_t sampleheader;

       /* 数据帧数据域 */
       UDPData_t sampledata[UDP_SAMPLENUM];

   //} UDPframe;
} UDPFrame_t;

/**********************************************************************
 * FUNCTIONS
 */

bool UDP_DataGet(uint8_t SampleIndex);
bool UDP_DataProcess(SampleTime_t *pSampleTime,bool error);

#endif  /* __EEGDATA_PROTOCOL_H */
