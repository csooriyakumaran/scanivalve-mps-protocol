#ifndef MPS_PROTOCOL_H_
#define MPS_PROTOCOL_H_

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
*   wire they are transmitted in big-endian (network) byte order. This
*   header only defines the logical layout. Callers are responsible for
*   performing host<->network byte order conversions as needed when
*   serializing/deserializing.
*
*   Author:         C. Sooriyakumaran
*
*   Reference:      Scanivalve MPS4200 16, 32, and 64 Gen2 Hardware, Software, 
*                   and User Manual. Software Version 4.01 (Feb 2026)
*
******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <string.h>

#define MPS_FIRMWARE_VERSION_MAJOR 4
#define MPS_FIRMWARE_VERSION_MINOR 01

#define MPS_STRINGIFY2(x) #x
#define MPS_STRINGIFY(x) MPS_STRINGIFY2(x)

#define MPS_FIRMWARE_VERSION_STRING \
    MPS_STRINGIFY(MPS_FIRMWARE_VERSION_MAJOR) "." \
    MPS_STRINGIFY(MPS_FIRMWARE_VERSION_MINOR) 

static const char* kMpsFirmwareVersionString = MPS_FIRMWARE_VERSION_STRING;


#define MPS_MAX_PRESSURE_CH_COUNT 64
#define MPS_MAX_TEMPERATURE_CH_COUNT 8
#define MPS_MAX_LABVIEW_ELEMENTS 66
#define MPS_MAX_BINARY_PACKET_SIZE 348

#define MPS_ADC_BITS 24
#define MPS_MAX_ADC_COUNTS ((uint32_t)((1u << MPS_ADC_BITS) - 1u))
#define MPS_MAX_SIGNED_ADC_COUNTS ((1 << (MPS_ADC_BITS -1)) - 1)
#define MPS_MIN_SIGNED_ADC_COUNTS (-(1 << (MPS_ADC_BITS -1)))


/******************************************************************************
*
*  mps_stricpmp
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
******************************************************************************/
static int mps_stricpmp(const char* a, const char* b)
{
#if defined(_WIN32) || defined(_WIN64)
    return _stricmp(a, b);   /* Windows */
#else
    return strcasecmp(a, b); /* POSIX */
#endif
}

/******************************************************************************
*
*  Device ID
*
******************************************************************************/

typedef enum MpsDeviceID
{
    MPS_4216 = 4216,
    MPS_4232 = 4232,
    MPS_4264 = 4264
} MpsDeviceID;


/******************************************************************************
*
*  Units index (Appendix A)
*
*   This is a subset of the ScanUnits enum used in the emulator, exposed as
*   a C-friendly enum. Values correspond to the index used in binary packets
*   ("Units index" in the manual).
*
******************************************************************************/

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
    MPS_UNITS_KNM2,
    MPS_UNITS_KGM2,
    MPS_UNITS_KGCM2,
    MPS_UNITS_KPA,
    MPS_UNITS_KIPIN2,
    MPS_UNITS_MPA,
    MPS_UNITS_MBAR,
    MPS_UNITS_MH2O,
    MPS_UNITS_MMHG,
    MPS_UNITS_NM2,
    MPS_UNITS_NCM2,
    MPS_UNITS_OZIN2,
    MPS_UNITS_OZFT2,
    MPS_UNITS_PA,
    MPS_UNITS_PSF,
    MPS_UNITS_TORR,
    MPS_UNITS_USER,
    MPS_UNITS_RAW,
    MPS_UNITS_COUNT
} MpsUnits;

static const float kMpsUnitConversion[MPS_UNITS_COUNT] = {
    1.0f,         /* PSI  */
    0.068046f,    /* ATM  */
    0.068947f,    /* BAR  */
    5.17149f,     /* CMHG */
    70.308f,      /* CMH2O */
    0.68947f,     /* DECIBAR */
    2.3067f,      /* FTH2O */
    70.306f,      /* GCM2 */
    2.0360f,      /* INHG */
    27.680f,      /* INH2O */
    0.070307f,    /* KNM2 */
    703.069f,     /* KGM2 */
    0.001f,       /* KGCM2 */
    6.89476f,     /* KPA */
    6.89476f,     /* KIPIN2 */
    68.947f,      /* MPA */
    0.70309f,     /* MBAR */
    51.7149f,     /* MH2O */
    0.00689476f,  /* MMHG */
    0.689476f,    /* NM2 */
    6894.759766f, /* NCM2 */
    2304.00f,     /* OZIN2 */
    16.0f,        /* OZFT2 */
    6894.759766f, /* PA */
    144.00f,      /* PSF */
    51.71490f,    /* TORR */
    1.0f,         /* USER */
    1.0f          /* RAW  */
};

/******************************************************************************
*
*  Units: enum <-> conversion factor
*
*   @param units        Units enumeration value.
*
*   @return             Conversion factor (float)
*                      
******************************************************************************/
static inline float mps_units_conversion_factor(MpsUnits units)
{
    if (units < 0 || units >= MPS_UNITS_COUNT)
        return 1.0f;
    return kMpsUnitConversion[(int)units];
}

/******************************************************************************
*
*  Units: enum <-> string mapping (e.g. "PA", "PSI").
*
******************************************************************************/

static const char* kMpsUnitStrings[MPS_UNITS_COUNT] = {
"PSI",   "ATM",  "BAR",    "CMHG",  "CMH2O", "DECIBAR",
"FTH2O", "GCM2", "INHG",   "INH2O", "KNM2",  "KGM2",
"KGCM2", "KPA",  "KIPIN2", "MPA",   "MBAR",  "MH2O",
"MMHG",  "NM2",  "NCM2",   "OZIN2", "OZFT2", "PA",
"PSF",   "TORR", "USER",   "RAW"
};

/******************************************************************************
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
******************************************************************************/
static inline const char* mps_units_to_string(MpsUnits units)
{
    if (units < 0 || units >= MPS_UNITS_COUNT)
        return (const char*)0;
    return kMpsUnitStrings[(int)units];
}

/******************************************************************************
*
*  mps_units_from_string
*
*   Parse a units string token (e.g. "PA") into a MpsUnits enum.
*
*   @param s           Null-terminated units string.
*
*   @param out         Pointer to MpsUnits for the parsed result.
*
*   @return success    Returns 1 on success, 0 if the string is not
*                      recognized or arguments are invalid.
*
******************************************************************************/
static inline int mps_units_from_string(const char* s, MpsUnits* out)
{
    if (!s || !out)
        return 0;

    for (int i = 0; i < (int)MPS_UNITS_COUNT; ++i)
    {
        if (mps_stricpmp(s, kMpsUnitStrings[i]) == 0)
        {
            *out = (MpsUnits)i;
            return 1;
        }
    }
    return 0;
}


/******************************************************************************
*
*  Data format (FORMAT command)
*
*   Enumerates the supported output data formats as configured by the
*   FORMAT command.
*
******************************************************************************/

typedef enum MpsDataFormat
{
    MPS_FMT_ASCII = 0,          /* A */
    MPS_FMT_CSV,                /* C */
    MPS_FMT_VT100,              /* F */
    MPS_FMT_BINARY,             /* B */
    MPS_FMT_LABVIEW,            /* L */
    MPS_FMT_COUNT
} MpsDataFormat;

/******************************************************************************
*
*  Formats: enum <-> single-char code
*
*   Conversion helpers for mapping MpsDataFormat to and from the single
*   character codes used by the FORMAT command (e.g. 'A' for ASCII,
*   'B' for Binary).
*
******************************************************************************/

static const char kMpsFormatChars[MPS_FMT_COUNT] = {
    'A', /* ASCII  */
    'C', /* CSV    */
    'F', /* VT100  */
    'B', /* BINARY */
    'L', /* LABVIEW*/
};

/******************************************************************************
*
*  mps_format_to_char
*
*   Convert a MpsDataFormat value to its single-character code.
*
*   @param fmt         MpsDataFormat value.
*
*   @return code       Character code for the format, or '\0' if the
*                      value is out of range.
*
******************************************************************************/
static inline char mps_format_to_char(MpsDataFormat fmt)
{
    if (fmt < 0 || fmt >= MPS_FMT_COUNT)
        return '\0';
    return kMpsFormatChars[(int)fmt];
}

/******************************************************************************
*
*  mps_format_from_char
*
*   Parse a single-character format code into a MpsDataFormat value.
*
*   @param c           Single-character code (case-insensitive).
*
 *  @param out_fmt     Pointer to MpsDataFormat for the parsed result.
*
*   @return success    Returns 1 on success, 0 if the character is not
*                      recognized or arguments are invalid.
*
******************************************************************************/
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


/******************************************************************************
*
*  Device status (STATUS command)
*
*   Enumerates high-level device states reported by the STATUS command.
*
******************************************************************************/

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

/******************************************************************************
*
*  Status: enum <-> string
*
*   Conversion helpers for mapping MpsStatus values to and from their
*   string tokens (e.g. "READY", "SCAN").
*
******************************************************************************/

static const char* kMpsStatusString[MPS_STATUS_COUNT] = {
    "READY", "SCAN", "CALZ", "SAVE", "CALVAL", "PGM"
};

/******************************************************************************
*
*  mps_status_to_string
*
*   Convert a MpsStatus value to its string token.
*
*   @param status      MpsStatus value.
*
*   @return token      Pointer to a null-terminated string, or NULL if the
*                      value is out of range.
*
******************************************************************************/
static inline const char* mps_status_to_string(MpsStatus status)
{
    if (status < 0 || status >= MPS_STATUS_COUNT)
        return (const char*)0;

    return kMpsStatusString[(int)status];
}

/******************************************************************************
*
*  mps_status_from_string
*
*   Parse a status string token (e.g. "READY") into a MpsStatus value.
*
*   @param s           Null-terminated status string.
*
*   @param out_status  Pointer to MpsStatus for the parsed result.
*
*   @return success    Returns 1 on success, 0 if the string is not
*                      recognized or arguments are invalid.
*
******************************************************************************/
static inline int mps_status_from_string(const char* s, MpsStatus* out_status)
{
    int i;
    if (!s || !out_status)
        return 0;

    for (i = 0; i < (int)MPS_STATUS_COUNT; ++i)
    {
        if (mps_stricpmp(s, kMpsStatusString[i]) == 0)
        {
            *out_status = (MpsStatus)i;
            return 1;
        }

    }
    return 0;
}

typedef enum MpsSimFlags
{
    MPS_SIM_NONE = 0x00,
    MPS_SIM_SHOW_PTP_DIFFS = 0x04,
    MPS_SIM_SIMULATED_64CH = 0x40
} MpsSimFlags;

typedef enum MpsValveStatus
{
    MPS_VALVE_STATUS_PX = 0,
    MPS_VALVE_STATUS_CAL = 1,
    MPS_VALVE_STATUS_COUNT
} MpsValveStatus;

static const char* kMpsValveStatusString[MPS_VALVE_STATUS_COUNT] = {
    "Px", "Cal"
};

static inline const char* mps_valve_status_to_string(MpsValveStatus status)
{
    if (status < 0 || status >= MPS_VALVE_STATUS_COUNT)
        return (const char*)0;

    return kMpsValveStatusString[(int)status];
}


/******************************************************************************
*  Data Packet Types
******************************************************************************/

typedef enum MpsBinaryPacketType
{
    MPS_PKT_UNDEFINED = 0,
    MPS_PKT_16_TYPE = 0x5D,
    MPS_PKT_32_TYPE = 0x65,
    MPS_PKT_64_TYPE = 0x6D,
    MPS_PKT_16_RAW_TYPE = 0x5B,
    MPS_PKT_32_RAW_TYPE = 0x63,
    MPS_PKT_64_RAW_TYPE = 0x69,
    MPS_PKT_LEGACY_TYPE = 0x0A,
} MpsBinaryPacketType;

typedef enum MpsBinaryPacketSize
{
    MPS_PKT_16_SIZE= 96,
    MPS_PKT_32_SIZE= 160,
    MPS_PKT_64_SIZE= 304,
    MPS_PKT_LEGACY_SIZE= 348,
    MPS_PKT_16_LABVIEW_SIZE = 72,
    MPS_PKT_32_LABVIEW_SIZE = 136,
    MPS_PKT_64_LABVIEW_SIZE = 264,
} MpsBinaryPacketSize;

static inline uint16_t mps_packet_size_from_type(MpsBinaryPacketType t)
{
    switch (t)
    {
        case MPS_PKT_16_TYPE: case MPS_PKT_16_RAW_TYPE: return MPS_PKT_16_SIZE;
        case MPS_PKT_32_TYPE: case MPS_PKT_32_RAW_TYPE: return MPS_PKT_32_SIZE;
        case MPS_PKT_64_TYPE: case MPS_PKT_64_RAW_TYPE: return MPS_PKT_64_SIZE;
        case MPS_PKT_LEGACY_TYPE: return MPS_PKT_LEGACY_SIZE;
        case MPS_PKT_UNDEFINED: defualt: return 0;
    }
}

static inline uint16_t mps_labview_packet_size(uint32_t ch)
{
    switch (ch)
    {
        case 16: return MPS_PKT_16_LABVIEW_SIZE;
        case 32: return MPS_PKT_32_LABVIEW_SIZE;
        case 64: return MPS_PKT_64_LABVIEW_SIZE;
        default: return 0;
    }
}

typedef struct Mps16RawPacket
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[4];
    int32_t counts[16];
} Mps16RawPacket;


typedef struct Mps32RawPacket
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[4];
    int32_t counts[32];
} Mps32RawPacket;


typedef struct Mps64RawPacket
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[8];
    int32_t counts[64];
} Mps64RawPacket;

typedef struct Mps16Packet
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[4];
    float pressure[16];
} Mps16Packet;


typedef struct Mps32Packet
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[4];
    float pressure[32];
} Mps32Packet;


typedef struct Mps64Packet
{
    int32_t type;
    uint32_t frame;
    uint32_t frame_time_s;
    uint32_t frame_time_ns;
    float temperature[8];
    float pressure[64];
} Mps64Packet;

typedef struct MpsLegacyPacket
{
    int32_t type;
    int32_t size;          /* 348 bytes for this packet type */
    int32_t frame;         /* frame number */
    int32_t serial_number; /* module serial number (SN) */

    float framerate;       /* scanning rate in Hz */
    int32_t valve_status;  /* 0 = Px, 1 = Cal */
    int32_t unit_index;    /* MpsUnits index */
    float unit_conversion; /* conversion factor from PSI to selected units */

    uint32_t ptp_scan_start_time_s;  /* Scan start time (SST) in seconds (epoch) */
    uint32_t ptp_scan_start_time_ns; /* Scan start time (SST) in nanoseconds */

    uint32_t external_trigger_time_us; /* External trigger time in microseconds */

    float temperature[8]; /* array of 4 RTD temps, remaining padded with 0 */

    /*
     * Pressures: array of 64 values.
     * The manual allows float or integer depending on units (EU vs RAW).
     * This struct models the most common float/EU case. For RAW, interpret
     * the same bytes as int32_t.
     */
    union {
        float pressure[64];
        int32_t counts[64];
    };

    uint32_t frame_time_s;  /* frame time in seconds, relative to SST */
    uint32_t frame_time_ns; /* frame time in nanoseconds, relative to SST */

    uint32_t external_trigger_time_s;  /* external trigger time in seconds (rel. to SST) */
    uint32_t external_trigger_time_ns; /* external trigger time in nanoseconds (rel. to SST) */
} MpsLegacyPacket;


typedef struct Mps16LabviewPacket
{
    float frame;        /* frame number */
    float temperature;  /* average MPS temperature */
    float pressure[16]; /* 16 channels of pressure data */
} Mps16LabviewPacket;

typedef struct Mps32LabviewPacket
{
    float frame;        /* frame number */
    float temperature;  /* average MPS temperature */
    float pressure[32]; /* 32 channels of pressure data */
} Mps32LabviewPacket;

typedef struct Mps64LabviewPacket
{
    float frame;        /* frame number */
    float temperature;  /* average MPS temperature */
    float pressure[64]; /* 64 channels of pressure data */
} Mps64LabviewPacket;

#ifdef __cplusplus
static_assert(sizeof(Mps16Packet)        == MPS_PKT_16_SIZE,         "Mps16Packet must be 96 bytes");
static_assert(sizeof(Mps32Packet)        == MPS_PKT_32_SIZE,         "Mps32Packet must be 160 bytes");
static_assert(sizeof(Mps64Packet)        == MPS_PKT_64_SIZE,         "Mps64Packet must be 340 bytes");
static_assert(sizeof(Mps16RawPacket)     == MPS_PKT_16_SIZE,         "Mps16RawPacket must be 96 bytes");
static_assert(sizeof(Mps32RawPacket)     == MPS_PKT_32_SIZE,         "Mps32RawPacket must be 160 bytes");
static_assert(sizeof(Mps64RawPacket)     == MPS_PKT_64_SIZE,         "Mps64RawPacket must be 340 bytes");
static_assert(sizeof(MpsLegacyPacket)    == MPS_PKT_LEGACY_SIZE,     "MpsBinaryPacket must be 348 bytes");
static_assert(sizeof(Mps16LabviewPacket) == MPS_PKT_16_LABVIEW_SIZE, "MpsLabviewFrame must be 72 bytes");
static_assert(sizeof(Mps32LabviewPacket) == MPS_PKT_32_LABVIEW_SIZE, "MpsLabviewFrame must be 136 bytes");
static_assert(sizeof(Mps64LabviewPacket) == MPS_PKT_64_LABVIEW_SIZE, "MpsLabviewFrame must be 264 bytes");
#endif


typedef struct MpsBinaryPacketInfo
{
    uint8_t type;
    uint8_t pressure_channel_count;
    uint8_t temperature_channel_count;
    uint16_t size_bytes;
} MpsBinaryPacketInfo;

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

static inline const MpsBinaryPacketInfo* mps_get_binary_packet_info_by_type(uint8_t type)
{
    for (size_t i = 0; i < MPS_BINARY_PACKET_INFO_COUNT; ++i)
    {
        if (kMpsBinaryPacketInfoTable[i].type == type) return &kMpsBinaryPacketInfoTable[i];
    }
    return (const MpsBinaryPacketInfo*)0;
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* MPS_PROTOCOL_H_ */
