/**
 * @file    main.cpp
 * @brief   Simple packet sniffer/monitor using ESP8266 in
 *          promiscuous mode.
 * 
 * @author  Simon Lövgren
 * @license MIT
 * 
 * Based on "PacketMonitor" by Stefan Kremser:
 * https://github.com/spacehuhn/PacketMonitor
 */

/**
 * ------------------------------------------------------------------
 * Includes
 * ------------------------------------------------------------------
 */
#include <Arduino.h>
#include <ESP8266WiFi.h>

/**
 * ------------------------------------------------------------------
 * Defines
 * ------------------------------------------------------------------
 */
// De-mystify enable/disable functions
#define DISABLE 0
#define ENABLE  1

// Max channel number (US = 11, EU = 13, Japan = 14)
#define MAX_CHANNEL   13

// Channel to set
#define CHANNEL       1

// Deauth alarm level (packet rate per second)
#define DEAUTH_ALARM_LEVEL    5

// How long to sleep in main loop
#define LOOP_DELAY_MS         1000

/**
 * ------------------------------------------------------------------
 * Typedefs
 * ------------------------------------------------------------------
 */

/**
 * ------------------------------------------------------------------
 * Prototypes
 * ------------------------------------------------------------------
 */

static void packetSniffer( uint8_t* buffer, uint16_t length );

/**
 * ------------------------------------------------------------------
 * Private data
 * ------------------------------------------------------------------
 */

// Packet counters
static unsigned long packets             = 0;
static unsigned long deauths             = 0;
static unsigned long totalPackets        = 0;       // Should probably be long long, but can't be bothered to fix the serial print of it...
static unsigned long totalDeauths        = 0;       // Should probably be long long, but can't be bothered to fix the serial print of it...
static unsigned long maxPackets          = 0;
static unsigned long maxDeauths          = 0;
static unsigned long minPackets          = -1;
static unsigned long minDeauths          = -1;

/**
 * ------------------------------------------------------------------
 * Interface implementation
 * ------------------------------------------------------------------
 */

/**
 * ******************************************************************
 * Function
 * ******************************************************************
 */
void setup( void )
{
    // Enable serial communication over UART @ 115200 baud
    Serial.begin( 115200 );

    // Set up ESP8266 in promiscuous mode
    wifi_set_opmode( STATION_MODE );
    wifi_promiscuous_enable( DISABLE );
    WiFi.disconnect();
    wifi_set_promiscuous_rx_cb( packetSniffer );
    wifi_promiscuous_enable( ENABLE );

    // Currently only sniffing pre-defined channel.
    // Should rotate through all channels in loop continuously and
    // use yield() instead of sleep.
    wifi_set_channel( CHANNEL );

    // Report setup completed
    Serial.println( "Setup completed." );
}

/**
 * ******************************************************************
 * Function
 * ******************************************************************
 */
void loop( void )
{
    delay( LOOP_DELAY_MS );
    unsigned long currentPackets = packets;
    unsigned long currentDeauths = deauths;

    // Add to total
    totalPackets += currentPackets;
    totalDeauths += currentDeauths;

    // Grab max/min
    if ( currentPackets > maxPackets )
    {
        maxPackets = currentPackets;
    }
    if ( currentPackets < minPackets )
    {
        minPackets = currentPackets;
    }
    if ( currentDeauths > maxDeauths )
    {
        maxDeauths = currentDeauths;
    }
    if ( currentDeauths < minDeauths )
    {
        minDeauths = currentDeauths;
    }

    // Spacing
    Serial.print( "\n" );

    // Print statistics
    Serial.print( "           SEEN    MAX     MIN     TOTAL\n" );
    Serial.print( "           --------------------------------------\n" );
    Serial.printf( "PACKETS    %-4lu    %-4lu    %-4lu    %lu\n", currentPackets, maxPackets, minPackets, totalPackets );
    Serial.printf( "DEAUTHS    %-4lu    %-4lu    %-4lu    %lu\n", currentDeauths, maxDeauths, minDeauths, totalDeauths );

    // Deauth alarm
    if ( deauths > DEAUTH_ALARM_LEVEL )
    {
        Serial.println("\n[ DEAUTH ALARM ]");
    }

    // For additional spacing
    Serial.print( "\n" );

    // Reset counters
    packets = 0;
    deauths = 0;
}

/**
 * ------------------------------------------------------------------
 * Private functions
 * ------------------------------------------------------------------
 */

/**
 * ******************************************************************
 * Function
 * ******************************************************************
 */
static void packetSniffer( uint8_t* buffer, uint16_t length )
{
    // Gets called for each packet
    ++packets;
    if ( buffer[ 12 ] == 0xA0 || buffer[ 12 ] == 0xC0 )
    {
        ++deauths;
    }
}