/*
 * Copyright (c) 2014-2018, Texas Instruments Incorporated
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
 *    ======== tcp_task.c ========
 *    Contains BSD sockets code.
 */

/*******************************************************************************
 * INCLUDES
 */

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <pthread.h>
/* BSD support */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include <ti/net/slnetutils.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>
#include <ti/display/Display.h>

#include <protocol/attr_protocol.h>

#define NUMTCPWORKERS 3

/*******************************************************************************
 *  EXTERNAL VARIABLES
 */
extern Display_Handle display;
extern uint8_t *pTCP_Tx_Buff;
extern uint8_t *pTCP_Rx_Buff;

/*******************************************************************************
 *  EXTERNAL FUNCITONS
 */
extern void *TaskCreate(void (*pFun)(), char *Name, uint32_t StackSize, 
                        uintptr_t Arg1, uintptr_t Arg2, uintptr_t Arg3);


/*******************************************************************************
 * FUNCTIONS
 */
/*
 *  ======== tcpWorker ========
 *  Task to handle TCP connection. Can be multiple Tasks running
 *  this function.
 */
void tcpWorker(uint32_t arg0, uint32_t arg1)
{
    int  clientfd = (int)arg0;
    int  bytesRcvd;


    Display_printf(display, 0, 0, "tcpWorker: start clientfd = 0x%x\n",
            clientfd);

    while ((bytesRcvd = recv(clientfd, pTCP_Rx_Buff, TCP_Rx_Buff_Size, 0)) > 0)
    {
        if( TCP_ProcessFSM(pTCP_Rx_Buff) == true ) // 控制通道帧协议处理完毕
        {
            send(clientfd, pTCP_Tx_Buff, (*(pTCP_Tx_Buff+1)+3), 0);
            memset(pTCP_Rx_Buff,0x00,TCP_Rx_Buff_Size);  //!< 清空接收缓冲区
        }
    }

    Display_printf(display, 0, 0, "tcpWorker stop clientfd = 0x%x\n", clientfd);

    close(clientfd);
}

/*
 *  ======== tcpHandler ========
 *  Creates new Task to handle new TCP connections.
 */
void tcpHandler(uint32_t arg0, uint32_t arg1)
{
    void *thread = NULL;
    int                status;
    int                clientfd;
    int                server;
    struct sockaddr_in localAddr;
    struct sockaddr_in clientAddr;
    int                optval;
    int                optlen = sizeof(optval);
    socklen_t          addrlen = sizeof(clientAddr);

    TCP_ProcessFSMInit(); //初始化控制通道协议处理状态机

    Display_printf(display, 0, 0, "TCP control channel start\n");

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        Display_printf(display, 0, 0, "tcpHandler: socket failed\n");
        goto shutdown;
    }


    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(arg0);

    status = bind(server, (struct sockaddr *)&localAddr, sizeof(localAddr));
    if (status == -1) {
        Display_printf(display, 0, 0, "tcpHandler: bind failed\n");
        goto shutdown;
    }

    status = listen(server, NUMTCPWORKERS);
    if (status == -1) {
        Display_printf(display, 0, 0, "tcpHandler: listen failed\n");
        goto shutdown;
    }

    optval = 1;
    status = setsockopt(server, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
    if (status == -1) {
        Display_printf(display, 0, 0, "tcpHandler: setsockopt failed\n");
        goto shutdown;
    }

    while ((clientfd =
            accept(server, (struct sockaddr *)&clientAddr, &addrlen)) != -1) {

        Display_printf(display, 0, 0,
                "tcpHandler: Creating thread clientfd = %x\n", clientfd);

        // 上位机(plumberhub)TCP连接建立，创建tcpWorker线程处理数据收发事务
        thread = TaskCreate(tcpWorker, NULL, 2048, (uintptr_t)clientfd, 0, 0);

        if (!thread) {
            Display_printf(display, 0, 0,
                    "tcpHandler: Error - Failed to create new thread.\n");
            close(clientfd);
        }

        /* addrlen is a value-result param, must reset for next accept call */
        addrlen = sizeof(clientAddr);
    }

    Display_printf(display, 0, 0, "tcpHandler: accept failed.\n");

shutdown:
    if (server != -1) {
        close(server);
    }
}
