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

#define NUM_SERVERS 2

/**
 * ------------------------------------------------------------------
 * Typedefs
 * ------------------------------------------------------------------
 */

typedef struct {
    uint16_t       port;
    struct espconn connection;
    esp_udp        udp;
} tUdpServer;

typedef struct {
    tUdpServer servers[ NUM_SERVERS ];
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

static tUdpServerVars udpServerVars = {
    .servers = {
        { 8000 },
        { 9000 }
    }
};


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
void IUdpServer_Start( void )
{
    for ( uint32_t i = 0; i < NUM_SERVERS; ++i )
    {
        tUdpServer *pServer = &udpServerVars.servers[ i ];

        memset( &pServer->connection, 0, sizeof( pServer->connection ) );
        memset( &pServer->udp, 0, sizeof( pServer->udp ) );

        pServer->connection.type      = ESPCONN_UDP;
        pServer->connection.state     = ESPCONN_NONE;
        pServer->connection.proto.udp = &pServer->udp;
        pServer->udp.local_port       = pServer->port;

        pServer->connection.reserve   = ( void * )i;

        // os_printf( "" );
        espconn_regist_recvcb( &pServer->connection, UdpServer_RecvCb );
        espconn_regist_sentcb( &pServer->connection, UdpServer_SendCb );

        int8 res = espconn_create( &pServer->connection );
        if ( res != 0 )
        {
            os_printf( "Unable to create UDP server on port [%d].\n", pServer->port );
            os_printf( "Reason: " );
            switch( res )
            {
                case ESPCONN_MEM:
                {
                    os_printf( "Out of memory\n" );
                }
                break;
                case ESPCONN_ISCONN:
                {
                    os_printf( "Already connected\n" );
                }
                break;
                case ESPCONN_ARG:
                {
                    os_printf( "Illegal argument\n" );
                }
                break;
                default:
                {
                    os_printf( "other [%u]\n", res );
                }
                break;
            }
        }
        else
        {
            os_printf( "UDP server started on port [%d].\n", pServer->port );
        }
        vTaskDelay( 1000/portTICK_RATE_MS );
    }
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

    os_printf( "[%d] Data: %s\n", (uint32_t)pConnection->reserve, pData );

    remot_info *remote = NULL;
    if( espconn_get_connection_info( pConnection, &remote, 0 ) == 0  && remote != NULL )
    {
        os_printf( "[%d] REMOTE INFO:\n\tremote_port: %d\n\tudp.remote_ip: %d.%d.%d.%d\n",
        (uint32_t)pConnection->reserve,
        remote->remote_port,
        remote->remote_ip[0],
        remote->remote_ip[1],
        remote->remote_ip[2],
        remote->remote_ip[3]
    );
    }
    else
    {
        os_printf( "[%d] Unable to retreive remote information...\n", (uint32_t)pConnection->reserve );
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
        "[%d] UDP_SEND_CB ip:%d.%d.%d.%d port:%d\n",
        (uint32_t)pConnection->reserve,
        pConnection->proto.udp->remote_ip[0],
        pConnection->proto.udp->remote_ip[1],
        pConnection->proto.udp->remote_ip[2],
        pConnection->proto.udp->remote_ip[3],
        pConnection->proto.udp->remote_port
    );
}