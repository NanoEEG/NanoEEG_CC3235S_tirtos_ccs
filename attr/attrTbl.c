/**
 * @file    attrTbl.c
 * @author  gjmsilly
 * @brief   CC3235S 属性表
 * @version 1.0.0
 * @date    2020-09-01
 *
 * @copyright (c) 2020 gjmsilly
 *
 */

/***********************************************************************
 * INCLUDES
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "attrTbl.h"
#include <protocol/eegdata_protocol.h>

/***********************************************************************
 * LOCAL VARIABLES
 */

//!< 属性总表
static uint8_t* pattr_offset[ATTR_NUM];     //!< 属性偏移地址

/* 基本信息 */
const   uint16_t channelnum = CHANNEL_NUM;
SlDeviceVersion_t ver= {0};

/* 采样状态与控制 */
static bool     sampling;
static bool     impMeas;
static uint8_t  impMeas_mode;
static float    impMeasval[CHANNEL_NUM]; //TODO 

/* 通信参数 */
NETParam_t networkparam;
uint8_t samplenum = UDP_SAMPLENUM;

/* 采样参数 */
static uint32_t curSamprate = SPS_250;
static const uint32_t samplerate_tbl[]={SPS_250,SPS_500,SPS_1K,SPS_2K,SPS_4K};
static uint32_t cur_gain = GAIN_X24;
static const uint32_t gain_tbl[]={GAIN_X1,GAIN_X2,GAIN_X4,,GAIN_X6,GAIN_X8,GAIN_X24};

/* 事件触发 */

/************************************************************************
 *  Attribute  Table
 */
const AttrTbl_t attr_tbl = {
    
    /*  ======================== 基本信息 ============================== */

        //!< 仪器UID
        .Dev_UID        = { ATTR_RO,                                    /* permissions */
                            ATTR_CONFIG,                                /* type */
                            4,                                          /* datasize */
                            (uint32_t*)&ver.ChipId                      /* pAttrValue */
                           },

        //!< 仪器总通道数
        .Dev_ChNum      = { ATTR_RO, 
                            ATTR_CONFIG,
                            2,
                            (uint32_t*)&dev_chnum
                           },

    /*  ====================== 采样状态与控制 =========================== */

        //!< 采样开关 0-停止采样 1-开始采样
        .Sampling       = { ATTR_RW,
                            ATTR_SW,      
                            1,
                            (uint32_t*)&sampling
                          },

        //!< 阻抗测量开关 0-无阻抗测量 1-阻抗测量
        .IMPMeas        = { ATTR_RW,
                            ATTR_SW,
                            1,
                            (uint32_t*)&impMeas
                           },

        //!< 阻抗测量方案 0- 正弦波测AC电阻 1- 测DC电阻 2- 交流激励测阻抗
        .IMPMeas_MODE   = { ATTR_RW,
                            ATTR_CONFIG,
                            1,
                            (uint32_t*)&impMeas_mode
                           },

        //!< 逐通道阻抗值 
        .IMPValue       = { ATTR_RW,
                            ATTR_MSG,
                            sizeof(impMeasval), //TODO
                            (uint32_t*)impMeasval
                           },

    /*  ======================== 通信参数 ============================== */

        //!< 仪器网口MAC地址
        .Dev_MAC         = { ATTR_RO,
                             ATTR_CONFIG,        
                             6,
                             (uint32_t*)&netparam.MAC_Addr,
                            },

        //!< 仪器当前IP地址
        .Dev_IP         = { ATTR_RO,
                            ATTR_CONFIG,
                            4,
                            (uint32_t*)&netparam.IP_Addr,
                            },

        //!<  EEG数据通道每包含AD样本数
        .SampleNum      = { ATTR_RO,
                            ATTR_CONFIG,
                            1,
                            (uint32_t*)&samplenum,
                            },

        //!< EEG数据通道端口
        .EEGDataPort    = { ATTR_RO,
                            ATTR_CONFIG,
                            2,
                            (uint32_t*)&netparam.EEGdataPort,
                            },

        //!< 事件标签通道端口
        .EventDataPort  = { ATTR_RO,
                            ATTR_CONFIG,
                            2,
                            (uint32_t*)&netparam.EvtdataPort,
                            },
    
    /*  ======================== 采样参数 ============================== */                            

        //!< 支持的采样率挡位
        .Samplerate_tbl = { ATTR_RO,
                            ATTR_CONFIG,
                            sizeof(samplerate_tbl),
                            (uint32_t*)samplerate_tbl
                            },

        //!< 当前全局采样率
        .CurSamplerate  = { ATTR_RW,
                            ATTR_CONFIG,
                            1,
                            (uint32_t*)&curSamprate
                            },

        //!< 支持的增益挡位
        .Gain_tbl       = { ATTR_RO,
                            ATTR_CONFIG,
                            sizeof(gain_tbl),
                            (uint32_t*)gain_tbl
                            },

        //!< 当前全局增益
        .CurGain        = { ATTR_RW,
                            ATTR_CONFIG,
                            1,
                            (uint32_t*)&cur_gain
                            },     

   /*  ======================== 事件触发 ============================== */    
        
        //!< 外触发信号延迟时间
        .Trig_delay     = { ATTR_RW,
                            ATTR_CONFIG,
                            1,
                            (uint32_t*)&trig_delay
                            },                           
};

/************************************************************************
 * FUNCTIONS
 */

/*!
    \brief      AttrTbl_Init
    
    This function is to create the attribute table.

    \param      void

    \return     void
*/
void AttrTbl_Init()
{

    /* 建立地址映射 */

    //!< 属性地址偏移映射关系
    //!< pattr_offset[n]即该属性的物理首地址 上位机通过下标进行偏移访问
    pattr_offset[DEV_UID] = (uint8_t*)&attr_tbl.Dev_UID.permissions;
    pattr_offset[DEV_CHANNEL_NUM] = (uint8_t*)&attr_tbl.Dev_ChNum.permissions;
    pattr_offset[SAMPLING] = (uint8_t*)&attr_tbl.Sampling.permissions;

    pattr_offset[IMPMEAS] = (uint8_t*)&attr_tbl.IMPMeas.permissions;
    pattr_offset[IMPMEAS_MODE] = (uint8_t*)&attr_tbl.IMPMeas_MODE.permissions;
    pattr_offset[IMPVAULE] = (uint8_t*)&attr_tbl.IMPValue.permissions;

    pattr_offset[DEV_MAC] = (uint8_t*)&attr_tbl.Dev_MAC.permissions;
    pattr_offset[DEV_IP] = (uint8_t*)&attr_tbl.Dev_IP.permissions;
    pattr_offset[SAMPLE_NUM] = (uint8_t*)&attr_tbl.SampleNum.permissions;
    
    pattr_offset[EEGDATAPORT] = (uint8_t*)&attr_tbl.EEGDataPort.permissions;
    pattr_offset[EVTDATAPORT] = (uint8_t*)&attr_tbl.EventDataPort.permissions;

    pattr_offset[SAMPLERATE_TBL] = (uint8_t*)&attr_tbl.Samplerate_tbl.permissions;
    pattr_offset[CURSAMPLERATE] = (uint8_t*)&attr_tbl.CurSamplerate.permissions;
    pattr_offset[GAIN_TBL] = (uint8_t*)&attr_tbl.Gain_tbl.permissions;
    pattr_offset[CURGAIN] = (uint8_t*)&attr_tbl.CurGain.permissions;
    
    pattr_offset[TRIGDELAY] = (uint8_t*)&attr_tbl.Trig_delay.permissions;
}
