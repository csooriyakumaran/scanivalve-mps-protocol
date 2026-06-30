/******************************************************************************
*
*  mps-protocol.h
*
*   Core data structures, enumerations, and helper routines for working with
*   Scanivalve MPS-42xx pressure scanners (following firmware version 4.01)
*
*   This header is intended to be shared by both the emulator implementation
*   and any client applications that need to construct, send, receive, and
*   interpret MPS packets.
*
*   All multibyte fields are defined according to the device manual; on the
*   wire they may be transmitted in big-endian (network) byte order. This
*   header provides helpers for identifying the byte order and swapping if
*   needed.
*
*   Author:         C. Sooriyakumaran
*
*   Reference:      Scanivalve MPS4200 16, 32, and 64 Gen2 Hardware, Software, 
*                   and User Manual. Software Version 4.01 (Feb 2026)
*
******************************************************************************/
#ifndef MPS_PROTOCOL_H_
#define MPS_PROTOCOL_H_

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#include <stdint.h>
#include <string.h>
#include <stddef.h>

#define MPS_FIRMWARE_VERSION_MAJOR 4
#define MPS_FIRMWARE_VERSION_MINOR 01

#define MPS_STRINGIFY2(x) #x
#define MPS_STRINGIFY(x) MPS_STRINGIFY2(x)

#define MPS_FIRMWARE_VERSION_STRING \
    MPS_STRINGIFY(MPS_FIRMWARE_VERSION_MAJOR) "." \
    MPS_STRINGIFY(MPS_FIRMWARE_VERSION_MINOR) 

#define MPS_MAX_PRESSURE_CH_COUNT 64
#define MPS_MAX_TEMPERATURE_CH_COUNT 8
#define MPS_MAX_LABVIEW_ELEMENTS 66
#define MPS_MAX_BINARY_PACKET_SIZE 348

#define MPS_ADC_BITS 24
#define MPS_MAX_ADC_COUNTS ((uint32_t)((1u << MPS_ADC_BITS) - 1u))
#define MPS_MAX_SIGNED_ADC_COUNTS ((1 << (MPS_ADC_BITS -1)) - 1)
#define MPS_MIN_SIGNED_ADC_COUNTS (-(1 << (MPS_ADC_BITS -1)))

#if defined(__cplusplus)
    #define MPS_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
    #define MPS_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg) /* C11 */
#endif // __cplusplus

#if defined(__cplusplus)
    #define MPS_ALIGNOF(T) alignof(T)
#else
    #define MPS_ALIGNOF(T) _Alignof(T) /* C11 */
#endif // __cplusplus

/*
*
*  mps_stricmp
*
*   Case-insensitive, cross-platform string compare.
*
*   @param a           First null-terminated string.
*
*   @param b           Second null-terminated string.
*
*   @return result     Zero if the strings are equal (ignoring case),
*                     non-zero otherwise.
*
*/
static inline int mps_stricmp(const char* a, const char* b)
{
#if defined(_WIN32) || defined(_WIN64)
    return _stricmp(a, b);   /* Windows */
#else
    #include <strings.h>
    return strcasecmp(a, b); /* POSIX */
#endif
}

/* Device ID */
typedef enum MpsDeviceID
{
    MPS_4216 = 4216,
    MPS_4232 = 4232,
    MPS_4264 = 4264
} MpsDeviceID;

/*
*
*  Units index (Appendix A)
*
*   Values correspond to the index used in binary packets
*   ("Units index" in the manual).
*
*/
typedef enum MpsUnits
{
    MPS_UNITS_PSI = 0,
    MPS_UNITS_ATM,
    MPS_UNITS_BAR,
    MPS_UNITS_CMHG,
    MPS_UNITS_CMH2O,
    MPS_UNITS_DECIBAR,
    MPS_UNITS_FTH2O,
    MPS_UNITS_GCM2,
    MPS_UNITS_INHG,
    MPS_UNITS_INH2O,
    MPS_UNITS_KGCM2,
    MPS_UNITS_KGM2,
    MPS_UNITS_KIPIN2,
    MPS_UNITS_KNM2,
    MPS_UNITS_KPA,
    MPS_UNITS_MBAR,
    MPS_UNITS_MH2O,
    MPS_UNITS_MMHG,
    MPS_UNITS_MPA,
    MPS_UNITS_NCM2,
    MPS_UNITS_NM2,
    MPS_UNITS_OZFT2,
    MPS_UNITS_OZIN2,
    MPS_UNITS_PA,
    MPS_UNITS_PSF,
    MPS_UNITS_TORR,
    MPS_UNITS_USER,
    MPS_UNITS_RAW,
    MPS_UNITS_COUNT
} MpsUnits;

/*
*
*  Unit Conversions
*
*   MPS Pressure scanners converts raw ADC counts to the base units of PSI.
*   For other Units, a conversion is made internally before transmitting the
*   data. This table provides the conversion factor.
*
*   >    PRESSURE PSI   = COUNTS x CALIBRATION
*   >    PRESSURE UNITS = PSI    x CONVERSION-FACTOR
*
*   Values are transcribed verbatim from Appendix A of the manual (firmware
*   v4.01). Do not "correct" them for physical precision; matching the manual
*   preserves fidelity with the device's own internal conversions.
*
*   Note: a few manual entries are not internally consistent (e.g. MH2O is
*   not exactly CMH2O/100). This is a quirk of the manual, not a typo.
*
*/
static const float kMpsUnitConversion[MPS_UNITS_COUNT] = {
    1.00000000e+00f,  /*  0 - PSI     */
    6.80460000e-02f,  /*  1 - ATM     */
    6.89470000e-02f,  /*  2 - BAR     */
    5.17149000e+00f,  /*  3 - CMHG    */
    7.03080000e+01f,  /*  4 - CMH2O   */
    6.89470000e-01f,  /*  5 - DECIBAR */
    2.30670000e+00f,  /*  6 - FTH2O   */
    7.03060000e+01f,  /*  7 - GCM2    */
    2.03600000e+00f,  /*  8 - INHG    */
    2.76800000e+01f,  /*  9 - INH2O   */
    7.03070000e-02f,  /* 10 - KGCM2   */
    7.03069000e+02f,  /* 11 - KGM2    */
    1.00000000e-03f,  /* 12 - KIPIN2  */
    6.89476000e+00f,  /* 13 - KNM2    */
    6.89476000e+00f,  /* 14 - KPA     */
    6.89470000e+01f,  /* 15 - MBAR    */
    7.03090000e-01f,  /* 16 - MH2O    */
    5.17149000e+01f,  /* 17 - MMHG    */
    6.89476000e-03f,  /* 18 - MPA     */
    6.89476000e-01f,  /* 19 - NCM2    */
    6.89475977e+03f,  /* 20 - NM2     */
    2.30400000e+03f,  /* 21 - OZFT2   */
    1.60000000e+01f,  /* 22 - OZIN2   */
    6.89475977e+03f,  /* 23 - PA      */
    1.44000000e+02f,  /* 24 - PSF     */
    5.17149010e+01f,  /* 25 - TORR    */
    1.00000000e+00f,  /* 26 - USER    */
    1.00000000e+00f   /* 27 - RAW     */
};

/*
*
*  Units: enum <-> conversion factor
*
*   @param units        Units enumeration value.
*
*   @return             Conversion factor (float)
*                      
*/
static inline float mps_units_conversion_factor(MpsUnits units)
{
    if (units < 0 || units >= MPS_UNITS_COUNT)
        return 1.0f;
    return kMpsUnitConversion[(size_t)units];
}

/* Units: enum <-> string mapping (e.g. "PA", "PSI"). */
static const char* kMpsUnitStrings[MPS_UNITS_COUNT] = {
    "PSI",
    "ATM",
    "BAR",
    "CMHG",
    "CMH2O",
    "DECIBAR",
    "FTH2O",
    "GCM2",
    "INHG",
    "INH2O",
    "KGCM2",
    "KGM2",
    "KIPIN2",
    "KNM2",
    "KPA",
    "MBAR",
    "MH2O",
    "MMHG",
    "MPA",
    "NCM2",
    "NM2",
    "OZFT2",
    "OZIN2",
    "PA",
    "PSF",
    "TORR",
    "USER",
    "RAW"
};

/*
*
*  mps_units_to_string
*
*   Convert a units enum to its string token.
*
*   @param units       Units enumeration value.
*
*   @return token      Pointer to a null-terminated string, or NULL if the
*                      enum value is out of range.
*
*/
static inline const char* mps_units_to_string(MpsUnits units)
{
    if (units < 0 || units >= MPS_UNITS_COUNT)
        return NULL;
    return kMpsUnitStrings[(size_t)units];
}

/*
*
*  mps_units_from_string
*
*   Parse a units string token (e.g. "PA") into a `MpsUnits` enum.
*
*   @param s           Null-terminated units string.
*
*   @param out         Pointer to `MpsUnits` for the parsed result.
*
*   @return success    Returns 1 on success, 0 if the string is not
*                      recognized or arguments are invalid.
*
*/
static inline int mps_units_from_string(const char* s, MpsUnits* out)
{
    if (!s || !out) return 0;

    for (size_t i = 0; i < (size_t)MPS_UNITS_COUNT; ++i)
    {
        if (mps_stricmp(s, kMpsUnitStrings[i]) == 0)
        {
            *out = (MpsUnits)i;
            return 1;
        }
    }
    return 0;
}

/*
*
*  Data format (`SET FORMAT` command)
*
*   Enumerates the supported output data formats as configured by the
*   FORMAT command.
*
*/
typedef enum MpsDataFormat
{
    MPS_FMT_ASCII = 0,          /* A */
    MPS_FMT_CSV,                /* C */
    MPS_FMT_VT100,              /* F */
    MPS_FMT_BINARY,             /* B */
    MPS_FMT_LABVIEW,            /* L */
    MPS_FMT_COUNT
} MpsDataFormat;

/*
*
*  Formats: `MpsDataFormat` <-> single-char code
*
*   Conversion helpers for mapping `MpsDataFormat` to and from the single
*   character codes used by the FORMAT command (e.g. `'A'` for ASCII,
*   `'B'` for Binary).
*
*/
static const char kMpsFormatChars[MPS_FMT_COUNT] = {
    'A', /* ASCII  */
    'C', /* CSV    */
    'F', /* VT100  */
    'B', /* BINARY */
    'L', /* LABVIEW*/
};

/*
*
*  mps_format_to_char
*
*   Convert a MpsDataFormat value to its single-character code.
*
*   @param fmt         `MpsDataFormat` value.
*
*   @return code       Character code for the format, or `'\0'` if the
*                      value is out of range.
*
*/
static inline char mps_format_to_char(MpsDataFormat fmt)
{
    if (fmt < 0 || fmt >= MPS_FMT_COUNT)
        return '\0';
    return kMpsFormatChars[(int)fmt];
}

/*
*
*  mps_format_from_char
*
*   Parse a single-character format code into a `MpsDataFormat` value.
*
*   @param c           Single-character code (case-insensitive).
*
*   @param out_fmt     Pointer to `MpsDataFormat` for the parsed result.
*
*   @return success    Returns `1` on success, `0` if the character is not
*                      recognized or arguments are invalid.
*
*/
static inline int mps_format_from_char(char c, MpsDataFormat* out_fmt)
{
    if (!out_fmt) return 0;

    switch (c)
    {
    case 'A': case 'a': *out_fmt = MPS_FMT_ASCII;              return 1;
    case 'C': case 'c': *out_fmt = MPS_FMT_CSV;                return 1;
    case 'F': case 'f': *out_fmt = MPS_FMT_VT100;              return 1;
    case 'B': case 'b': *out_fmt = MPS_FMT_BINARY;             return 1;
    case 'L': case 'l': *out_fmt = MPS_FMT_LABVIEW;            return 1;
    default: break;
    }
    return 0;
}


/* Device status (`STATUS` command) */
typedef enum MpsStatus
{
    MPS_STATUS_READY = 0,
    MPS_STATUS_SCAN,
    MPS_STATUS_CALZ,
    MPS_STATUS_SAVE,
    MPS_STATUS_CALVAL,
    MPS_STATUS_PGM,
    MPS_STATUS_COUNT
} MpsStatus;

/*
*
*  Status: `MpsStatus` <-> string
*
*   Conversion table for mapping `MpsStatus` values to and from their
*   string tokens (e.g. `"READY"`, `"SCAN"`).
*
*/
static const char* kMpsStatusString[MPS_STATUS_COUNT] = {
    "READY", "SCAN", "CALZ", "SAVE", "CALVAL", "PGM"
};

/*
*
*  mps_status_to_string
*
*   Convert an `MpsStatus` value to its string token.
*
*   @param status      `MpsStatus` value.
*
*   @return token      Pointer to a null-terminated string, or `NULL` if the
*                      value is out of range.
*
*/
static inline const char* mps_status_to_string(MpsStatus status)
{
    if (status < 0 || status >= MPS_STATUS_COUNT)
        return (const char*)0;

    return kMpsStatusString[(int)status];
}

/*
*
*  mps_status_from_string
*
*   Parse a status string token (e.g. `"READY"`) into an `MpsStatus` value.
*
*   @param s           Null-terminated status string.
*
*   @param out_status  Pointer to `MpsStatus` for the parsed result.
*
*   @return success    Returns `1` on success, `0` if the string is not
*                      recognized or arguments are invalid.
*
*/
static inline int mps_status_from_string(const char* s, MpsStatus* out_status)
{
    if (!s || !out_status) return 0;

    for (size_t i = 0; i < (size_t)MPS_STATUS_COUNT; ++i)
    {
        if (mps_stricmp(s, kMpsStatusString[i]) == 0)
        {
            *out_status = (MpsStatus)i;
            return 1;
        }

    }
    return 0;
}

/* `MpsSimFlags` -> enum `MpsSimFlags_` */
typedef int MpsSimFlags;

enum MpsSimFlags_
{
    MPS_SIM_NONE           = 0x00,
    MPS_SIM_SHOW_PTP_DIFFS = 0x04,
    MPS_SIM_SIMULATED_64CH = 0x40
};

/* `MpsValveStatus`: e.g., `Px`=`0`, `Cal`=`1`*/
typedef enum MpsValveStatus
{
    MPS_VALVE_STATUS_PX  = 0,
    MPS_VALVE_STATUS_CAL = 1,
    MPS_VALVE_STATUS_COUNT
} MpsValveStatus;

static const char* kMpsValveStatusString[MPS_VALVE_STATUS_COUNT] = {
    "Px", "Cal"
};

/*
*  mps_valve_status_to_string
*
*   Convert an `MpsValveStatus` value to its string token.
*
*   @param status      `MpsValveStatus` value.
*
*   @return token      Pointer to a null-terminated string, or `NULL` if the
*                      value is out of range.
*
*/
static inline const char* mps_valve_status_to_string(MpsValveStatus status)
{
    if (status < 0 || status >= MPS_VALVE_STATUS_COUNT)
        return NULL;

    return kMpsValveStatusString[(size_t)status];
}


/* Binary Packet Types: defined in Section 7 of the Manual*/
typedef enum MpsBinaryPacketType
{
    MPS_PKT_UNDEFINED   = 0,
    MPS_PKT_16_TYPE     = 0x5D,
    MPS_PKT_32_TYPE     = 0x65,
    MPS_PKT_64_TYPE     = 0x6D,
    MPS_PKT_16_RAW_TYPE = 0x5B,
    MPS_PKT_32_RAW_TYPE = 0x63,
    MPS_PKT_64_RAW_TYPE = 0x69,
    MPS_PKT_LEGACY_TYPE = 0x0A,
} MpsBinaryPacketType;

#define MPS_TYPE_FITS_U8(x) ((x) >= 0 && (x) <= 0xFF)
MPS_STATIC_ASSERT(
    MPS_TYPE_FITS_U8(MPS_PKT_16_TYPE) && MPS_TYPE_FITS_U8(MPS_PKT_16_RAW_TYPE) &&
    MPS_TYPE_FITS_U8(MPS_PKT_32_TYPE) && MPS_TYPE_FITS_U8(MPS_PKT_32_RAW_TYPE) &&
    MPS_TYPE_FITS_U8(MPS_PKT_64_TYPE) && MPS_TYPE_FITS_U8(MPS_PKT_64_RAW_TYPE) &&
    MPS_TYPE_FITS_U8(MPS_PKT_LEGACY_TYPE),
    "MPS packet type must fit one byte; byte-order auto-detect depends on it"
);

/* Binary Packet Size: defined in Section 7 of the Manual*/
typedef enum MpsBinaryPacketSize
{
    MPS_PKT_16_SIZE         = 96,
    MPS_PKT_32_SIZE         = 160,
    MPS_PKT_64_SIZE         = 304,
    MPS_PKT_LEGACY_SIZE     = 348,
    MPS_PKT_16_LABVIEW_SIZE = 72,
    MPS_PKT_32_LABVIEW_SIZE = 136,
    MPS_PKT_64_LABVIEW_SIZE = 264,
} MpsBinaryPacketSize;

/*
 *  mps_packet_size_from_type
 *
 *    Returns the packet size based on the `MpsBinaryPacketType`
 *
 *    @param t      `MpsBinaryPacketType`
 *
 *    @returns      a `uint16_t` packet size
 */
static inline uint16_t mps_packet_size_from_type(MpsBinaryPacketType t)
{
    switch (t)
    {
        case MPS_PKT_16_TYPE: case MPS_PKT_16_RAW_TYPE: return MPS_PKT_16_SIZE;
        case MPS_PKT_32_TYPE: case MPS_PKT_32_RAW_TYPE: return MPS_PKT_32_SIZE;
        case MPS_PKT_64_TYPE: case MPS_PKT_64_RAW_TYPE: return MPS_PKT_64_SIZE;
        case MPS_PKT_LEGACY_TYPE: return MPS_PKT_LEGACY_SIZE;
        case MPS_PKT_UNDEFINED: default: return 0u;
    }
}

/*
 *  Returns the packet size for a Labview packet based on the pressure channel count
 *
 *  Labview packets are send if `SET FORMAT B L`
 *
 */
static inline uint16_t mps_labview_packet_size(uint32_t ch)
{
    switch (ch)
    {
        case 16: return MPS_PKT_16_LABVIEW_SIZE;
        case 32: return MPS_PKT_32_LABVIEW_SIZE;
        case 64: return MPS_PKT_64_LABVIEW_SIZE;
        default: return 0u;
    }
}

/*  MPS-4216, RAW count packet */
typedef struct Mps16RawPacket
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[4];
    int32_t counts[16];
} Mps16RawPacket;


/*  MPS-4232, RAW count packet */
typedef struct Mps32RawPacket
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[4];
    int32_t counts[32];
} Mps32RawPacket;


/*  MPS-4264, RAW count packet */
typedef struct Mps64RawPacket
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[8];
    int32_t counts[64];
} Mps64RawPacket;

/*  MPS-4216, EU units packet */
typedef struct Mps16Packet
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[4];
    float pressure[16];
} Mps16Packet;


/*  MPS-4232, EU units packet */
typedef struct Mps32Packet
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[4];
    float pressure[32];
} Mps32Packet;


/*  MPS-4264, EU units packet */
typedef struct Mps64Packet
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[8];
    float pressure[64];
} Mps64Packet;

/*
 *
 * MPS-4264 Legacy packet
 *
 *   Used by MPS-4264Gen1 and by MPS-4200 with `SET SIM 0x40` (with zeros
 *   padding unused channels)
 *
 */
typedef struct MpsLegacyPacket
{
    int32_t type;                      /* 0x0A */
    int32_t size;                      /* 348 bytes */
    int32_t frame;                     /* frame number */
    int32_t serial_number;             /* module serial number (SN) */

    float framerate;                   /* scanning rate in Hz */
    int32_t valve_status;              /* 0 = Px, 1 = Cal */
    int32_t unit_index;                /* MpsUnits index */
    float unit_conversion;             /* conversion factor from PSI to selected units */

    uint32_t ptp_scan_start_time_s;    /* Scan start time (SST) in seconds (epoch) */
    uint32_t ptp_scan_start_time_ns;   /* Scan start time (SST) in nanoseconds */
    uint32_t external_trigger_time_us; /* External trigger time in microseconds */

    float temperature[8];              /* array of 8 RTD temperatures */

    /* Pressures: array of 64 pressure or count values.
     * The manual allows float or integer depending on units (EU vs RAW). */
    union {
        float pressure[64];            /* pressures in EU units */
        int32_t counts[64];            /* Raw ADC counts if units == RAW */
    };

    uint32_t frame_time_s;             /* frame time in seconds, relative to SST */
    uint32_t frame_time_ns;            /* frame time in nanoseconds, relative to SST */

    uint32_t external_trigger_time_s;  /* external trigger time in seconds (rel. to SST) */
    uint32_t external_trigger_time_ns; /* external trigger time in nanoseconds (rel. to SST) */
} MpsLegacyPacket;


/* 16-Channel Labview packet */
typedef struct Mps16LabviewPacket
{
    float frame;        /* frame number */
    float temperature;  /* average temperature */
    float pressure[16]; /* 16 channels of pressure data */
} Mps16LabviewPacket;

/* 32-Channel Labview packet */
typedef struct Mps32LabviewPacket
{
    float frame;        /* frame number */
    float temperature;  /* average temperature */
    float pressure[32]; /* 32 channels of pressure data */
} Mps32LabviewPacket;

/* 64-Channel Labview packet */
typedef struct Mps64LabviewPacket
{
    float frame;        /* frame number */
    float temperature;  /* average MPS temperature */
    float pressure[64]; /* 64 channels of pressure data */
} Mps64LabviewPacket;

typedef union MpsPacket
{
    uint8_t bytes[MPS_MAX_BINARY_PACKET_SIZE];
    MpsLegacyPacket    legacy64;
    Mps64Packet        eu64;
    Mps32Packet        eu32;
    Mps16Packet        eu16;
    Mps64RawPacket     raw64;
    Mps32RawPacket     raw32;
    Mps16RawPacket     raw16;
    Mps64LabviewPacket lab64;
    Mps32LabviewPacket lab32;
    Mps16LabviewPacket lab16;
} MpsPacket;

MPS_STATIC_ASSERT(sizeof(MpsPacket)               >= MPS_MAX_BINARY_PACKET_SIZE,               "MpsPacket union must >= 348 bytes");
MPS_STATIC_ASSERT(sizeof(Mps16Packet)             == MPS_PKT_16_SIZE,                          "Mps16Packet     must be 96 bytes");
MPS_STATIC_ASSERT(sizeof(Mps32Packet)             == MPS_PKT_32_SIZE,                          "Mps32Packet     must be 160 bytes");
MPS_STATIC_ASSERT(sizeof(Mps64Packet)             == MPS_PKT_64_SIZE,                          "Mps64Packet     must be 304 bytes");
MPS_STATIC_ASSERT(sizeof(Mps16RawPacket)          == MPS_PKT_16_SIZE,                          "Mps16RawPacket  must be 96 bytes");
MPS_STATIC_ASSERT(sizeof(Mps32RawPacket)          == MPS_PKT_32_SIZE,                          "Mps32RawPacket  must be 160 bytes");
MPS_STATIC_ASSERT(sizeof(Mps64RawPacket)          == MPS_PKT_64_SIZE,                          "Mps64RawPacket  must be 304 bytes");
MPS_STATIC_ASSERT(sizeof(MpsLegacyPacket)         == MPS_PKT_LEGACY_SIZE,                      "MpsBinaryPacket must be 348 bytes");
MPS_STATIC_ASSERT(sizeof(Mps16LabviewPacket)      == MPS_PKT_16_LABVIEW_SIZE,                  "MpsLabviewFrame must be 72 bytes");
MPS_STATIC_ASSERT(sizeof(Mps32LabviewPacket)      == MPS_PKT_32_LABVIEW_SIZE,                  "MpsLabviewFrame must be 136 bytes");
MPS_STATIC_ASSERT(sizeof(Mps64LabviewPacket)      == MPS_PKT_64_LABVIEW_SIZE,                  "MpsLabviewFrame must be 264 bytes");
MPS_STATIC_ASSERT(MPS_ALIGNOF(MpsPacket)          == 4 && sizeof(MpsPacket)          % 4 == 0, "MpsPacket union must be 4-byte aligned");
MPS_STATIC_ASSERT(MPS_ALIGNOF(Mps16Packet)        == 4 && sizeof(Mps16Packet)        % 4 == 0, "Mps16Packet     must be 4-byte aligned");
MPS_STATIC_ASSERT(MPS_ALIGNOF(Mps32Packet)        == 4 && sizeof(Mps32Packet)        % 4 == 0, "Mps32Packet     must be 4-byte aligned");
MPS_STATIC_ASSERT(MPS_ALIGNOF(Mps64Packet)        == 4 && sizeof(Mps64Packet)        % 4 == 0, "Mps64Packet     must be 4-byte aligned");
MPS_STATIC_ASSERT(MPS_ALIGNOF(Mps16RawPacket)     == 4 && sizeof(Mps16RawPacket)     % 4 == 0, "Mps16RawPacket  must be 4-byte aligned");
MPS_STATIC_ASSERT(MPS_ALIGNOF(Mps32RawPacket)     == 4 && sizeof(Mps32RawPacket)     % 4 == 0, "Mps32RawPacket  must be 4-byte aligned");
MPS_STATIC_ASSERT(MPS_ALIGNOF(Mps64RawPacket)     == 4 && sizeof(Mps64RawPacket)     % 4 == 0, "Mps64RawPacket  must be 4-byte aligned");
MPS_STATIC_ASSERT(MPS_ALIGNOF(MpsLegacyPacket)    == 4 && sizeof(MpsLegacyPacket)    % 4 == 0, "MpsBinaryPacket must be 4-byte aligned");
MPS_STATIC_ASSERT(MPS_ALIGNOF(Mps16LabviewPacket) == 4 && sizeof(Mps16LabviewPacket) % 4 == 0, "MpsLabviewFrame must be 4-byte aligned");
MPS_STATIC_ASSERT(MPS_ALIGNOF(Mps32LabviewPacket) == 4 && sizeof(Mps32LabviewPacket) % 4 == 0, "MpsLabviewFrame must be 4-byte aligned");
MPS_STATIC_ASSERT(MPS_ALIGNOF(Mps64LabviewPacket) == 4 && sizeof(Mps64LabviewPacket) % 4 == 0, "MpsLabviewFrame must be 4-byte aligned");

/*
*  Contains type, pressure/temperature channel count, and
*  the size of the packet in bytes
*
*/
typedef struct MpsBinaryPacketInfo
{
    uint8_t type;
    uint8_t pressure_channel_count;
    uint8_t temperature_channel_count;
    uint16_t size_bytes;
} MpsBinaryPacketInfo;

/* Table of defined packet types and info */
static const MpsBinaryPacketInfo kMpsBinaryPacketInfoTable[] = {
    {MPS_PKT_16_RAW_TYPE, 16, 4, MPS_PKT_16_SIZE},
    {MPS_PKT_32_RAW_TYPE, 32, 4, MPS_PKT_32_SIZE},
    {MPS_PKT_64_RAW_TYPE, 64, 8, MPS_PKT_64_SIZE},
    {MPS_PKT_16_TYPE,     16, 4, MPS_PKT_16_SIZE},
    {MPS_PKT_32_TYPE,     32, 4, MPS_PKT_32_SIZE},
    {MPS_PKT_64_TYPE,     64, 8, MPS_PKT_64_SIZE},
    {MPS_PKT_LEGACY_TYPE, 64, 8, MPS_PKT_LEGACY_SIZE},
};


#define MPS_BINARY_PACKET_INFO_COUNT (sizeof(kMpsBinaryPacketInfoTable)/sizeof(kMpsBinaryPacketInfoTable[0]))

/*
*
*  Retreive a pointer a pre-populated `MpsBinaryPacketInfo` struct from the type
*
*/
static inline const MpsBinaryPacketInfo* mps_get_binary_packet_info_by_type(uint8_t type)
{
    for (size_t i = 0; i < (size_t)MPS_BINARY_PACKET_INFO_COUNT; ++i)
    {
        if (kMpsBinaryPacketInfoTable[i].type == type)
            return &kMpsBinaryPacketInfoTable[i];
    }
    return (const MpsBinaryPacketInfo*)0;
}

typedef enum MpsByteOrder
{
    MPS_BYTE_ORDER_UNKNOWN = 0,
    MPS_BYTE_ORDER_LE,
    MPS_BYTE_ORDER_BE
} MpsByteOrder;

typedef struct MpsStream
{
    const MpsBinaryPacketInfo* info;
    MpsByteOrder order;
    int byte_swap_needed;

} MpsStream;

typedef enum MpsParseStatus
{
    MPS_PARSE_OK = 0,
    MPS_PARSE_ERR_NULL,         /* buf or out was NULL */
    MPS_PARSE_ERR_SHORT,        /* len smaller than the packet needs */
    MPS_PARSE_ERR_UNKNOWN_TYPE  /* type field not a recognized packet */
} MpsParseStatus;

static inline MpsByteOrder mps_host_byte_order(void)
{
    const uint16_t one = 1u;
    return (*(const uint8_t*)&one == 1u) ? MPS_BYTE_ORDER_LE : MPS_BYTE_ORDER_BE;
}

/* since the type of packet is always encoded as 1 byte, we can therefore check which byte it is
 * and consequently determine the byte ordering of the data */
static inline MpsParseStatus mps_peek(const uint8_t* buf, size_t len,
                                      const MpsBinaryPacketInfo** out_info,
                                      MpsByteOrder* out_order,
                                      int* byte_swap_needed)
{
    if (!buf || !out_info || !out_order || !byte_swap_needed) return MPS_PARSE_ERR_NULL;
    if (len < 4) return MPS_PARSE_ERR_SHORT;

    const MpsBinaryPacketInfo* le = (buf[1]==0 && buf[2]==0 && buf[3]==0) ? mps_get_binary_packet_info_by_type(buf[0]) : NULL;
    const MpsBinaryPacketInfo* be = (buf[0]==0 && buf[1]==0 && buf[2]==0) ? mps_get_binary_packet_info_by_type(buf[3]) : NULL;

    MpsByteOrder host = mps_host_byte_order();
    if (le && !be) { *out_info = le; *out_order = MPS_BYTE_ORDER_LE; *byte_swap_needed = (host != *out_order); return MPS_PARSE_OK; }
    if (be && !le) { *out_info = be; *out_order = MPS_BYTE_ORDER_BE; *byte_swap_needed = (host != *out_order); return MPS_PARSE_OK; }

    /* all-zero (type 0), unknown, or ambiguous */
    *out_info  = NULL;
    *out_order = MPS_BYTE_ORDER_UNKNOWN;
    *byte_swap_needed = 0;
    return MPS_PARSE_ERR_UNKNOWN_TYPE;
}

/* Every MPS binary packet field is a 4-byte word, so byte-order conversion is a 
 * uniform per-word reversal - no struct layout knowledge required. Call only when 
 * the packet's order differs from the host. Mutates buf; idempotent only in pairs
 * (running twice restores the original order) */
static inline void mps_byte_swap_inplace(uint8_t* buf, size_t size_bytes)
{
    if(!buf) return;
    for (size_t i = 0; i + 4 <= size_bytes; i += 4)
    {
        uint8_t a = buf[i+0], b = buf[i+1];
        buf[i+0] = buf[i+3];
        buf[i+1] = buf[i+2];
        buf[i+2] = b;
        buf[i+3] = a;
    }
}

/* Read-only-safe counterpart to mps_byte_swap_inplace: copy a packet from src 
 * into dst, ononverting to host order when byte_swap is set. src is untouch, so 
 * this is the path for shared memory / mmap'd files. With byte_swap == 0 it is 
 * a plain memcpy. If dst is a real (aligned) packet struct, the field reads
 * afterward are well-defined - no cast over a raw byte buffer */
static inline void mps_copy_packet(uint8_t* dst, const uint8_t* src, size_t size_bytes, int byte_swap)
{
    if (!byte_swap) { memcpy(dst, src, size_bytes); return; }
    for (size_t i = 0; i + 4 <= size_bytes; i +=4)
    {
        dst[i+0] = src[i+3];
        dst[i+1] = src[i+2];
        dst[i+2] = src[i+1];
        dst[i+3] = src[i+0];
    }
}

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // MPS_PROTOCOL_H_ 
