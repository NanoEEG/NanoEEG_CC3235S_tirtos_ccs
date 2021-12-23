/**
 * @file    control_task.c
 * @author  gjmsilly
 * @brief   NanoEEG 控制线程，处理属性值变化
 * @version 1.0.0
 * @date    2021-12-23
 *
 * @copyright (c) 2021 gjmsilly
 *
 */

/*********************************************************************
 * INCLUDES
 */
#include <stdbool.h>

#include <ti/display/Display.h>

/* POSIX Header files */
#include <mqueue.h>

#include <service/ads1299.h>
#include <service/timestamp.h>
#include <attr/attrTbl.h>
#include <task/sample_task.h>

/*********************************************************************
 * GLOBAL VARIABLES
 */
mqd_t controlMQueue;                    //!< 消息队列

/*********************************************************************
 *  EXTERNAL VARIABLES
 */
extern SampleTime_t *pSampleTime;
extern uint8_t eegSamplingState;
extern Display_Handle display;

/*********************************************************************
 *  Callbacks
 */

/*!
    \brief          Attr_ChangeCBs

    Callback from Attribute Service indicating a attribute value change.
    本回调函数由control_task注册给属性层，当属性层的属性值被上位机修改时会触发此回调函数。
    本函数通过向消息队列写入变化的属性编号通知control_task处理。

    \param          paramId - parameter Id of the value that was changed

    \return         void

*/
static void Attr_ChangeCBs(uint8_t AttrNum)
{
    int32_t msgqRetVal;
    uint8_t AttrChangeNum = AttrNum;

    msgqRetVal = mq_send(controlMQueue, (char *)&AttrChangeNum, 1, 0);
}



/*********************************************************************
 * FUNCTIONS
 */

/*!
    \brief  AttrChangeProcess

    \param  AttrChangeNum - 变化的属性值编号

    \return true    处理完成
            false   处理异常
 */
static bool AttrChangeProcess (uint8_t AttrChangeNum)
{
    bool ret = true;

    static uint32_t attrvalue=0;            //!< 属性值
    static uint32_t *pValue =&attrvalue;    //!< 属性值指针

    switch(AttrChangeNum)
    {
        case SAMPLING:
            App_GetAttr(SAMPLING,pValue); //!< 获取属性值

            if(*(uint8_t*)pValue == SAMPLE_START )
            {
                //!< 开始计时
                if (Timer_start(pSampleTime->SampleTimer) == Timer_STATUS_ERROR) {
                    /* Failed to start timer */
                    while (1) {}
                }
                eegSamplingState = 0; //复位
                eegSamplingState |= EEG_DATA_START_EVT; //!< 标识采样状态: 开始采样

                /* ads1299 开始采集 */
                ADS1299_Sampling_Control(1);

            }else
            {
                Timer_stop(pSampleTime->SampleTimer); //!< 停止计时

                eegSamplingState |= EEG_STOP_EVT; //!< 标识采样状态

                /* ads1299 停止采集 */
                ADS1299_Sampling_Control(0);

            }
        break;

        case CURSAMPLERATE:
            App_GetAttr(CURSAMPLERATE,pValue); //获取属性值

            if(!ADS1299_SetSamplerate(0,*pValue)){
                //TODO led
            }

        break;

        case CURGAIN:
            App_GetAttr(CURGAIN,pValue); //获取属性值
            if(!ADS1299_SetGain(0,*(uint8_t*)pValue)){
                //TODO led
            }
        break;
    }
   return ret;
}

/*!
    \brief          Control task
                    This task handles the Attribute change event

    \param          None

    \return         void

*/
void controlTask(uint32_t arg0, uint32_t arg1)
{
    uint8_t queueMsg;
    int32_t retVal;
    mq_attr attr;

    /* Register callback with Attribute Service */
    AttrTbl_RegisterAppCBs(&Attr_ChangeCBs);

    /* initializes message queue for attribute value changed messages */
    attr.mq_maxmsg = 10;    // queue size
    attr.mq_msgsize = sizeof(uint8_t);  // Size of message
    controlMQueue = mq_open("control msg q", O_CREAT, 0, &attr);
    if(((int)controlMQueue) <= 0)
    {
        Display_printf(display, 0, 0,"[Control task] Could not create msg queue!\r\n");
        while(1);
    }else {
        Display_printf(display, 0, 0,"Control task ready \r\n");
    }

    while(1)
    {
        /* wait the message enqueue */
        retVal = mq_receive(controlMQueue, (char *)&queueMsg,
                            sizeof(uint8_t),
                            NULL);
        if(retVal>0)
        {
            /* 属性值变化处理 */
            AttrChangeProcess(queueMsg);
            Display_printf(display, 0, 0,"[Control task] Attr %2x Value Changed.\r\n",queueMsg);

        }
    }
}
