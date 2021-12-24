#ifndef __EVTDATA_PROTOCOL_H
#define __EVTDATA_PROTOCOL_H

/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include <stdint.h>
#include <service/timestamp.h>

/*********************************************************************
 * Macros
 */
#define UDP_EvtTx_Buff_Size           17     //!< 设备ID(4)+UNIX时间戳(8)+精密时间戳(4)+标签类型(1)


/*********************************************************************
 * TYPEDEFS
 */

/*!
    \brief    UDP事件标签通道 帧头数据结构体

    [DANFER]  本结构体内数据需通过属性表获取
 */
#pragma pack(push)
#pragma pack(1)

 typedef struct
{
     uint32_t DevID;                //!< 设备ID
     uint8_t  UNIXTimeStamp[8];     //!< Unix时间戳（未用）
} UDPHeader_t;

/*!
    \brief  UDP事件标签通道 数据帧数据域结构体
 */
#pragma pack(push)
#pragma pack(1)

typedef struct
{
    uint32_t    Timestamp;          //!< 精密时间戳
    uint8_t     evtType;            //!< 事件类型
}UDPData_t;

/*!
    \brief    UDP数据通道发送缓冲区 联合体
              本联合体定义UDP脑电数据通道的发送缓冲区帧格式
 */
typedef struct
{
       /* 数据帧头部 */
       UDPHeader_t  Evtheader;

       /* 数据帧数据域 */
       UDPData_t    Evtdata;

} UDPEvtFrame_t;

/**********************************************************************
 * FUNCTIONS
 */
void UDP_DataFrameHeaderGet();

#endif  /* __EVTDATA_PROTOCOL_H */
