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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include "Ap.h"
#include <Config/IApCfg.h>

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

typedef struct ApVars_s
{
    struct station_config stationConfig;
    bool connected;
    bool ready;
} tApVars;

/**
 * ------------------------------------------------------------------
 * Prototypes
 * ------------------------------------------------------------------
 */

void Ap_HandleEventCb( System_Event_t *event );

/**
 * ------------------------------------------------------------------
 * Private data
 * ------------------------------------------------------------------
 */

static tApVars apVars = {
    .connected = FALSE,
    .ready     = FALSE
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
void IAp_Init( void )
{
    static bool initialized = FALSE;
    if ( !initialized )
    {
        initialized = TRUE;
        memset( &apVars, 0, sizeof( apVars ) );
    }
}

/**
 * ****************************************************************************
 * Function
 * ****************************************************************************
 */
void IAp_Start( void )
{
    // Nothing to do for now
}

/**
 * ****************************************************************************
 * Function
 * ****************************************************************************
 */
bool IAp_ConnectToWifi( void )
{
    if ( wifi_set_opmode( STATIONAP_MODE ) == FALSE )
    {
        return FALSE;
    }
    
    memset( &apVars.stationConfig, 0, sizeof( apVars.stationConfig ) );
    sprintf( apVars.stationConfig.ssid, AP_SSID );
    sprintf( apVars.stationConfig.password, AP_PASS );

    if ( wifi_station_set_config( &apVars.stationConfig ) == FALSE )
    {
        return FALSE;
    }
    if ( wifi_set_event_handler_cb( Ap_HandleEventCb ) == FALSE )
    {
        return FALSE;
    }
    if ( wifi_station_connect() == FALSE )
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * ****************************************************************************
 * Function
 * ****************************************************************************
 */
bool IAp_IsReady( void )
{
    return apVars.connected;
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
void Ap_HandleEventCb( System_Event_t *event )
{
    switch( event->event_id )
    {
        case EVENT_STAMODE_CONNECTED:
        {
            // Connected to access point
            printf( "Connected to WiFi AP\n" );
            apVars.connected = TRUE;
        }
        break;
        case EVENT_STAMODE_DISCONNECTED:
        {
            // Disconnected from access point
            printf( "Disconnected from WiFi\n" );
            apVars.connected = FALSE;
            apVars.ready     = FALSE;
        }
        break;
        case EVENT_STAMODE_AUTHMODE_CHANGE:
        {
            // Authentication mode change
            printf( "Auth mode change\n" );
        }
        break;
        case EVENT_STAMODE_GOT_IP:
        {
            // Got IP from access point
            printf( "Got IP:" IPSTR "\n", IP2STR( &event->event_info.got_ip.ip ) );
            apVars.ready = TRUE;
        }
        break;
        default:
        {
            // Unhandled event
        }
        break;
    }
}