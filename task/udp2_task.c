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

/*
 *    ======== udp2_task.c ========
 *    Contains BSD sockets code.
 */

/*********************************************************************
 * INCLUDES
 */
#include <string.h>
#include <stdint.h>

/* BSD support */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <ti/net/slnetutils.h>
#include <ti/drivers/net/wifi/netcfg.h>
#include <ti/display/Display.h>

/* POSIX Header files */
#include <pthread.h>
#include <semaphore.h>

#include <protocol/evtdata_protocol.h>
#include <attr/attrTbl.h>

/***********************************************************************
 *  GLOBAL VARIABLES
 */

/***********************************************************************
 *  EXTERNAL VARIABLES
 */
extern UDPEvtFrame_t UDP_EvtTX_Buff;    //!< UDP发送缓冲区
extern sem_t UDPEvtDataReady;           //!< UDP脑电数据包完毕信号量
extern Display_Handle display;

/***********************************************************************
 * FUNCTIONS
 */
/*
 *  ======== udp2Handler ========
 *  Transmit Event data via UDP2 channel
 *
 */
void udp2Worker(uint32_t arg0, uint32_t arg1)
{
    int                bytesSent;
    int                status;
    int                server;
    struct sockaddr_in localAddr;
    struct sockaddr_in clientAddr;

    Display_printf(display, 0, 0, "UDP1 data channel start\n");

    server = socket(AF_INET, SOCK_DGRAM, 0);
    if (server == -1) {
        Display_printf(display, 0, 0, "Error: socket not created.\n");
        goto shutdown;
    }

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(arg0);

    status = bind(server, (struct sockaddr *)&localAddr, sizeof(localAddr));
    if (status == -1) {
        Display_printf(display, 0, 0, "Error: bind failed.\n");
        goto shutdown;
    }

    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(arg0); //!< 事件标签数据通道端口
    clientAddr.sin_addr.s_addr = htonl(SL_IPV4_VAL(255,255,255,255)); //!< 局域网广播

    while(1)
    {

        /* 等待信号量 */
        sem_wait(&UDPEvtDataReady);

        bytesSent = sendto(server, (uint8_t*)(&UDP_EvtTX_Buff),UDP_EvtTx_Buff_Size,0,
                       (struct sockaddr*)&clientAddr,sizeof(SlSockAddr_t));

    }

shutdown:
    if (server != -1) {
        close(server);
    }
}
