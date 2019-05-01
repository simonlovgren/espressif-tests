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

#include "esp_common.h"
#include "freertos/task.h"


#include <Ap/IAp.h>
#include <UdpServer/IUdpServer.h>

#include "gpio.h"

/**
 * ****************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 * ****************************************************************************
 */
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

/**
 * ****************************************************************************
 * Function
 * ****************************************************************************
 */
void task_blink(void* ignore)
{
    uint32_t count = 0;
    gpio16_output_conf();
    while(true) {
        gpio16_output_set(0);
        vTaskDelay(1000/portTICK_RATE_MS);
        gpio16_output_set(1);
        vTaskDelay(1000/portTICK_RATE_MS);
        ++count;
    }

    vTaskDelete(NULL);
}

/**
 * ****************************************************************************
 * Function
 * ****************************************************************************
 */
void task_tick_announcement(void* ignore)
{
    uint32_t secondsSinceStart = 0;
    while(true) {
        vTaskDelay(1000/portTICK_RATE_MS);
        ++secondsSinceStart;
        os_printf( "%u s\n", secondsSinceStart );
    }

    vTaskDelete(NULL);
}


/**
 * ****************************************************************************
 * Function
 * ****************************************************************************
 */
void task_connect(void* ignore)
{
    IAp_Init();
    IAp_Start();

    if ( IAp_ConnectToWifi() == FALSE )
    {
        os_printf( "Unable to connect to WiFi\n" );
        goto error;
    }
    
    while( IAp_IsReady() == FALSE )
    {
        os_printf( "Not ready\n" );
        vTaskDelay( 1000 );
    }
    os_printf( "ready\n" );

    if ( IUdpServer_Start() == FALSE )
    {
        os_printf( "Unable to start UDP server.\n" );
    }

    
    error:
    vTaskDelete(NULL);
}



/**
 * ****************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 * ****************************************************************************
 */
void user_init(void)
{
    xTaskCreate(&task_connect, "connect_wifi", 512, NULL, 6, NULL);
    xTaskCreate(&task_blink, "startup", 512, NULL, 1, NULL);
    xTaskCreate(&task_tick_announcement, "tick_announcer", 512, NULL, 1, NULL);
}