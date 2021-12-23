/**
 * @file    attrTbl.h
 * @author  gjmsilly
 * @brief   CC3235S attrtbl
 * @version 1.0.0
 * @date    2020-09-01
 *
 * @copyright (c) 2020 gjmsilly
 *
 */

#ifndef __ATTRTBL_H
#define __ATTRTBL_H

#include <stdbool.h>

/*******************************************************************
 * CONSTANTS
 */
#define ATTR_NUM                        22      //!< 属性表支持的属性数量（除通道属性）

/* 属性权限 */
#define ATTR_RO                         0x00    //!< 只读属性
#define ATTR_RW                         0x01    //!< 读写属性，可读可写

/* 属性类型 */
#define ATTR_SW                         0x00    //!< 开关类型属性
#define ATTR_CONFIG                     0x01    //!< 配置类型属性
#define ATTR_MSG                        0x02    //!< 消息类型属性

/* 属性编号 */
#define DEV_UID                         0
#define DEV_CHANNEL_NUM                 1
#define SAMPLING                        2
#define IMPMEAS                         3
#define IMPMEAS_MODE                    4
#define IMPVAULE                        5
#define DEV_MAC                         6
#define DEV_IP                          7
#define SAMPLE_NUM                      8
#define EEGDATAPORT                     9
#define EVTDATAPORT                     10
#define SAMPLERATE_TBL                  11
#define CURSAMPLERATE                   12
#define GAIN_TBL                        13
#define CURGAIN                         14
#define TRIGDELAY                       15

/* 属性值定义 */

#define SAMPLE_START                    1           //!< 开始采集
#define SAMPLE_STOP                     0           //!< 停止采集

#define IMPMES_START                    1           //!< 开始阻抗检测
#define IMPMES_STOP                     0           //!< 停止阻抗检测

/*******************************************************************
 * TYPEDEFS
 */

/*!
 *  @def    Attr_t
 *  @brief  属性 结构体
 */
#pragma pack(push)
#pragma pack(1)
typedef struct
{
    uint8_t         permissions;        //!< 属性权限 - 读写允许
    uint8_t         type;               //!< 属性类型 - 开关/配置/消息
    uint8_t         Attrsize;           //!< 属性长度 - 以字节为单位
    uint32_t* const pAttrValue;         //!< 属性值地址
} Attr_t;

/*!
 *  @def    AttrTbl_t
 *  @brief  属性表 结构体（紧凑结构确保连续内存，供上位机地址偏移访问）
 */
#pragma pack(push)
#pragma pack(1)
typedef struct
{
    /* 基本信息 */
    Attr_t  Dev_UID;                //!< 仪器UID
    Attr_t  Dev_ChNum;              //!< 仪器总通道数

    /* 采样状态与控制 */
    Attr_t  Sampling;               //!< 采样开关
    Attr_t  IMPMeas;                //!< 阻抗测量开关
    Attr_t  IMPMeas_MODE;           //!< 阻抗测量方案
    Attr_t  IMPValue;               //!< 逐通道阻抗值

    /* 采样参数 */
    Attr_t  Samplerate_tbl;         //!< 支持的采样率挡位
    Attr_t  CurSamplerate;          //!< 当前全局采样率
    Attr_t  Gain_tbl;               //!< 支持的增益挡位
    Attr_t  CurGain;                //!< 当前全局增益

    /* 事件触发 */
    Attr_t  Trig_en;                //!< 外触发允许
    Attr_t  Trig_delay;             //!< 外触发信号延迟时间

    /* 通信参数 */
    Attr_t  Dev_MAC;                //!< 仪器MAC地址
    Attr_t  Dev_IP;                 //!< 仪器当前IP地址
    Attr_t  SampleNum;              //!< 每包含AD样本数
    Attr_t  EEGDataPort;            //!< EEG数据通道端口
    Attr_t  EventDataPort;          //!< 事件标签通道端口

    /* 功耗电量 */
    Attr_t  Dev_BAT_vol;            //!< 仪器电池电压
    Attr_t  Dev_charg;              //!< 仪器充电状态
    Attr_t  Dev_BAT_pct;            //!< 仪器电池电量百分比
    Attr_t  Dev_LPC;                //!< 仪器光污染控制
    
}AttrTbl_t;

/*!
 *  @brief  支持的分档采样率表
 */
typedef enum
{
    SPS_250 = 250,
    SPS_500 = 500,
    SPS_1K = 1000,
    SPS_2K = 2000,
    SPS_4K = 4000,
}Samplerate_tbl_t;

/*!
 *  @brief  支持的分档增益表
 */
typedef enum
{
    GAIN_X1 = 1,
    GAIN_X2 = 2,
    GAIN_X4 = 4,
    GAIN_X6 = 6,
    GAIN_X8 = 8,
    GAIN_X24 = 24
}Gain_tbl_t;

/*!
 *  @brief      仪器网络参数状态
 */
typedef struct
{
    uint32_t IP_Addr;           //!< 本机IP地址
    uint8_t  MAC_Addr[6];       //!< 本机MAC地址
    uint16_t EEGdataPort;       //!< EEG数据通道端口
    uint16_t EvtdataPort;       //!< 事件标签通道端口
    
}NETParam_t;



/*!
    \brief  属性值变化回调函数原型
    \param  AttrNum - 值变化的属性

 */
typedef void (*pfnAttrChangeCB_t)( uint8_t AttrNum );


/*********************************************************************
 * FUNCTIONS
 */
void AttrTbl_Init();
bool AttrTbl_RegisterAppCBs(void *appCallbacks);
uint8_t App_GetAttr(uint8_t InsAttrNum, uint32_t *pValue);
uint8_t App_WriteAttr(uint8_t InsAttrNum, uint8_t Value);

#endif /* __ATTRTBL_H */
