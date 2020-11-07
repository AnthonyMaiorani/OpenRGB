/*---------------------------------------------------------*\
|  Driver for Philips Hue Entertainment Mode                |
|                                                           |
|  Adam Honse (calcprogrammer1@gmail.com), 11/6/2020        |
\*---------------------------------------------------------*/

#include "PhilipsHueEntertainmentController.h"

std::vector<char> HexToBytes(const std::string& hex)
{
    std::vector<char> bytes;

    for (unsigned int i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        char byte = (char) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

PhilipsHueEntertainmentController::PhilipsHueEntertainmentController(hueplusplus::Bridge& bridge_ptr, hueplusplus::Group& group_ptr):bridge(bridge_ptr),group(group_ptr)
{
    /*-------------------------------------------------*\
    | Signal the bridge to start streaming              |
    \*-------------------------------------------------*/
    bridge.StartStreaming(std::to_string(group.getId()));

    /*-------------------------------------------------*\
    | Fill in location string with bridge IP            |
    \*-------------------------------------------------*/
    location                            = "IP: " + bridge.getBridgeIP();

    /*-------------------------------------------------*\
    | Get the number of lights from the group           |
    \*-------------------------------------------------*/
    num_leds                            = group.getLightIds().size();

    /*-------------------------------------------------*\
    | Create Entertainment Mode message buffer          |
    \*-------------------------------------------------*/
    entertainment_msg_size              = HUE_ENTERTAINMENT_HEADER_SIZE + (num_leds * HUE_ENTERTAINMENT_LIGHT_SIZE);
    entertainment_msg                   = new unsigned char[entertainment_msg_size];

    /*-------------------------------------------------*\
    | Fill in Entertainment Mode message header         |
    \*-------------------------------------------------*/
    memcpy(entertainment_msg, "HueStream", 9);
    entertainment_msg[9]                = 0x01;                                     // Version Major (1)
    entertainment_msg[10]               = 0x00;                                     // Version Minor (0)
    entertainment_msg[11]               = 0x00;                                     // Sequence ID
    entertainment_msg[12]               = 0x00;                                     // Reserved
    entertainment_msg[13]               = 0x00;                                     // Reserved
    entertainment_msg[14]               = 0x00;                                     // Color Space (RGB)
    entertainment_msg[15]               = 0x00;                                     // Reserved

    /*-------------------------------------------------*\
    | Fill in Entertainment Mode light data             |
    \*-------------------------------------------------*/
    for(unsigned int light_idx = 0; light_idx < num_leds; light_idx++)
    {
        unsigned int msg_idx            = HUE_ENTERTAINMENT_HEADER_SIZE + (light_idx * HUE_ENTERTAINMENT_LIGHT_SIZE);

        entertainment_msg[msg_idx + 0]  = 0x00;                                     // Type (Light)
        entertainment_msg[msg_idx + 1]  = group.getLightIds()[light_idx] >> 8;      // ID MSB
        entertainment_msg[msg_idx + 2]  = group.getLightIds()[light_idx] & 0xFF;    // ID LSB
        entertainment_msg[msg_idx + 3]  = 0x00;                                     // Red MSB
        entertainment_msg[msg_idx + 4]  = 0x00;                                     // Red LSB;
        entertainment_msg[msg_idx + 5]  = 0x00;                                     // Green MSB;
        entertainment_msg[msg_idx + 6]  = 0x00;                                     // Green LSB;
        entertainment_msg[msg_idx + 7]  = 0x00;                                     // Blue MSB;
        entertainment_msg[msg_idx + 8]  = 0x00;                                     // Blue LSB;
    }

    /*-------------------------------------------------*\
    | Initialize mbedtls contexts                       |
    \*-------------------------------------------------*/
    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init( &entropy );

    /*-------------------------------------------------*\
    | Seed the Deterministic Random Bit Generator (RNG) |
    \*-------------------------------------------------*/
    int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);

    /*-------------------------------------------------*\
    | Parse certificate                                 |
    \*-------------------------------------------------*/
    ret = mbedtls_x509_crt_parse( &cacert, (const unsigned char *) mbedtls_test_cas_pem, mbedtls_test_cas_pem_len );

    /*-------------------------------------------------*\
    | Connect to the Hue bridge UDP server              |
    \*-------------------------------------------------*/
    ret = mbedtls_net_connect( &server_fd, bridge.getBridgeIP().c_str(), "2100", MBEDTLS_NET_PROTO_UDP );

    /*-------------------------------------------------*\
    | Configure defaults                                |
    \*-------------------------------------------------*/
    ret = mbedtls_ssl_config_defaults( &conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_DATAGRAM, MBEDTLS_SSL_PRESET_DEFAULT );

    mbedtls_ssl_conf_authmode( &conf, MBEDTLS_SSL_VERIFY_OPTIONAL );
    mbedtls_ssl_conf_ca_chain( &conf, &cacert, NULL );
    mbedtls_ssl_conf_rng( &conf, mbedtls_ctr_drbg_random, &ctr_drbg );

    /*-------------------------------------------------*\
    | Convert client key to binary array                |
    \*-------------------------------------------------*/
    std::vector<char> psk_binary = HexToBytes(bridge.getClientKey());

    /*-------------------------------------------------*\
    | Configure SSL pre-shared key and identity         |
    | PSK - binary array from client key                |
    | Identity - username (ASCII)                       |
    \*-------------------------------------------------*/
    ret = mbedtls_ssl_conf_psk(&conf, (const unsigned char *)&psk_binary[0], psk_binary.size(), (const unsigned char *)bridge.getUsername().c_str(), bridge.getUsername().length());

    /*-------------------------------------------------*\
    | Set up the SSL                                    |
    \*-------------------------------------------------*/
    ret = mbedtls_ssl_setup( &ssl, &conf );

    ret = mbedtls_ssl_set_hostname( &ssl, "localhost" );

    mbedtls_ssl_set_bio( &ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout );
    mbedtls_ssl_set_timer_cb( &ssl, &timer, mbedtls_timing_set_delay, mbedtls_timing_get_delay );

    do
    {
        ret = mbedtls_ssl_handshake( &ssl );
    } while( ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE );
}

PhilipsHueEntertainmentController::~PhilipsHueEntertainmentController()
{
    mbedtls_ssl_close_notify(&ssl);

    /*-------------------------------------------------*\
    | Signal the bridge to stop streaming               |
    \*-------------------------------------------------*/
    bridge.StopStreaming(std::to_string(group.getId()));
}

std::string PhilipsHueEntertainmentController::GetLocation()
{
    return(location);
}

std::string PhilipsHueEntertainmentController::GetName()
{
    return(group.getName());
}

std::string PhilipsHueEntertainmentController::GetVersion()
{
    return("");
}

std::string PhilipsHueEntertainmentController::GetManufacturer()
{
    return("");
}

std::string PhilipsHueEntertainmentController::GetUniqueID()
{
    return("");
}

unsigned int PhilipsHueEntertainmentController::GetNumLEDs()
{
    return(num_leds);
}

void PhilipsHueEntertainmentController::SetColor(RGBColor* colors)
{
    /*-------------------------------------------------*\
    | Fill in Entertainment Mode light data             |
    \*-------------------------------------------------*/
    for(unsigned int light_idx = 0; light_idx < num_leds; light_idx++)
    {
        unsigned int msg_idx            = HUE_ENTERTAINMENT_HEADER_SIZE + (light_idx * HUE_ENTERTAINMENT_LIGHT_SIZE);

        RGBColor color                  = colors[light_idx];
        unsigned char red               = RGBGetRValue(color);
        unsigned char green             = RGBGetGValue(color);
        unsigned char blue              = RGBGetBValue(color);

        entertainment_msg[msg_idx + 3]  = red;                                      // Red MSB
        entertainment_msg[msg_idx + 4]  = red;                                      // Red LSB;
        entertainment_msg[msg_idx + 5]  = green;                                    // Green MSB;
        entertainment_msg[msg_idx + 6]  = green;                                    // Green LSB;
        entertainment_msg[msg_idx + 7]  = blue;                                     // Blue MSB;
        entertainment_msg[msg_idx + 8]  = blue;                                     // Blue LSB;
    }

    mbedtls_ssl_write(&ssl, (const unsigned char *)entertainment_msg, entertainment_msg_size);
}
