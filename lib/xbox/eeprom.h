#ifndef EEPROM_H
#define EEPROM_H

#include "xbox.h"

typedef struct xbox_timezone_date
{
    uint8_t Month;     // 1-12, a 0 indicates there is no timezone information
    uint8_t Day;       // 1 = 1st occurrence of DayOfWeek up to 5 (5th or last)
    uint8_t DayOfWeek; // 0 = Sunday to 6 = Saturday
    uint8_t Hour;
} xbox_timezone_date_t;

// https://xboxdevwiki.net/EEPROM
typedef struct xbox_eeprom
{
    struct
    {
        uint8_t sha1_hash[20];                   // 0x00
        uint8_t confounder[8];                   // 0x14
        uint8_t hdd_key[16];                     // 0x1C
        uint32_t xbe_region;                     // 0x2C
    } encrypted;                                 //
                                                 //
    struct                                       //
    {                                            //
        uint32_t checksum;                       // 0x30 (Checksum from 0x34 to 0x5F)
        uint8_t serial_number[12];               // 0x34
        uint8_t mac_address[6];                  // 0x40
        uint8_t unknown[2];                      // 0x46
        uint8_t online_key[16];                  // 0x48
        uint32_t video_standard;                 // 0x58
        uint8_t padding[4];                      // 0x5C
    } factory_settings;                          //
                                                 //
    struct                                       //
    {                                            //
        uint32_t checksum;                       // 0x60 (Checksum from 0x64 to 0xBF)
        int32_t timezone_bias;                   // 0x64
        uint8_t timezone_std_name[4];            // 0x68
        uint8_t timezone_dlt_name[4];            // 0x6C
        uint8_t padding[8];                      // 0x70
        xbox_timezone_date_t timezone_std_start; // 0x78
        xbox_timezone_date_t timezone_dlt_start; // 0x7C
        uint32_t reserved2[2];                   // 0x80
        int32_t timezone_std_bias;               // 0x88
        int32_t timezone_dlt_bias;               // 0x8C
        uint32_t language;                       // 0x90
        uint32_t video_settings;                 // 0x94
        uint32_t audio_settings;                 // 0x98
        uint32_t parental_control_games;         // 0x9C
        uint32_t parental_control_passcode;      // 0xA0
        uint32_t parental_control_movies;        // 0xA4
        uint32_t xlive_ip_address;               // 0xA8
        uint32_t xlive_dns_address;              // 0xAC
        uint32_t xlive_default_gateway_address;  // 0xB0
        uint32_t xlive_subnet_mask;              // 0xB4
        uint32_t misc_flags;                     // 0xB8
        uint32_t dvd_region;                     // 0xBC
    } user_settings;                             //
                                                 //
    uint8_t unknown[0xFF - 0xC0];                // 0xC0
} xbox_eeprom_t;

#define XBOX_EEPROM_VIDEO_STANDARD_MASK   0x00000F00
#define XBOX_EEPROM_VIDEO_STANDARD_NTSC_M 0x00000100
#define XBOX_EEPROM_VIDEO_STANDARD_NTSC_J 0x00000200
#define XBOX_EEPROM_VIDEO_STANDARD_PAL_I  0x00000300
#define XBOX_EEPROM_VIDEO_STANDARD_PAL_M  0x00000400

#define XBOX_EEPROM_VIDEO_SETTINGS_480P       0x00080000
#define XBOX_EEPROM_VIDEO_SETTINGS_720P       0x00020000
#define XBOX_EEPROM_VIDEO_SETTINGS_1080I      0x00040000
#define XBOX_EEPROM_VIDEO_SETTINGS_WIDESCREEN 0x00010000
#define XBOX_EEPROM_VIDEO_SETTINGS_LETTERBOX  0x00100000
#define XBOX_EEPROM_VIDEO_SETTINGS_60HZ       0x00400000
#define XBOX_EEPROM_VIDEO_SETTINGS_50HZ       0x00800000

#define XBOX_EEPROM_AUDIO_SETTINGS_STEREO        0x00000000
#define XBOX_EEPROM_AUDIO_SETTINGS_MONO          0x00000001
#define XBOX_EEPROM_AUDIO_SETTINGS_SURROUND      0x00000002
#define XBOX_EEPROM_AUDIO_SETTINGS_SURROUND_AC3  0x00010000
#define XBOX_EEPROM_AUDIO_SETTINGS_SURROUND_DTS3 0x00020000

xbox_eeprom_t *xbox_eeprom_get();
int16_t xbox_eeprom_set(xbox_eeprom_t *eeprom);

#endif
