/**
 * @file    attr_protocol.c
 * @author  gjmsilly
 * @brief   NanoEEG 控制通道帧服务
 * @version 1.0.0
 * @date    2020-12-27
 * @ref     FSM框架 - https://github.com/misje/stateMachine
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

#include "attr_protocol.h"
#include <utility/stateMachine.h>

/*********************************************************************
 *  LOCAL VARIABLES
 */
/* 控制通道变量 */
static TCPFrame_t tcpframe;             //!< FSM存储一帧数据
static bool fsmFinalState=false;        //!< 标识FSM退出状态

/*********************************************************************
 *  GLOBAL VARIABLES
 */
extern Display_Handle display;

uint8_t TCP_Tx_Buff[TCP_Tx_Buff_Size];      //!< TCP发送缓冲区
uint8_t TCP_Rx_Buff[TCP_Rx_Buff_Size];      //!< TCP接收缓冲区
uint8_t *pTCP_Tx_Buff=TCP_Tx_Buff;
uint8_t *pTCP_Rx_Buff=TCP_Rx_Buff;

/*********************************************************************
 *  Callback
 */

static AttrCBs_t *pattr_CBs = NULL; //!< 属性表服务回调指针

/*!
    \brief      protocol_RegisterAttrCBs

    本函数是协议层提供给属性层注册属性值读写回调的回调注册函数

    \param      pAttrcallbacks - 协议层定义的属性值读写回调指针

    \return     true - 注册成功
                false - 注册失败
*/
bool protocol_RegisterAttrCBs(AttrCBs_t *pAttrcallbacks)
{
  if ( pAttrcallbacks )
  {
        pattr_CBs = pAttrcallbacks;

        return true;
  }
  else
    {
         return false;
    }
}

/*
 *  ======================== TCP控制通道帧协议 =============================
 */

/* Types of events */
enum eventType
{
    Event_TCPFRAME,
};

static bool FrameSeekHead(void *condition, struct event *event);
static bool FrameCheck(void *condition, struct event *event);
static bool FrameIns(void *condition, struct event *event);
static bool FrameReply(void *condition, struct event *event);
static void printErrMsg( void *stateData, struct event *event );
static void printExitMsg( void *stateData, struct event *event );

/* States in order */
static struct state frameProcessGroup,frame_seekhead, frame_chk, frame_Ins, frame_rpy;

/* 父状态 */
static struct state frameProcessGroup = {
   .parentState = NULL,
   .entryState = &frame_seekhead,

   .transitions = (struct transition[])
   {
       {
          .eventType = Event_TCPFRAME,
          .condition = NULL,
          .guard = NULL,
          .action = NULL,
          .nextState = &frame_seekhead
       },
   },
   .numTransitions = 1,

   .data = "group",
   .entryAction = NULL,
   .exitAction = &printExitMsg,
};

/* 状态1: 帧头检测 */
static struct state frame_seekhead = {
   .parentState = &frameProcessGroup,
   .entryState = NULL,

   .transitions = (struct transition[])
   {
       {
          .eventType = Event_TCPFRAME,
          .condition = (void *)(uint8_t)TCP_Recv_FH,
          .guard = &FrameSeekHead,
          .action = NULL,
          .nextState = &frame_chk
       },
   },
   .numTransitions = 1,

   .data = "frame_seekhead",
   .entryAction = NULL,
   .exitAction = &printExitMsg,
};

/* 状态2: 帧完整性检测 */
static struct state frame_chk = {
   .parentState = &frameProcessGroup,
   .entryState = NULL,

   .transitions = (struct transition[])
   {
       {
          .eventType = Event_TCPFRAME,
          .condition = (void *)(uint8_t)TCP_Recv_FT,
          .guard = &FrameCheck,
          .action = NULL,
          .nextState = &frame_Ins
       },
   },
   .numTransitions = 1,

   .data = "frame_chk",
   .entryAction = NULL,
   .exitAction = &printExitMsg,
};

/* 状态3: 帧解析 */
static struct state frame_Ins = {
   .parentState = &frameProcessGroup,
   .entryState = NULL,

   .transitions = (struct transition[])
   {
       {
          .eventType = Event_TCPFRAME,
          .condition = NULL,
          .guard = &FrameIns,
          .action = NULL,
          .nextState = &frame_rpy
       },
   },
   .numTransitions = 1,

   .data = "frame_Ins",
   .entryAction = NULL,
   .exitAction = &printExitMsg,
};

/* 状态4: 帧回复 */
static struct state frame_rpy = {
   .parentState = &frameProcessGroup,
   .entryState = NULL,

   .transitions = (struct transition[])
   {
       {
          .eventType = Event_TCPFRAME,
          .condition = NULL,
          .guard = &FrameReply,
          .action = NULL,
          .nextState = &frame_seekhead
       },
   },
   .numTransitions = 1,

   .data = "frame_rpy",
   .entryAction = NULL,
   .exitAction = &printExitMsg,
};

/* 状态机错误
 * @warning:受限于框架设计，本状态只由程序未写引起，
 *          帧错误的报错程序在各个状态中体现*/
static struct state falseState = {
   .entryAction = &printErrMsg
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*!
   \brief  FrameSeekHead

   TCP帧头检测帧，由FSM状态1调用，当TCP接收一帧帧头为本协议规定的帧头时返回true，
   FSM进入状态2；否则，持续在状态1直到状态机轮询结束。
 */
static bool FrameSeekHead(void *condition, struct event *event)
{
   //!< 复位
   memset((uint8_t*)&tcpframe,0xff,sizeof(tcpframe));
   fsmFinalState=false;

   tcpframe.FrameHeader = (uint8_t)event->data; //!< 获取帧头

   return (uint8_t)condition == (uint8_t)tcpframe.FrameHeader;
}

/*!
   \brief  FrameSeekHead

   TCP帧完整性检测，由FSM状态2调用，根据TCP接收一帧的第二字节（即有效帧长度）
   判断接收一帧的帧尾是否为协议规定的帧尾，是则返回true，FSM进入状态3；
   否则，报错并退回状态1。
 */
static bool FrameCheck(void *condition, struct event *event)
{

   if ( event->type != Event_TCPFRAME )
      return false;

   tcpframe.FrameLength = (uint8_t)event->data; //!< 获取有效帧长度（除去帧头、帧尾和有效帧长三字节）

   if((uint8_t)condition == (uint8_t)*(pTCP_Rx_Buff+tcpframe.FrameLength+2))//!< 帧尾检测
      return true;
   else
       printErrMsg(frame_chk.data, event);
      return false;

}
/*!
   \brief  FrameSeekHead

   TCP帧解析，由FSM状态3调用，根据TCP接收一帧的第三字节（即指令码）对属性表进行读写操作
   指令码正确则返回true，FSM进入状态4;否则，报错并退回状态1。
 */
static bool FrameIns(void *condition, struct event *event)
{
   bool InsState = true;

   if ( event->type != Event_TCPFRAME )
      return false;

   /* 根据有效帧长判定指令码，对接收帧数据预处理*/
   if(tcpframe.FrameLength ==3)//!< 读属性操作 - 有效帧定长3
   {
       tcpframe.InsAttrNum = *(pTCP_Rx_Buff+3);
       tcpframe.ChxNum = *(pTCP_Rx_Buff+4);
       tcpframe.pDataLength = &tcpframe.DataLength;
   }
   else if(tcpframe.FrameLength == 1); //!< 空指令 - 有效帧定长1
   else //!< 写属性操作 - 有效帧>4 不定长
   {
       tcpframe.DataLength = tcpframe.FrameLength-3;
       tcpframe.InsAttrNum = *(pTCP_Rx_Buff+3);
       tcpframe.ChxNum = *(pTCP_Rx_Buff+4);
       tcpframe._OP_ = pTCP_Rx_Buff+5;
   }

   tcpframe.InsNum = (uint8_t)event->data; //!< 获取指令码

   switch(tcpframe.InsNum)
   {
       case CAttr_Read: //!< 读普通属性
           //!< 读属性回调
           tcpframe.ERR_NUM = pattr_CBs->pfnReadAttrCB( tcpframe.InsAttrNum,tcpframe.ChxNum, \
                                                       (pTCP_Tx_Buff+4),tcpframe.pDataLength);
           tcpframe.FrameLength = *(tcpframe.pDataLength)+2;
           break;

       case CAttr_Write: //!< 写普通属性

           //!< 写属性回调
           tcpframe.ERR_NUM = pattr_CBs->pfnWriteAttrCB( tcpframe.InsAttrNum,tcpframe.ChxNum, \
                                                         tcpframe._OP_,tcpframe.DataLength);

           if( tcpframe.ERR_NUM == true )
           {
               //!< 拷贝一份用于回复
               memcpy((pTCP_Tx_Buff+4),tcpframe._OP_,tcpframe.DataLength);
               tcpframe.FrameLength = tcpframe.DataLength+2;
           }
           //!< 否则以错误码的形式返回 不通过串口返回
           break;

       default:
           printErrMsg(frame_Ins.data, event);
           InsState=false;
           break;
   }

   return InsState ;
}

/*!
   \brief  FrameReply

   TCP帧回复，由FSM状态4调用。
   用于对接收帧的回复，本状态结束则标识状态机解析本帧正常。

 */
static bool FrameReply(void *condition, struct event *event)
{
   /* 打包回复 */
    *(pTCP_Tx_Buff) = TCP_Send_FH; //!< 帧头

   if( tcpframe.ERR_NUM != true )
       tcpframe.FrameLength=2; //!< 有效帧只含错误码+回复类型

   *(pTCP_Tx_Buff+1) = tcpframe.FrameLength;  //!< 有效帧
   *(pTCP_Tx_Buff+2) = tcpframe.ERR_NUM;      //!< 错误码
   *(pTCP_Tx_Buff+3) = tcpframe.InsAttrNum;   //!< 回复类型（属性编号）

   *(pTCP_Tx_Buff+tcpframe.FrameLength+2)=TCP_Send_FT; //!< 帧尾

   fsmFinalState=true; //!< 状态机运行正常

   return true;
}

static void printErrMsg( void *stateData, struct event *event )
{
    Display_printf(display, 0, 0, "false STATE: %s",(char *)stateData);

    fsmFinalState=false; //!< 状态机从错误状态退出
}

static void printExitMsg( void *stateData, struct event *event )
{
    Display_printf(display, 0, 0, "Complete %s state\n", (char *)stateData);

}

/*********************************************************************
 * EXPORTED FUNCTIONS
 */

struct stateMachine TCP_Processfsm;
/*!
    \brief  控制通道帧协议状态机初始化
 */
void TCP_ProcessFSMInit(void)
{

    stateM_init( &TCP_Processfsm, &frame_seekhead, &falseState );
}
/*!
   \brief  控制通道帧协议解析

   \param  pdata - 需要帧解析的数据
 */
bool TCP_ProcessFSM(uint8_t *pdata)
{
    uint8_t stateNum; //!< 状态切换的次数/状态机一次被调用轮询的次数

    /* 受框架机制限制的处理方式 - 最多轮4次完成一次帧解析 */
    for(stateNum=0;stateNum<4;stateNum++)
    {
        stateM_handleEvent(&TCP_Processfsm,
                           &(struct event){ Event_TCPFRAME,(void *)(uint8_t)(*pdata++)});
     }

    return fsmFinalState;
}
