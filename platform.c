/*
 * Copyright (c) 2015-2018, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/********************************************************************************
 *  INCLUDES
 */
// TI-Driver includes
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>

#include <ti/display/Display.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/Timer.h>

#include "ti_drivers_config.h"

// POSIX Header files
#include <mqueue.h>
#include <semaphore.h>
#include "pthread.h"

// User Services & tasks
#include "platform.h"
#include <attr/attrTbl.h>
#include <task/cc1310_Sync.h>
#include <service/timestamp.h>
#include <service/ads1299.h>

/********************************************************************************
 *  GLOBAL VARIABLES
 */
int32_t   mode;

// Thread Object
pthread_t spawn_thread = (pthread_t)NULL;
pthread_t tcpThread = (pthread_t)NULL;
pthread_t tcpworker_thread = (pthread_t)NULL;
pthread_t udp1Thread = (pthread_t)NULL;
pthread_t udp2Thread = (pthread_t)NULL;
pthread_t ControlThread = (pthread_t)NULL;
pthread_t SampleThread = (pthread_t)NULL;


//!< Driver handle
Display_Handle display;

SampleTime_t *pSampleTime = NULL;
sem_t UDPDataReady;
sem_t SampleReady;

/********************************************************************************
 *  EXTERNAL VARIABLES
 */
extern NETParam_t netparam;
extern SlDeviceVersion_t ver;

/********************************************************************************
 *  EXTERNAL FUNCTIONS
 */
extern void tcpHandler(uint32_t arg0, uint32_t arg1);
extern void tcpWorker(uint32_t arg0, uint32_t arg1);
extern void udp1Worker(uint32_t arg0, uint32_t arg1);
//extern void udp2Worker(uint32_t arg0, uint32_t arg1);
extern void SampleTask(void* arg0 , void* arg1);
extern void controlTask(uint32_t arg0, uint32_t arg1);

extern int32_t ti_net_SlNet_initConfig();

static void printError(char *errString, int code);
static void DisplayBanner(char * AppName,char * AppVer);

/********************************************************************************
 *  FUNCTIONS
 */

/********************************************************************************
                          Callback Functions
*********************************************************************************/

/*!
    \brief          SimpleLinkNetAppEventHandler

    This handler gets called whenever a Netapp event is reported
    by the host driver / NWP. Here user can implement he's own logic
    for any of these events. This handler is used by 'network_terminal'
    application to show case the following scenarios:

    1. Handling IPv4 / IPv6 IP address acquisition.
    2. Handling IPv4 / IPv6 IP address Dropping.

    \param          pNetAppEvent     -   pointer to Netapp event data.

    \return         void

    \note           For more information, please refer to: user.h in the porting
                    folder of the host driver and the  CC31xx/CC32xx NWP programmer's
                    guide (SWRU455) section 5.7

*/
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent)
{
    int32_t             status = 0;
    pthread_attr_t      pAttrs;
    struct sched_param  priParam;

    if(pNetAppEvent == NULL)
    {
        return;
    }

    switch(pNetAppEvent->Id)
    {
        case SL_NETAPP_EVENT_IPV4_ACQUIRED:
        case SL_NETAPP_EVENT_IPV6_ACQUIRED:

            /* Initialize SlNetSock layer with CC3x20 interface                      */
            status = ti_net_SlNet_initConfig();
            if(0 != status)
            {
                Display_printf(display, 0, 0, "Failed to initialize SlNetSock\n\r");
            }

            if(mode != ROLE_AP)
            {
                Display_printf(display, 0, 0,"[NETAPP EVENT] IP Acquired: IP=%d.%d.%d.%d , "
                            "Gateway=%d.%d.%d.%d\n\r",
                            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,3),
                            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,2),
                            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,1),
                            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Ip,0),
                            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,3),
                            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,2),
                            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,1),
                            SL_IPV4_BYTE(pNetAppEvent->Data.IpAcquiredV4.Gateway,0));
                
                // update Dev_IP Attr
                netparam.IP_Addr = pNetAppEvent->Data.IpAcquiredV4.Ip;

                /* When router is connected, create 3 threads to handle tcp & udp,
                   2 threads: control_task to handle attr change, sample_task to handle sample */
                
                /*  tcpThread with acess function tcpHadler to deal with client connection, 
                    when client is connnected, tcpHandler create tcpWorker thread to deal 
                    with Attribute Value Interaction.         
                    @ref taskCreate
                */
                pthread_attr_init(&pAttrs);
                priParam.sched_priority = 1;
                status = pthread_attr_setschedparam(&pAttrs, &priParam);
                status |= pthread_attr_setstacksize(&pAttrs, TASK_STACK_SIZE);
                status = pthread_create(&tcpThread, &pAttrs, (void *(*)(void *))tcpHandler, (void*)TCPPORT);
                if(status)
                {
                    printError("tcpThread create failed", status);
                }

                /*  udp1Thread with acess function udp1Worker to deal with eeg data transmission */
                pthread_attr_init(&pAttrs);
                priParam.sched_priority = SOCKET_TASK_PRIORITY;
                status = pthread_attr_setschedparam(&pAttrs, &priParam);
                status |= pthread_attr_setstacksize(&pAttrs, TASK_STACK_SIZE);
                status = pthread_create(&udp1Thread, &pAttrs, (void *(*)(void *))udp1Worker, (void*)UDP1PORT);
                if(status)
                {
                    printError("udp1Thread create failed", status);
                }
                // update EEGDataPort Attr
                netparam.EEGdataPort = UDP1PORT;

                /* Start the Control task */
                pthread_attr_init(&pAttrs);
                priParam.sched_priority = CONTROL_TASK_PRIORITY;
                status = pthread_attr_setschedparam(&pAttrs, &priParam);
                status |= pthread_attr_setstacksize(&pAttrs, CONTROL_STACK_SIZE);

                status = pthread_create(&ControlThread, &pAttrs, (void *(*)(void *))controlTask, NULL);

                if(status)
                {
                    printError("control task create failed", status);
                }

                /* Start the Sample task */
                pthread_attr_init(&pAttrs);
                priParam.sched_priority = SAMPLE_TASK_PRIORITY;
                status = pthread_attr_setschedparam(&pAttrs, &priParam);
                status |= pthread_attr_setstacksize(&pAttrs, SAMPLE_STACK_SIZE);

                status = pthread_create(&SampleThread, &pAttrs, (void *(*)(void *))SampleTask , NULL);

                if(status)
                {
                    printError("sample task create failed", status);
                }

//                /*  udp2Thread with acess function udp2Worker to deal with event data transmission */
//                pthread_attr_init(&pAttrs);
//                priParam.sched_priority = SOCKET_TASK_PRIORITY;
//                status = pthread_attr_setschedparam(&pAttrs, &priParam);
//                status |= pthread_attr_setstacksize(&pAttrs, TASK_STACK_SIZE);
//                status = pthread_create(&udp2Thread, &pAttrs, (void *(*)(void *))udp2Worker, (void*)UDP2PORT);
//                if(status)
//                {
//                    printError("udp2Thread create failed", status);
//                }
//                // update EventDataPort Attr
//                netparam.EventDataPort = UDP2PORT;

            }
            break;
        default:
            break;
   }
}
/*!
    \brief          SimpleLinkFatalErrorEventHandler

    This handler gets called whenever a socket event is reported
    by the NWP / Host driver. After this routine is called, the user's
    application must restart the device in order to recover.

    \param          slFatalErrorEvent    -   pointer to fatal error event.

    \return         void

    \note           For more information, please refer to: user.h in the porting
                    folder of the host driver and the  CC31xx/CC32xx NWP programmer's
                    guide (SWRU455) section 17.9.

*/
void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *slFatalErrorEvent)
{
    /* Unused in this application */
}
/*!
    \brief          SimpleLinkNetAppRequestMemFreeEventHandler

    This handler gets called whenever the NWP is done handling with
    the buffer used in a NetApp request. This allows the use of
    dynamic memory with these requests.

    \param          pNetAppRequest     -   Pointer to NetApp request structure.

    \param          pNetAppResponse    -   Pointer to NetApp request Response.

    \note           For more information, please refer to: user.h in the porting
                    folder of the host driver and the  CC31xx/CC32xx NWP programmer's
                    guide (SWRU455) section 17.9.

    \return         void

*/
void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer)
{
    /* Unused in this application */
}

/*!
    \brief          SimpleLinkNetAppRequestEventHandler

    This handler gets called whenever a NetApp event is reported
    by the NWP / Host driver. User can write he's logic to handle
    the event here.

    \param          pNetAppRequest     -   Pointer to NetApp request structure.

    \param          pNetAppResponse    -   Pointer to NetApp request Response.

    \note           For more information, please refer to: user.h in the porting
                    folder of the host driver and the  CC31xx/CC32xx NWP programmer's
                    guide (SWRU455) section 17.9.

    \return         void

*/
void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest, SlNetAppResponse_t *pNetAppResponse)
{
    /* Unused in this application */
}

/*!
    \brief          SimpleLinkHttpServerEventHandler

    This handler gets called whenever a HTTP event is reported
    by the NWP internal HTTP server.

    \param          pHttpEvent       -   pointer to http event data.

    \param          pHttpEvent       -   pointer to http response.

    \return         void

    \note           For more information, please refer to: user.h in the porting
                    folder of the host driver and the  CC31xx/CC32xx NWP programmer's
                    guide (SWRU455) chapter 9.

*/
void SimpleLinkHttpServerEventHandler(SlNetAppHttpServerEvent_t *pHttpEvent,
                                      SlNetAppHttpServerResponse_t *pHttpResponse)
{
    /* Unused in this application */
}

/*!
    \brief          SimpleLinkWlanEventHandler

    This handler gets called whenever a WLAN event is reported
    by the host driver / NWP. Here user can implement he's own logic
    for any of these events. This handler is used by 'network_terminal'
    application to show case the following scenarios:

    1. Handling connection / Disconnection.
    2. Handling Addition of station / removal.
    3. RX filter match handler.
    4. P2P connection establishment.

    \param          pWlanEvent       -   pointer to Wlan event data.

    \return         void

    \note           For more information, please refer to: user.h in the porting
                    folder of the host driver and the  CC31xx/CC32xx NWP programmer's
                    guide (SWRU455) sections 4.3.4, 4.4.5 and 4.5.5.

    \sa             cmdWlanConnectCallback, cmdEnableFilterCallback, cmdWlanDisconnectCallback,
                    cmdP2PModecallback.

*/
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent)
{
    /* Unused in this application */
}
/*!
    \brief          SimpleLinkGeneralEventHandler

    This handler gets called whenever a general error is reported
    by the NWP / Host driver. Since these errors are not fatal,
    application can handle them.

    \param          pDevEvent    -   pointer to device error event.

    \return         void

    \note           For more information, please refer to: user.h in the porting
                    folder of the host driver and the  CC31xx/CC32xx NWP programmer's
                    guide (SWRU455) section 17.9.

*/
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent)
{
    /* Unused in this application */
}

/*!
    \brief          SimpleLinkSockEventHandler

    This handler gets called whenever a socket event is reported
    by the NWP / Host driver.

    \param          SlSockEvent_t    -   pointer to socket event data.

    \return         void

    \note           For more information, please refer to: user.h in the porting
                    folder of the host driver and the  CC31xx/CC32xx NWP programmer's
                    guide (SWRU455) section 7.6.

*/
void SimpleLinkSockEventHandler(SlSockEvent_t *pSock)
{
    /* Unused in this application */
}

/********************************************************************************
                         Local Functions
********************************************************************************/
/*
 *  ======== printError ========
 */
static void printError(char *errString, int code)
{
    Display_printf(display, 0, 0, "Error! code = %d, Description = %s\n", code,
            errString);
    while(1);
}

/*!
    \brief      DisplayBanner

     Application startup display

    \param      void

    \return     void
*/
static void DisplayBanner(char * AppName,char * AppVer)
{
    int32_t ret = 0;
    uint16_t ConfigSize = 0;
    uint16_t macAddressLen = 6;
    uint8_t ConfigOpt = SL_DEVICE_GENERAL_VERSION;
    ConfigSize = sizeof(SlDeviceVersion_t);

    /* Get device version info. */
    ret = sl_DeviceGet(SL_DEVICE_GENERAL, &ConfigOpt, &ConfigSize,
                       (uint8_t*)(&ver));
    if(ret)
    {
        printError("Error code", ret);
    }

    /* Get device Mac address */
    ret = sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, 0, &macAddressLen,
                       &netparam.MAC_Addr[0]);
    
    /* TODO FAKE ID create */
    ver.ChipId = SL_IPV4_VAL(netparam.MAC_Addr[2],netparam.MAC_Addr[3],\
                            netparam.MAC_Addr[4],netparam.MAC_Addr[5]);

    if(ret)
    {
        printError("Error code", ret);
    }

    Display_printf(display, 0, 0, "===============================================");
    Display_printf(display, 0, 0, "\t      %s Ver: %s \r\n",AppName, AppVer);
    Display_printf(display, 0, 0, "===============================================");
    Display_printf(display, 0, 0, "\t CHIPId: 0x%x \r\n",ver.ChipId);
    Display_printf(display, 0, 0, "\t MAC Version:  %d.%d.%d.%d \r\n",
                   ver.FwVersion[0],ver.FwVersion[1],
                   ver.FwVersion[2],ver.FwVersion[3]);
    Display_printf(display, 0, 0,"\t PHY Version:  %d.%d.%d.%d \r\n",
                   ver.PhyVersion[0],ver.PhyVersion[1],
                   ver.PhyVersion[2],ver.PhyVersion[3]);
    Display_printf(display, 0, 0,"\t NWP Version:  %d.%d.%d.%d \r\n",
                   ver.NwpVersion[0],ver.NwpVersion[1],
                   ver.NwpVersion[2],ver.NwpVersion[3]);
    Display_printf(display, 0, 0,"\t MAC Address: %02x-%02x-%02x-%02x-%02x-%02x\r\n",
                   netparam.MAC_Addr[0],netparam.MAC_Addr[1],\
                   netparam.MAC_Addr[2],netparam.MAC_Addr[3], \
                   netparam.MAC_Addr[4],netparam.MAC_Addr[5]);
}

/*!
    \brief      Connect

    This function is to connect the device to the specific router.
    @ref Router Param

    \param      void

    \return     void
*/
static void Connect(void)
{
    SlWlanSecParams_t   secParams = {0};
    int16_t ret = 0;
    secParams.Key = (signed char*)SECURITY_KEY;
    secParams.KeyLen = strlen(SECURITY_KEY);
    secParams.Type = SECURITY_TYPE;
    Display_printf(display, 0, 0, "Connecting to : %s.\r\n",SSID_NAME);
    ret = sl_WlanConnect((signed char*)SSID_NAME, strlen(SSID_NAME), 0, &secParams, 0);
    if (ret)
    {
        printError("Connection failed", ret);
    }
}

/********************************************************************************
                         External Functions
********************************************************************************/
/*!
    \brief      TaskCreate
    
    This function is to create a new tcpWorker task after tcp communication 
    is created

    \param      void

    \return     void
*/
void *TaskCreate(void (*pFun)(void *), char *Name, uint32_t StackSize,
        uintptr_t Arg1, uintptr_t Arg2, uintptr_t argReserved)
{
    int32_t             status = 0;
    pthread_attr_t      pAttrs_tcp;
    struct sched_param  priParam;

    /* Start the TCP Worker  */
    pthread_attr_init(&pAttrs_tcp);
    priParam.sched_priority = TCP_WORKER_PRIORITY;
    status = pthread_attr_setschedparam(&pAttrs_tcp, &priParam);
    status |= pthread_attr_setstacksize(&pAttrs_tcp, StackSize);

    status = pthread_create(&tcpworker_thread, &pAttrs_tcp, (void *(*)(void *))pFun, (void *)Arg1);

    if (status < 0) {
        return (NULL);
    }
    pthread_detach(tcpworker_thread);

    return ((void *)!status);
}

/********************************************************************************
                         Main Functions
********************************************************************************/
void mainThread(void *pvParameters)
{
    int32_t             status = 0;
    pthread_attr_t      pAttrs_spawn;
    struct sched_param  priParam;

    Timer_Params timerparams;

    /* Initial all the Peripherals */
    GPIO_init();
    SPI_init(); //[DANGER] never delete it because NWP need to communicate with AP by SPI
    Timer_init();

    Display_init();
    display = Display_open(Display_Type_UART, NULL);
    if (display == NULL) {
        /* Failed to open display driver */
        while(1);
    }
    
    /* pSampleTime work as the system timestamp */
    pSampleTime = SampleTimestamp_Service_Init(&timerparams);

    /* Initial ads1299 */
    ADS1299_Init(0);
    ADS1299_Mode_Config(1); //!< set ads1299 mode

    /* Initial AttrTbl */
    AttrTbl_Init();

    /* initializes signals for all tasks */
    sem_init(&UDPDataReady, 0, 0);
    sem_init(&SampleReady, 0, 0);

    /* led_green on to indicate all the drivers are ready */
    GPIO_write(LED_GREEN_GPIO,0);

    /* Start the SimpleLink Host */
    pthread_attr_init(&pAttrs_spawn);
    priParam.sched_priority = SPAWN_TASK_PRIORITY;
    status = pthread_attr_setschedparam(&pAttrs_spawn, &priParam);
    status |= pthread_attr_setstacksize(&pAttrs_spawn, TASK_STACK_SIZE);

    status = pthread_create(&spawn_thread, &pAttrs_spawn, sl_Task, NULL);
    if(status)
    {
        printError("Task create failed", status);
    }

    /* Turn NWP on - initialize the device*/
    mode = sl_Start(0, 0, 0);
    if( mode >= 0 )
    {
        /* Create the displaybanner */
        DisplayBanner(APPLICATION_NAME, APPLICATION_VERSION);
    }

    else if (mode < 0)
    {
        Display_printf(display, 0, 0,"\n\r[line:%d, error code:%d] %s\n\r", __LINE__, mode, DEVICE_ERROR);
    }

    if(mode != ROLE_STA)
    {
        /* Set NWP role as STA */
        mode = sl_WlanSetMode(ROLE_STA);
        if (mode < 0)
        {
            Display_printf(display, 0, 0,"\n\r[line:%d, error code:%d] %s\n\r", __LINE__, mode, WLAN_ERROR);
        }

        /* For changes to take affect, we restart the NWP */
        status = sl_Stop(SL_STOP_TIMEOUT);
        if (status < 0)
        {
            Display_printf(display, 0, 0,"\n\r[line:%d, error code:%d] %s\n\r", __LINE__, status, DEVICE_ERROR);
        }

        mode = sl_Start(0, 0, 0);
        if (mode < 0)
        {
            Display_printf(display, 0, 0,"\n\r[line:%d, error code:%d] %s\n\r", __LINE__, mode, DEVICE_ERROR);
        }
    }

    if(mode != ROLE_STA)
    {
        printError("Failed to configure device to it's default state", mode);
    }

    /* try to connect the router */
    Connect();
}
