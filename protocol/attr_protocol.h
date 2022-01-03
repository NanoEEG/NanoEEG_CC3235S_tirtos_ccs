#ifndef __ATTR_PROTOCOL_H__
#define __ATTR_PROTOCOL_H__

/*********************************************************************
 * INCLUDES
 */
#include <stdbool.h>

/*********************************************************************
 * MARCOS
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
#define ChxAttr_Read                0x02    //!< 读一个通道属性    //TODO
#define ChxAttr_Write               0x20    //!< 写一个通道属性    //TODO
// 错误码
#define ATTR_SUCCESS                0x00    //!< 属性读写正常
#define ATTR_ERR_RO                 0x01    //!< 属性不允许写操作
#define ATTR_ERR_SIZE               0x02    //!< 待写数据长度与属性值长度不符
#define ATTR_NOT_FOUND              0x03    //!< 待读写的属性不存在
#define ATTR_VAL_INVALID            0x04    //!< 待读写的属性值非法  //TODO

// 通讯收发缓冲区参数
#define TCP_Rx_Buff_Size            16
#define TCP_Tx_Buff_Size            128     //!< try to fix：考虑阻抗值

/*******************************************************************
 * TYPEDEFS
 */

/*!
    /brief  声明 读属性回调函数原型
    /param  InsAttrNum - 待读属性编号
            CHxNum - 通道编号（本版本不涉及 0xFF //TODO）
            pValue - 属性值 （to be returned）
            pLen - 属性值大小（to be returned）
 */
typedef uint8_t (*pfnReadAttrCB_t)( uint8_t InsAttrNum, uint8_t CHxNum,
                                    uint8_t *pValue, uint8_t *pLen );

/*!
    /brief  声明 写属性回调函数原型
    /param  InsAttrNum - 待写入属性编号
            CHxNum - 通道编号（本版本不涉及 0xFF //TODO）
            pValue - 待写入数据的指针
            pLen - 待写入数据大小
 */
typedef uint8_t (*pfnWriteAttrCB_t)( uint8_t InsAttrNum, uint8_t CHxNum,
                                     uint8_t *pValue, uint8_t len );

/*!
    /brief  属性读写回调函数 结构体
 */
typedef struct
{
  pfnReadAttrCB_t   pfnReadAttrCB;                  //!< 读属性回调函数指针
  pfnWriteAttrCB_t  pfnWriteAttrCB;                 //!< 写属性回调函数指针
} AttrCBs_t;

/*!
    /brief  TCP控制通道帧 结构体
            本结构体定义TCP控制通道一次帧收发的数据
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

/**********************************************************************
 * FUNCTIONS
 */
void TCP_ProcessFSMInit(void);
bool TCP_ProcessFSM(uint8_t *pdata);
bool protocol_RegisterAttrCBs(AttrCBs_t *pAttrcallbacks);

#endif  /* __ATTR_PROTOCOL_H__ */
