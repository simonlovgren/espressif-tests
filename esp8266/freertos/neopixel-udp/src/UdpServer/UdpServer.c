/**
 * MIT License
 * 
 * Copyright (c) 2019 Simon Lövgren
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * ------------------------------------------------------------------
 * Includes
 * ------------------------------------------------------------------
 */

#include <esp_common.h>
#include <espconn.h>

#include <freertos/task.h>
#include <freertos/queue.h>

#include "UdpServer.h"

/**
 * ------------------------------------------------------------------
 * Defines
 * ------------------------------------------------------------------
 */

/**
 * ------------------------------------------------------------------
 * Typedefs
 * ------------------------------------------------------------------
 */

typedef struct {
    struct espconn connection;
    esp_udp        udp;
} tUdpServerVars;


/**
 * ------------------------------------------------------------------
 * Prototypes
 * ------------------------------------------------------------------
 */

void UdpServer_RecvCb( void *pArg, char *pData, unsigned short len );
void UdpServer_SendCb( void *pArg );

/**
 * ------------------------------------------------------------------
 * Private data
 * ------------------------------------------------------------------
 */

static tUdpServerVars udpServerVars;


/**
 * ------------------------------------------------------------------
 * Interface implementation
 * ------------------------------------------------------------------
 */

/**
 * ****************************************************************************
 * Function
 * ****************************************************************************
 */
bool IUdpServer_Start( void )
{
    memset( &udpServerVars.connection, 0, sizeof( udpServerVars.connection ) );
    memset( &udpServerVars.udp, 0, sizeof( udpServerVars.udp ) );

    udpServerVars.connection.type      = ESPCONN_UDP;
    udpServerVars.connection.state     = ESPCONN_NONE;
    udpServerVars.connection.proto.udp = &udpServerVars.udp;
    udpServerVars.udp.local_port      = 8000;

    // os_printf( "" );
    espconn_regist_recvcb( &udpServerVars.connection, UdpServer_RecvCb );
    espconn_regist_sentcb( &udpServerVars.connection, UdpServer_SendCb );

    int8 res = 0;
    if ( espconn_create( &udpServerVars.connection ) != 0 )
    {
        os_printf( "Unable to create UDP server.\n" );
        return FALSE;
    }

    os_printf( "UDP server started on port [%d].\n", udpServerVars.udp.local_port );
    return TRUE;
}


/**
 * ------------------------------------------------------------------
 * Private functions
 * ------------------------------------------------------------------
 */

/**
 * ****************************************************************************
 * Function
 * ****************************************************************************
 */
void UdpServer_RecvCb( void *pArg, char *pData, unsigned short len )
{
    struct espconn *pConnection = (struct espconn *)pArg;
    // Store the IP address from the sender of this data.

    os_printf( "Data: %s\n", pData );

    remot_info *remote = NULL;
    if( espconn_get_connection_info( pConnection, &remote, 0 ) == 0  && remote != NULL )
    {
        os_printf( "REMOTE INFO:\n\tremote_port: %d\n\tudp.remote_ip: %d.%d.%d.%d\n",
        remote->remote_port,
        remote->remote_ip[0],
        remote->remote_ip[1],
        remote->remote_ip[2],
        remote->remote_ip[3]
    );
    }
    else
    {
        os_printf( "Unable to retreive remote information...\n" );
    }

    // Send data right back
    pConnection->proto.udp->remote_port = remote->remote_port;
    memcpy( &(pConnection->proto.udp->remote_ip), &(remote->remote_ip), sizeof( remote->remote_ip ) );
    
    espconn_send(pConnection, pData, len);
}

/**
 * ****************************************************************************
 * Function
 * ****************************************************************************
 */
void UdpServer_SendCb( void *pArg )
{
    struct espconn* pConnection = pArg;
    os_printf(
        "UDP_SEND_CB ip:%d.%d.%d.%d port:%d\n",
        pConnection->proto.udp->remote_ip[0],
        pConnection->proto.udp->remote_ip[1],
        pConnection->proto.udp->remote_ip[2],
        pConnection->proto.udp->remote_ip[3],
        pConnection->proto.udp->remote_port
    );
}