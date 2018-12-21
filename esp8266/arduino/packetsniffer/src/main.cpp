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

typedef struct RxControl {
    signed rssi:8;
    unsigned rate:4;
    unsigned is_group:1;
    unsigned:1;
    unsigned sig_mode:2;
    unsigned legacy_length:12;
    unsigned damatch0:1;
    unsigned damatch1:1;
    unsigned bssidmatch0:1;
    unsigned bssidmatch1:1;
    unsigned MCS:7;
    unsigned CWB:1;
    unsigned HT_length:16;
    unsigned Smoothing:1;
    unsigned Not_Sounding:1;
    unsigned:1;
    unsigned Aggregation:1;
    unsigned STBC:2;
    unsigned FEC_CODING:1;
    unsigned SGI:1;
    unsigned rxend_state:8;
    unsigned ampdu_cnt:8;
    unsigned channel:4;
    unsigned:12;
} tRxControl;

typedef struct Ampdu_Info
{
  uint16 length;
  uint16 seq;
  uint8  address3[6];
} tAmpduInfo;

typedef struct sniffer_buf {
    tRxControl rx_ctrl;
    uint8_t  buf[36];
    uint16_t cnt;
    tAmpduInfo ampdu_info[1];
} tSnifferBuf;


typedef enum{
    FRAME_TYPE_MANAGEMENT = 0x0,
    FRAME_TYPE_CONTROL    = 0x1,
    FRAME_TYPE_DATA       = 0x2,
    FRAME_TYPE_RESERVED   = 0x3
} tFrameType;

typedef enum{
    MANAGEMENT_TYPE_ASSOC_REQ = 0,
    MANAGEMENT_TYPE_ASSOC_RSP,
    MANAGEMENT_TYPE_REASSOC_REQ,
    MANAGEMENT_TYPE_REASSOC_RSP,
    MANAGEMENT_TYPE_PROBE_REQ,
    MANAGEMENT_TYPE_PROBE_RSP,
    // 0110 - 0111 RESERVED
    MANAGEMENT_TYPE_BEACON = 0x80,
    MANAGEMENT_TYPE_ATIM,
    MANAGEMENT_TYPE_DISASSOC,
    MANAGEMENT_TYPE_AUTHENTICATION,
    MANAGEMENT_TYPE_DEAUTHENTICATION,
    MANAGEMENT_TYPE_ACTION
    // 1110 - 1111 RESERVED
} tManagementSubType;

typedef enum{
    // 0000 - 0111 RESERVED
    CONTROL_TYPE_BLOCK_ACK_REQ = 0x80,
    CONTROL_TYPE_BLOCK_ACK,
    CONTROL_TYPE_PS_POLL,
    CONTROL_TYPE_RTS,
    CONTROL_TYPE_CTS,
    CONTROL_TYPE_ACK,
    CONTROL_TYPE_CF_END,
    CONTROL_TYPE_CF_END_ACK
} tControlSubType;

typedef enum{
    DATA_TYPE_DATA,
    DATA_TYPE_DATA_CF_ACK,
    DATA_TYPE_DATA_CF_POLL,
    DATA_TYPE_DATA_CF_ACK_POLL,
    DATA_TYPE_NULL,
    DATA_TYPE_CF_ACK,
    DATA_TYPE_CF_POLL,
    DATA_TYPE_CF_ACK_POLL,
    DATA_TYPE_QOS_DATA,
    DATA_TYPE_QOS_DATA_CF_ACK,
    DATA_TYPE_QOS_DATA_CF_POLL,
    DATA_TYPE_QOS_DATA_CF_ACK_POLL,
    DATA_TYPE_QOS_NULL,
    DATA_TYPE_RESERVED,
    DATA_TYPE_QOS_CF_POLL_NODATA,
    DATA_TYPE_QOS_CF_ACK_NODATA
} tDataSubType;

typedef struct
{
    uint8_t protocol;
    uint8_t type;
    uint8_t subtype;
    uint8_t toDS;
    uint8_t fromDS;
    uint8_t moreFragments;
    uint8_t retry;
    uint8_t powerManagement;
    uint8_t moreData;
    uint8_t protectedBit;
    uint8_t order;

} tFrameControl;

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

void expandFrameControl( uint8_t frameBytes[2], tFrameControl* pFrameControl )
{
    // First Byte
    pFrameControl->protocol = ( frameBytes[0] & 0x03 );
    pFrameControl->type     = ( frameBytes[0] & 0x0C ) >> 2;
    pFrameControl->subtype  = ( frameBytes[0] & 0xF0 ) >> 4;

    // Second Byte
    pFrameControl->toDS                 = ( frameBytes[1] & 0x01 );
    pFrameControl->fromDS               = ( frameBytes[1] & 0x02 ) >> 1;
    pFrameControl->moreFragments        = ( frameBytes[1] & 0x04 ) >> 2;
    pFrameControl->retry                = ( frameBytes[1] & 0x08 ) >> 3;
    pFrameControl->powerManagement      = ( frameBytes[1] & 0x10 ) >> 4;
    pFrameControl->moreData             = ( frameBytes[1] & 0x20 ) >> 5;
    pFrameControl->protectedBit         = ( frameBytes[1] & 0x40 ) >> 6;
    pFrameControl->order                = ( frameBytes[1] & 0x80 ) >> 7;
}


/**
 * ******************************************************************
 * Function
 * ******************************************************************
 */
static void packetSniffer( uint8_t* buffer, uint16_t length )
{
    // Gets called for each packet
    ++packets;

    tSnifferBuf* pBuf = (tSnifferBuf*)buffer;
    if ( length > 12 )
    {
        // Split out frame control
        tFrameControl frameControl;
        expandFrameControl( pBuf->buf, &frameControl );

        if ( frameControl.type == FRAME_TYPE_MANAGEMENT && frameControl.subtype == MANAGEMENT_TYPE_DEAUTHENTICATION )
        {
            ++deauths;
        }
        else if ( frameControl.type == FRAME_TYPE_MANAGEMENT && frameControl.subtype == MANAGEMENT_TYPE_PROBE_REQ )
        {
            Serial.println( "Probe request encountered" );
            Serial.printf( "Data length: %u\n", length );
            Serial.print( "Data: ");
            for ( uint8_t i = 0; i < 32; ++i )
            {
                Serial.printf( "%02X", pBuf->buf[i] );
            }
            Serial.print( "\n" );
        }
    }

}