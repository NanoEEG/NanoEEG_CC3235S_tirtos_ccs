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
#include <ti/display/Display.h>

#include "evtdata_protocol.h"

/*********************************************************************
 *  GLOBAL VARIABLES
 */
UDPEvtFrame_t UDP_EvtTX_Buff;                //!< UDP发送缓冲区

/*********************************************************************
 *  LOCAL FUNCTIONS
 */

/*!
    \brief  UDP_DataFrameHeaderGet

    事件标签数据通道 数据帧头部静态变量获取
    只在第一次帧头封包时调用本函数,帧头部的静态变量在采集过程中不需要更新。
 */
static void UDP_DataFrameHeaderGet()
{

    extern SlDeviceVersion_t ver;

    /* 设备ID */
    UDP_EvtTX_Buff.Evtheader.DevID = ver.ChipId;

}

/*********************************************************************
 *  FUNCTIONS
 */
void UDP_DataProcess(uint32_t RecvTimestamp,uint16_t delay, uint8_t type){

    UDP_DataFrameHeaderGet();

    UDP_EvtTX_Buff.Evtdata.evtType = type;
    UDP_EvtTX_Buff.Evtdata.Timestamp = RecvTimestamp - delay;

}




