/**
 * @file    attrTbl.c
 * @author  gjmsilly
 * @brief   NanoEEG 属性表
 * @version 1.0.0
 * @date    2020-09-01
 *
 * @copyright (c) 2020 gjmsilly
 *
 */

/***********************************************************************
 * INCLUDES
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "attrTbl.h"
#include <protocol/attr_protocol.h>
#include <protocol/eegdata_protocol.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>

/***********************************************************************
 * LOCAL VARIABLES
 */

//!< 属性总表
static uint8_t* pattr_offset[ATTR_NUM];     //!< 属性偏移地址

/* 基本信息 */
const   uint8_t dev_chnum = CHANNEL_NUM;
SlDeviceVersion_t ver= {0};

/* 采样状态与控制 */
static bool     sampling;
static bool     impMeas;
static uint8_t  impMeas_mode;
static float    impMeasval[16]; //TODO 读取该属性有bug会死机 尝试增大TCP_Tx_Buff_Size

/* 通信参数 */
NETParam_t netparam;
uint8_t samplenum = UDP_SAMPLENUM;

/* 采样参数 */
static uint16_t curSamprate = SPS_250;
static const uint16_t samplerate_tbl[]={SPS_250,SPS_500,SPS_1K,SPS_2K};
static uint8_t curGain = GAIN_X24;
static const uint8_t gain_tbl[]={GAIN_X1,GAIN_X2,GAIN_X4,GAIN_X6,GAIN_X8,GAIN_X24};

/* 事件触发 */
static uint16_t trig_delay = 1000; //TODO us为单位

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
                            1,
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
                            2,
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
                            (uint32_t*)&curGain
                            },     

   /*  ======================== 事件触发 ============================== */
        
        //!< 外触发信号延迟时间 us
        .Trig_delay     = { ATTR_RW,
                            ATTR_CONFIG,
                            2,
                            (uint32_t*)&trig_delay
                            },                           
};


/************************************************************************
 *  Callbacks
 */

static pfnAttrChangeCB_t pAppCallbacks; //!< 应用层回调函数指针

/*!
    \brief    Attr_Tbl_RegisterAppCBs
    
    应用层注册回调函数的接口，当属性值被上位机修改成功后，属性层告知上位机变化的属性编号
  
    \param  appcallbacks - 不指定函数类型
  
    \return true - 回调函数注册成功
            false - 回调函数注册失败
 */
bool AttrTbl_RegisterAppCBs(void *appcallbacks)
{
    if ( appcallbacks )
  {
        pAppCallbacks = appcallbacks;
    return ( true );
    }
    else
    {
      return ( false );
    }
}
//////////////////////////////////////////////////////////////////////////

/* Attribute table callbacks */
static uint8_t ReadAttrCB(  uint8_t InsAttrNum,
                            uint8_t CHxNum,
                            uint8_t *pValue,
                            uint8_t *pLen );

static uint8_t WriteAttrCB( uint8_t InsAttrNum,
                            uint8_t CHxNum,
                            uint8_t *pValue,
                            uint8_t len );

static AttrCBs_t attr_CBs =
{
    .pfnReadAttrCB = ReadAttrCB,                    //!< 读属性回调函数指针
    .pfnWriteAttrCB = WriteAttrCB                   //!< 写属性回调函数指针
};


/*!
    \brief  ReadAttrCB      读属性回调函数
            
    \param  InsAttrNum      待读属性编号
            CHxNum          通道编号（通道属性专用，默认不用   0xFF）
            pValue          属性值 （to be returned）
            pLen            属性值大小（to be returned）
            
    \return ATTR_SUCCESS    读取属性值成功
            ATTR_NOT_FOUND  属性不存在
 */
static uint8_t ReadAttrCB(  uint8_t InsAttrNum,uint8_t CHxNum,
                            uint8_t *pValue, uint8_t *pLen )
{
    uint8_t status = ATTR_SUCCESS;
    uint8_t *pAttrValue;    //!< 属性值地址

    if( (InsAttrNum > ATTR_NUM ) && ( CHxNum == 0xFF ))
    {
        status = ATTR_NOT_FOUND; //!< 属性不存在
    }

    //!< 读属性值
    if(status == ATTR_SUCCESS)
    {
        pAttrValue = (uint8_t*)*(uint32_t*)(pattr_offset[InsAttrNum]+3);//!< 属性值地址传递
        *pLen = *(pattr_offset[InsAttrNum]+2); //!< 属性值大小传递（值传递!地址不变 9.13）
        memcpy(pValue,pAttrValue,*pLen); //!< 属性值读取
    }

    return status;

}

/*!
    \brief  WriteAttrCB     写属性回调函数
  
    \param  InsAttrNum      待写入属性编号
            CHxNum          通道编号（通道属性专用，默认不用   0xFF）
            pValue          待写入数据的指针
            pLen            待写入数据大小
            
    \return ATTR_SUCCESS    读取属性值成功
            ATTR_NOT_FOUND  属性不存在
            ATTR_ERR_RO     属性不允许写操作
            ATTR_ERR_SIZE   待写数据长度与属性值长度不符
 */
static uint8_t WriteAttrCB( uint8_t InsAttrNum,uint8_t CHxNum,
                            uint8_t *pValue, uint8_t len )
{
    uint8_t status = ATTR_SUCCESS;
    uint8_t notifyApp=0xFF; //!< 标志位 - 通知上层应用程序属性值变化
    uint8_t AttrPermission; //!< 属性读写权限
    uint8_t AttrType;       //!< 属性类型
    uint8_t AttrLen;        //!< 属性值大小
    uint8_t *pAttrValue;    //!< 属性值地址

    AttrPermission = *(pattr_offset[InsAttrNum]);
    AttrType = *(pattr_offset[InsAttrNum]+1);
    AttrLen = *(pattr_offset[InsAttrNum]+2);

    if( (InsAttrNum > ATTR_NUM ) && ( CHxNum == 0xFF ))
    {
        status = ATTR_NOT_FOUND; //!< 属性不存在
    }
    else if( AttrPermission == ATTR_RO )
    {
        status = ATTR_ERR_RO; //!< 属性不允许写操作
    }
    else if( len!= AttrLen )
    {
        status = ATTR_ERR_SIZE; //!< 待写数据长度与属性值长度不符
    }
    else status = ATTR_SUCCESS;

    //!< 根据写属性类型校验写入数据有效性
    // TODO 写属性类型是配置类型时，需要检查该值是否是配置类型有效值，checkValid();

    //!< 写属性值并通知应用层（AttrChange_Process）
    if(status == ATTR_SUCCESS)
    {
        pAttrValue = (uint8_t*)*(uint32_t*)(pattr_offset[InsAttrNum]+3);//!< 属性值地址传递

        memcpy(pAttrValue,pValue,len); //!< 属性值写入
        notifyApp=InsAttrNum;
    }

    if( (notifyApp!=0xFF) && pAppCallbacks )
    {
        (*pAppCallbacks)(notifyApp);
    }

    return status;

}


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

    /* 向控制通道协议层 注册属性值读写回调函数 */
    protocol_RegisterAttrCBs(&attr_CBs);

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


/*!
    \brief  读属性函数 （供应用层获取属性）

    \param  InsAttrNum - 待写入属性编号
            pValue - 待写入数据的指针

    \return true 读取属性值成功
            //TODO 纠错机制
 */
uint8_t App_GetAttr(uint8_t InsAttrNum, uint32_t *pValue)
{
    uint8_t ret = true;

    switch(InsAttrNum)
    {
        case SAMPLING:
            memcpy(pValue,&sampling,1);
            break;

        case CURSAMPLERATE:
            memcpy(pValue,&curSamprate,4);
            break;

        case CURGAIN:
            memcpy(pValue,&curGain,4);
            break;
    }
  return ( ret );
}

/*!
    \brief  写属性函数 （供应用层修改属性值）

    \param  InsAttrNum - 待写入属性编号
            Value - 待写入数据

    \return true 写属性值成功
            //TODO 纠错机制
 */
uint8_t App_WriteAttr(uint8_t InsAttrNum, uint8_t Value)
{
    uint8_t ret = true;

    switch(InsAttrNum)
    {
        case SAMPLING:
            sampling=Value; //TODO 开关类型属性需要状态机实现
            break;
    }
  return ( ret );
}
