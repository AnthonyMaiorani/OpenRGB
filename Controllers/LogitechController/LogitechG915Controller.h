/*-----------------------------------------*\
|  LogitechG915Controller.h                 |
|                                           |
|  Generic RGB Interface for Logitech G915  |
|  RGB Mechanical Gaming Keyboard           |
|                                           |
|  Cheerpipe      3/20/2021                 |
\*-----------------------------------------*/

#include "RGBController.h"

#include <string>
#include <hidapi/hidapi.h>

#define LOGITECH_G915_COMMIT_BYTE       0x7F
#define LOGITECH_READ_TIMEOUT           300     //Timeout in ms

#pragma once

enum
{
    LOGITECH_G915_ZONE_MODE_KEYBOARD        = 0x00,
    LOGITECH_G915_ZONE_MODE_LOGO            = 0x01,
    LOGITECH_G915_ZONE_MODE_MULTIMEDIA      = 0X02,
    LOGITECH_G915_ZONE_MODE_GKEYS           = 0x03,
    LOGITECH_G915_ZONE_MODE_MODIFIERS       = 0x04
};

enum
{
    LOGITECH_G915_ZONE_FRAME_TYPE_LITTLE    = 0x1F,
    LOGITECH_G915_ZONE_FRAME_TYPE_BIG       = 0x6F
};

enum
{
    LOGITECH_G915_ZONE_DIRECT_KEYBOARD      = 0x01,
    LOGITECH_G915_ZONE_DIRECT_MEDIA         = 0x02,
    LOGITECH_G915_ZONE_DIRECT_LOGO          = 0x10,
    LOGITECH_G915_ZONE_DIRECT_INDICATORS    = 0x40,
};

enum
{
    LOGITECH_G915_MODE_OFF                  = 0x00,
    LOGITECH_G915_MODE_STATIC               = 0x01,
    LOGITECH_G915_MODE_BREATHING            = 0x02,
    LOGITECH_G915_MODE_CYCLE                = 0x03,
    LOGITECH_G915_MODE_WAVE                 = 0x04,
    LOGITECH_G915_MODE_DIRECT               = 0x05,
};

/*---------------------------------------------------------------------------------------------*\
| Speed is 1000 for fast and 20000 for slow.                                                    |
| Values are multiplied by 100 later to give lots of GUI steps.                                 |
\*---------------------------------------------------------------------------------------------*/
enum
{
    LOGITECH_G915_SPEED_SLOWEST             = 0xC8,     /* Slowest speed                       */
    LOGITECH_G915_SPEED_NORMAL              = 0x32,     /* Normal speed                        */
    LOGITECH_G915_SPEED_FASTEST             = 0x0A,     /* Fastest speed                       */
};

class LogitechG915Controller
{
public:
    LogitechG915Controller(hid_device* dev_handle, bool wired);
    ~LogitechG915Controller();

    std::string GetSerialString();
    void        Commit();
    void        InitializeDirect();
    void        SetDirect
                    (
                    unsigned char       frame_type,
                    unsigned char *     frame_data
                    );
    void        SendSingleLed
                    (
                     unsigned char       keyCode,
                     unsigned char       r,
                     unsigned char       g,
                     unsigned char       b
                    );
    void        SetMode
                    (
                    unsigned char       mode,
                    unsigned short      speed,
                    unsigned char       red,
                    unsigned char       green,
                    unsigned char       blue
                    );

private:
    hid_device* dev_handle;
    char feature_4522_idx;
    char feature_8040_idx;
    char feature_8071_idx;
    char feature_8081_idx;

    void        SendDirectFrame
                    (
                    unsigned char       frame_type,
                    unsigned char *     frame_data
                    );

    void        SendMode
                    (
                    unsigned char       zone,
                    unsigned char       mode,
                    unsigned short      speed,
                    unsigned char       red,
                    unsigned char       green,
                    unsigned char       blue
                    );
    void        SendCommit();
};