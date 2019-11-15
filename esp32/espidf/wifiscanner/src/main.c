/**
 *  @file  main.c
 *  @brief User application main implementation.
 */

/**
 * ----------------------------------------------------------------------------------------------
 * includes
 * ----------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>

/**
 * ----------------------------------------------------------------------------------------------
 * Defines
 * ----------------------------------------------------------------------------------------------
 */

/**
 * @def   TAG
 * @brief Tag used in logs
 */
#define TAG  ( "scan" )

/**
 * @def   SCAN_LIST_SIZE
 * @brief Max number of AP records (size of AP record array)
 */
#define SCAN_LIST_SIZE ( 50 )

/**
 * ----------------------------------------------------------------------------------------------
 * Datatypes
 * ----------------------------------------------------------------------------------------------
 */

typedef struct
{
    bool started;
    uint16_t apCount;
    wifi_ap_record_t accessPoints[ SCAN_LIST_SIZE ];
} tAppData;

/**
 * ----------------------------------------------------------------------------------------------
 * Prototypes
 * ----------------------------------------------------------------------------------------------
 */

/**
 * @brief      app_init
 *             Initialization of stuff used by the app
 * @param[ - ] -
 * @return     -
 */
static void app_init( void );

/**
 * @brief      app_start
 *             Start of stuff used by the app
 * @param[ - ] -
 * @return     -
 */
static void app_start( void );

/**
 * @brief      app_run
 *             Non-returning function.
 * @param[ - ] -
 * @return     -
 */
static void app_run( void );

/**
 * @brief      configure_wifi
 *             Configure onboard WiFi.
 * @param[ - ] -
 * @return     -
 */
static void configure_wifi( void );

/**
 * @brief      scan_wifi
 *             Scans for WiFi access points and prints them to the log.
 * @param[ - ] -
 * @return     -
 */
static void scan_wifi( void );

/**
 * @brief      print_aps
 *             Print access points in a nice table.
 * @param[ - ] -
 * @return     -
 */
static void print_aps( void );

/**
 * @brief      wifi_event_handler
 *             Eventhandler for WiFi events. Mainly to shut up error logs.
 * @param[ - ] -
 * @return     -
 */
esp_err_t wifi_event_handler( system_event_t *event );

/**
 * @brief      authmode_to_str
 *             Get string representation of authentication mode.
 * @param[in]  authmode
 * @return     Pointer to string representation of authentication mode.
 */
static char *authmode_to_str( int authmode );

/**
 * @brief      cipher_to_str
 *             Get string representation of cipher.
 * @param[in]  cipher
 * @return     Pointer to string representation of cipher.
 */
static char *cipher_to_str( int cipher );

/**
 * ----------------------------------------------------------------------------------------------
 * Local variables
 * ----------------------------------------------------------------------------------------------
 */

static tAppData appData = { .started = false };

/**
 * ----------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------
 */

/**
 * @brief      app_main
 *             This is the entrypoint of the user application.
 * @param[ - ] -
 * @return     -
 */
void app_main( void )
{
    app_init();
    app_start();
    app_run();
}

/**
 * **********************************************************************************************
 * Function
 * **********************************************************************************************
 */
static void app_init( void )
{
    static bool initialized = false;
    if ( false == initialized )
    {
        initialized = true;

        // Initialize other modules used
        tcpip_adapter_init();

        // Initialize NVS -- Required for WiFi (apparently)
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK( ret );

        // Initialize data
        memset( &appData, 0, sizeof( appData ) );
    }
}

/**
 * **********************************************************************************************
 * Function
 * **********************************************************************************************
 */
static void app_start( void )
{
    if ( false == appData.started )
    {
        appData.started = true;

        // Create default event loop, used for system events such as WiFi-events.
        ESP_ERROR_CHECK( esp_event_loop_create_default() );

        // Configure WiFi
        configure_wifi();
    }
}

/**
 * **********************************************************************************************
 * Function
 * **********************************************************************************************
 */
static void app_run( void )
{
    while( true )
    {
        scan_wifi();
        vTaskDelay( pdMS_TO_TICKS( 1000 ) ); // Wait 1 second
    }
}

/**
 * **********************************************************************************************
 * Function
 * **********************************************************************************************
 */
static void configure_wifi( void )
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.event_handler = &wifi_event_handler;
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

/**
 * **********************************************************************************************
 * Function
 * **********************************************************************************************
 */
static void scan_wifi( void )
{
    // Scan in blocking mode, since this is all we do
    ESP_ERROR_CHECK( esp_wifi_scan_start( NULL, true ) );
    // Read out found access points
    uint16_t arrSize = SCAN_LIST_SIZE;
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_num( &(appData.apCount) ) );
    ESP_ERROR_CHECK( esp_wifi_scan_get_ap_records( &arrSize, (wifi_ap_record_t *)&(appData.accessPoints) ) );
    // Print out all access points
    print_aps();
}

/**
 * **********************************************************************************************
 * Function
 * **********************************************************************************************
 */
static void print_aps( void )
{
    // Print header
    printf( "SSID                              | BSSID             | RSSI | AUTH MODE       | PAIRWISE CIPHER | GROUP CIPHER \n" );
    printf( "----------------------------------+-------------------+------+-----------------+-----------------+--------------\n" );

    // Print access points
    for ( uint16_t i = 0; i < appData.apCount; ++i )
    {
        wifi_ap_record_t *pAp = &(appData.accessPoints[ i ]);
        printf( "%-33s   %02X:%02X:%02X:%02X:%02X:%02X   %d   %-15s   %-15s   %-12s\n",
                pAp->ssid,
                pAp->bssid[0],
                pAp->bssid[1],
                pAp->bssid[2],
                pAp->bssid[3],
                pAp->bssid[4],
                pAp->bssid[5],
                pAp->rssi,
                authmode_to_str( pAp->authmode ),
                cipher_to_str( pAp->pairwise_cipher ),
                cipher_to_str( pAp->group_cipher )
        );
    }
    printf( "\n" );
    printf( "[ %d APs found ] (max list size: %d)\n", appData.apCount, SCAN_LIST_SIZE );
    printf( "\n" );
}

/**
 * **********************************************************************************************
 * Function
 * **********************************************************************************************
 */
esp_err_t wifi_event_handler( system_event_t *event )
{
    return ESP_OK;
}

/**
 * **********************************************************************************************
 * Function
 * **********************************************************************************************
 */
static char *authmode_to_str( int authmode )
{
    switch ( authmode ) {
        case WIFI_AUTH_OPEN:
            return "OPEN";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA_PSK";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2_PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA_WPA2_PSK";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2_ENTERPRISE";
        default:
            return "UNKNOWN";
    }
}

/**
 * **********************************************************************************************
 * Function
 * **********************************************************************************************
 */
static char *cipher_to_str( int cipher )
{
    switch ( cipher ) {
        case WIFI_CIPHER_TYPE_NONE:
            return "NONE";
        case WIFI_CIPHER_TYPE_WEP40:
            return "WEP40";
        case WIFI_CIPHER_TYPE_WEP104:
            return "WEP104";
        case WIFI_CIPHER_TYPE_TKIP:
            return "TKIP";
        case WIFI_CIPHER_TYPE_CCMP:
            return "CCMP";
        case WIFI_CIPHER_TYPE_TKIP_CCMP:
            return "TKIP_CCMP";
        default:
            return "UNKNOWN";
    }
}