#ifndef __PROTOCOL_WIFI_H__
#define __PROTOCOL_WIFI_H__

/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include <stdint.h>
#include <service/NanoEEG_misc.h>

/*********************************************************************
 * Macros
 */
/* TCP控制通道参数 */

// 上位机->设备 指令解析
#define TCP_Recv_FH                 0xAC    //!< TCP接收帧头
#define TCP_Recv_FT                 0xCC    //!< TCP接收帧尾
// 设备->上位机 回复
#define TCP_Send_FH                 0xA2    //!< TCP发送帧头
#define TCP_Send_FT                 0xC2    //!< TCP发送帧尾
// 指令码
#define DummyIns                    0x00    //!< 空指令
#define CAttr_Read                  0x01    //!< 读一个普通属性
#define CAttr_Write                 0x10    //!< 写一个普通属性
#define ChxAttr_Read                0x02    //!< 读一个通道属性
#define ChxAttr_Write               0x20    //!< 写一个通道属性

// 通讯收发缓冲区参数
#define TCP_Rx_Buff_Size            16
#define TCP_Tx_Buff_Size            16

/* UDP数据通道参数*/
#define UDP_SAMPLE_FH               0x23    //!< UDP帧数据域 样起始分隔符
#define UDP_SAMPLENUM               10      //!< UDP每包含ad样本数

// 通讯发送缓冲区参数
#ifdef Dev_Ch32
#define UDP_Tx_Buff_Size            1173    //!< 数据帧头部23 + 样本数10 x（数据域头部7 + (本组通道状态3 + 八通道8 x 每通道量化字节数3）x 通道组数4)字节
#define UDP_SampleValSize           108     //!< 通道组数4 - 108字节状态+量化值
#endif
#ifdef Dev_Ch24
#define UDP_Tx_Buff_Size            903     //!< 数据帧头部23 + 样本数10 x（数据域头部7 + (本组通道状态3 + 八通道8 x 每通道量化字节数3）x 通道组数3)字节
#define UDP_SampleValSize           81      //!< 通道组数3 - 81字节状态+量化值
#endif
#ifdef Dev_Ch16
#define UDP_Tx_Buff_Size            633     //!< 数据帧头部23 + 样本数10 x（数据域头部7 + (本组通道状态3 + 八通道8 x 每通道量化字节数3）x 通道组数2)字节
#define UDP_SampleValSize           54      //!< 通道组数2 - 51字节状态+量化值
#endif
#ifdef Dev_Ch8
#define UDP_Tx_Buff_Size            363     //!< 数据帧头部23 + 样本数10 x（数据域头部7 + (本组通道状态3 + 八通道8 x 每通道量化字节数3）x 通道组数1)字节
#define UDP_SampleValSize           27      //!< 通道组数1 - 27字节状态+量化值
#endif

/*******************************************************************
 * TYPEDEFS
 */

/*!
 *  @def    声明 读属性回调函数原型
 *  @param  InsAttrNum - 待读属性编号
 *          CHxNum - 通道编号（通道属性专用，默认不用   0xFF）
 *          pValue - 属性值 （to be returned）
 *          pLen - 属性值大小（to be returned）
 */
typedef uint8_t (*pfnReadAttrCB_t)( uint8_t InsAttrNum, uint8_t CHxNum,
                                    uint8_t *pValue, uint8_t *pLen );

/*!
 *  @def    声明 写属性回调函数原型
 *  @param  InsAttrNum - 待写入属性编号
 *          CHxNum - 通道编号（通道属性专用，默认不用   0xFF）
 *          pValue - 待写入数据的指针
 *          pLen - 待写入数据大小
 */
typedef uint8_t (*pfnWriteAttrCB_t)( uint8_t InsAttrNum, uint8_t CHxNum,
                                     uint8_t *pValue, uint8_t len );

/*!
 *  @def    属性读写回调函数 结构体
 */
typedef struct
{
  pfnReadAttrCB_t   pfnReadAttrCB;                  //!< 读属性回调函数指针
  pfnWriteAttrCB_t  pfnWriteAttrCB;                 //!< 写属性回调函数指针
} AttrCBs_t;

/*!
 *  @def    TCP控制通道帧 结构体
 *
 *  @brief  本结构体定义TCP控制通道一次帧收发的数据
 */
 typedef struct
{
    uint8_t FrameHeader;    //!< 帧头
    uint8_t FrameLength;    //!< 有效帧长
    uint8_t DataLength;     //!< 数据帧长
    uint8_t InsNum;         //!< 指令码
    uint8_t InsAttrNum;     //!< 指令作用属性编号
    uint8_t ChxNum;         //!< 指令作用通道
    uint8_t ERR_NUM;        //!< 错误码
    uint8_t *pDataLength;   //!< 数据帧长指针（读回调用）
    uint8_t *_OP_;          //!< 操作立即数指针（写回调用）
} TCPFrame_t;

/*!
 *  @def    UDP帧头数据 结构体
 *
 *  @brief  ！本结构体内数据需通过属性表获取
 */
 typedef struct
{
     uint8_t  DevID[4];             //!< 设备ID
     uint8_t  UDPNum[4];            //!< UDP包累加滚动码
     uint8_t  UDPSampleNum[2];      //!< 本UDP包总样数
     uint8_t  UDP_ChannelNum;       //!< 本UDP包有效通道总数
     uint8_t  UNIXTimeStamp[8];     //!< Unix时间戳（未用）
     uint8_t  ReservedNum[4];       //!< 保留数
} UDPHeader_t;

/*!
 *  @def    UDP数据通道 数据帧数据域结构体 - 一个通道组（8通道）
 *
 *  @brief  本结构体定义UDP数据通道 数据帧数据域格式
 */
typedef struct
{
    uint8_t     FrameHeader;                        //!< 起始分隔符
    uint8_t     Index[2];                           //!< 样本序号
    uint8_t     Timestamp[4];                       //!< 本样本时间戳
    uint8_t     ChannelVal[UDP_SampleValSize];      //!< 8通道状态+量化值
}UDPData_t;

/*!
 *  @def    UDP数据通道发送缓冲区 联合体
 *
 *  @brief  本联合体定义UDP数据通道的发送缓冲区帧格式
 */
typedef //union
//{
   //uint8_t UDP_Tx_Buff[UDP_Tx_Buff_Size];
   struct
   {
       /* 数据帧头部     - 23字节*/
       UDPHeader_t sampleheader;

       /*数据帧数据域  */
       UDPData_t sampledata[UDP_SAMPLENUM];

   //} UDPframe;
} UDPFrame_t;

/**********************************************************************
 * FUNCTIONS
 */
void TCP_ProcessFSMInit(void);
bool TCP_ProcessFSM(uint8_t *pdata);

bool UDP_DataGet(uint8_t SampleIndex,uint8_t Procesflag);
bool UDP_DataProcess(SampleTime_t *pSampleTime,uint8_t Procesflag);

bool protocol_RegisterAttrCBs(AttrCBs_t *pAttrcallbacks);

#endif  /* __PROTOCOL_WIFI_H__ */
