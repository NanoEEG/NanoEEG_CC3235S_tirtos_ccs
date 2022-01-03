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
 *    ======== detect_task.c ========
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

#include <attr/attrTbl.h>

/***********************************************************************
 *  GLOBAL VARIABLES
 */
uint8_t UDP_DetectedBuff[6];

/***********************************************************************
 *  EXTERNAL VARIABLES
 */
extern SlDeviceVersion_t ver;           //!< 仪器参数
extern Display_Handle display;

/***********************************************************************
 * FUNCTIONS
 */
void DetectTask(uint32_t arg0, uint32_t arg1)
{
    int                bytesRcvd;
    int                bytesSent;
    int                status;
    int                server;
    int                replycnt;
    fd_set             readSet;
    socklen_t          addrlen;
    struct sockaddr_in localAddr;
    struct sockaddr_in clientAddr;

    Display_printf(display, 0, 0, "Detect Task start\n");

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
    clientAddr.sin_port = htons(arg0);
    clientAddr.sin_addr.s_addr = htonl(SL_IPV4_VAL(255,255,255,255));

    while(1)
    {
        /*
         *  readSet and addrlen are value-result arguments, which must be reset
         *  in between each select() and recvfrom() call
         */
        FD_ZERO(&readSet);
        FD_SET(server, &readSet);
        addrlen = sizeof(clientAddr);

        /* Wait forever for the reply */
        status = select(server + 1, &readSet, NULL, NULL, NULL);
        if (status > 0) {

            if (FD_ISSET(server, &readSet)) {
                bytesRcvd = recvfrom(server, UDP_DetectedBuff, 6, 0,
                        (struct sockaddr *)&clientAddr, &addrlen);

                if(UDP_DetectedBuff[0]==0xCC && UDP_DetectedBuff[5]==0xC2){
                    memcpy(&UDP_DetectedBuff[1],&(ver.ChipId),4);
                    UDP_DetectedBuff[0] = 0xC2;
                    UDP_DetectedBuff[5] = 0xCC;
                    replycnt ++;
                }

                if (bytesRcvd > 0 ) {
                    bytesSent = sendto(server, UDP_DetectedBuff, 6, 0,
                            (struct sockaddr *)&clientAddr, sizeof(SlSockAddr_t));

                    if (bytesSent < 0 || bytesSent != bytesRcvd) {
                        Display_printf(display, 0, 0,
                                "Error: sendto failed.\n");
                        goto shutdown;
                    }
                }
            }
        }


    }

shutdown:
    if (server != -1) {
        close(server);
    }
}
