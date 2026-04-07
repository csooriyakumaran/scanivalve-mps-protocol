# SCANIVALVE MPS PROTOCOL

Core data structures, enumerations, and helper routines for working with [Scanivalve MPS-42xx](https://scanivalve.com/products/pressure-measurement/miniature-ethernet-pressure-scanners/mps4200/) pressure scanners. This single-header c-library is intended to be shared by both client and [emulator](https://github.com/csooriyakumaran/mps-emulator.git) applications (e.g., for simulating a virtual scanner to aid in development of the client applications), that need to construct, send, receive, and interpret MPS data packets. 

All multi-byte fields are defined according to the [Scanivavle Hardware, Software, and User Manual](https://scanivalve.com/wp-content/uploads/2026/03/MPS4200_v401_260304.pdf)

Consumers of this library are responsible for handling the byte-ordering of packets that are streamed from devices, or read from or written to files.

## Including the Interface in a Project

The interface is a single-header file which can simply be copied to a directory in your project's include path.

The header tracks the on-device firmware version via `MPS_FIRMWARE_VERSION_MAJOR` / `MPS_FIRMWARE_VERSION_MINOR` / `MPS_FIRMWARE_VERSION_STRING` in `scanivalve/mps-protocol.h` (for example, firmware v4.01). Separately, the header-only library itself is versioned using semantic versioning. When using CMake, this appears as:

- `MPS_PROTOCOL_VERSION_MAJOR`, `MPS_PROTOCOL_VERSION_MINOR`, `MPS_PROTOCOL_VERSION_PATCH`
- `MPS_PROTOCOL_VERSION_STRING` (e.g. "0.1.0") via the generated `scanivalve/mps-protocol-version.h` header. 

Consumers that do not use CMake can rely on git tags for the library version and on the firmware version macros in `mps-protocol.h`.

### As a git submodule

To keep up to date with development on the MPS protocol, include the directory as a git submodule. 

```bash
# add as git submodule
git submodule add https://github.com/csooriyakumaran/scanivalve-mps-protocol external/scanivalve-mps-protocol
git submodule update --init --recursive

# [Optiona] checkout specific tag
cd external/scanivalve-mps-protocol
git fetch --tags

git checkout <tag_name>
#e.g. for firmware version v4.01 check version compatibility table below
git checkout v0.1.0

# stage and commit the changes in the parent repository
cd <project-root>
git add external/scanivalve-mps-protocol
git commit -m "Add submodule at specific tag <tag_name>"

```

### CMake

```cmake 
include(FetchContent)

FetchContent_Declare(
    scanivalve-mps-protocol
    GIT_REPOSITORY https://github.com/csooriyakumaran/scanivalve-mps-protocol.git
    GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(scanivalve-mps-protocol)

# ... #

target_link_libraries(your_app PRIVATE scanivalve-mps-protocol)
```

## PARSING DATA

### from a binary file

```c++
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "scanivalve/mps-protocol.h"

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "Usage: %s frames.bin\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];
    FILE* fp = std::fopen(filename, "rb");
    if (!fp)
    {
        std::perror("fopen");
        return 1;
    }

    std::uint8_t buf[MPS_MAX_BINARY_PACKET_SIZE];

    for (;;)
    {
        // Read the first 4 bytes for the type field (already little-endian)
        std::uint32_t type_le;
        std::size_t n = std::fread(&type_le, 1, sizeof(type_le), fp);
        if (n == 0)
            break; // EOF
        if (n != sizeof(type_le))
        {
            std::fprintf(stderr, "Short read for type field\n");
            break;
        }

        std::uint8_t type_id = static_cast<std::uint8_t>(type_le & 0xFFu);

        const MpsBinaryPacketInfo* info = mps_get_binary_packet_info_by_type(type_id);
        if (!info)
        {
            std::fprintf(stderr, "Unknown packet type 0x%02X\n", type_id);
            break;
        }

        if (info->size_bytes > MPS_MAX_BINARY_PACKET_SIZE)
        {
            std::fprintf(stderr, "Packet size %u exceeds buffer\n", info->size_bytes);
            break;
        }

        // Copy type field into the buffer, then read the remainder
        std::memcpy(buf, &type_le, sizeof(type_le));

        std::size_t remaining = info->size_bytes - sizeof(type_le);
        if (remaining > 0)
        {
            n = std::fread(buf + sizeof(type_le), 1, remaining, fp);
            if (n != remaining)
            {
                std::fprintf(stderr, "Short read for packet body\n");
                break;
            }
        }

        // Now interpret buf[0..size_bytes-1] as the appropriate packet struct
        switch (info->type)
        {
        case MPS_PKT_LEGACY_TYPE:
        {
            const MpsLegacyPacket* pkt = reinterpret_cast<const MpsLegacyPacket*>(buf);

            std::uint32_t frame        = pkt->frame;          // already little-endian
            std::uint32_t frame_time_s = pkt->frame_time_s;
            std::uint32_t unit_index   = pkt->unit_index;

            std::printf("Legacy frame %u, time %u, units index %u\n",
                        frame, frame_time_s, unit_index);

            if (unit_index == MPS_UNITS_RAW)
            {
                // RAW: 64 int32 counts in pkt->counts[]
                for (std::size_t i = 0; i < 4; ++i)
                    std::printf("  P[%zu] = %d (counts)\n", i, pkt->counts[i]);
            }
            else
            {
                // EU: 64 floats in pkt->pressure[]
                for (std::size_t i = 0; i < 4; ++i)
                    std::printf("  P[%zu] = %f\n", i, pkt->pressure[i]);
            }

            for (std::size_t i = 0; i < 2; ++i)
                std::printf("  T[%zu] = %f\n", i, pkt->temperature[i]);
            break;
        }

        case MPS_PKT_16_TYPE:
        {
            const Mps16Packet* pkt = reinterpret_cast<const Mps16Packet*>(buf);
            std::printf("16 EU frame %u\n", pkt->frame);

            for (std::size_t i = 0; i < 2; ++i)
                std::printf("  T[%zu] = %f\n", i, pkt->temperature[i]);
            for (std::size_t i = 0; i < 4; ++i)
                std::printf("  P[%zu] = %f\n", i, pkt->pressure[i]);
            break;
        }

        case MPS_PKT_16_RAW_TYPE:
        {
            const Mps16RawPacket* pkt = reinterpret_cast<const Mps16RawPacket*>(buf);
            std::printf("16 RAW frame %u\n", pkt->frame);

            for (std::size_t i = 0; i < 2; ++i)
                std::printf("  T[%zu] = %f\n", i, pkt->temperature[i]);
            for (std::size_t i = 0; i < 4; ++i)
                std::printf("  P[%zu] = %d (counts)\n", i, pkt->counts[i]);
            break;
        }

        // Add analogous cases for 32/64 EU and RAW types as needed.

        default:
            std::printf("Unhandled type 0x%02X (size %u)\n",
                        info->type, info->size_bytes);
            break;
        }

        std::printf("----\n");
    }

    std::fclose(fp);
    return 0;
}

```

## DATA FORMATS

### TCP BINARY SERVER DATA FORMATTING MATRIX
| DEVICE ID | DATA DESTINATION | UNITS | SET FORMAT | SIM        | PACKET TYPE | PACKET SIZE |
|-----------|------------------|-------|------------|------------|-------------|-------------|
| 4216      | TCP port 503     | EU    | B B        | 0          | 0x5D        | 96          |
| 4216      | TCP port 503     | RAW   | B B        | 0          | 0x5B        | 96          |
| 4216      | TCP port 503     | EU    | B L        | 0          | LABVIEW16   | 72          |
| 4216      | TCP port 503     | RAW   | B L        | 0          | LABVIEW16   | 72          |
| 4232      | TCP port 503     | EU    | B B        | 0          | 0x65        | 160         |
| 4232      | TCP port 503     | RAW   | B B        | 0          | 0x63        | 160         |
| 4232      | TCP port 503     | EU    | B L        | 0          | LABVIEW32   | 136         |
| 4232      | TCP port 503     | RAW   | B L        | 0          | LABVIEW32   | 136         |
| 4264      | TCP port 503     | EU    | B B        | 0          | 0x6D        | 304         |
| 4264      | TCP port 503     | RAW   | B B        | 0          | 0x69        | 304         |
| 4264      | TCP port 503     | EU    | B L        | 0          | LABVIEW64   | 264         |
| 4264      | TCP port 503     | RAW   | B L        | 0          | LABVIEW64   | 264         |
| - ANY -   | TCP port 503     | EU    | B B        | 0x40 (64)  | 0x0A        | 348(*)      |
| - ANY -   | TCP port 503     | RAW   | B B        | 0x40 (64)  | 0x0A        | 348(*)      |
| - ANY -   | TCP port 503     | EU    | B L        | 0x40 (64)  | LABVIEW64   | 264(*)      |
| - ANY -   | TCP port 503     | RAW   | B L        | 0x40 (64)  | LABVIEW64   | 264(*)      |

(*) ZERO PADDED FOR MPS-4216 & MPS-4232

### UDP BINARY DATA FORMATTING MATRIX
| DEVICE ID | DATA DESTINATION | ENUDP | ENFTP | UNITS | SET FORMAT | SIM        | PACKET TYPE | PACKET SIZE |
|-----------|------------------|-------|-------|-------|------------|------------|-------------|-------------|
| 4216      | UDP              | 1     | 0     |  EU   | F B        |  0         | 0x5D        | 96          |
| 4216      | UDP              | 1     | 0     |  RAW  | F B        |  0         | 0x5B        | 96          |
| 4232      | UDP              | 1     | 0     |  EU   | F B        |  0         | 0x65        | 160         |
| 4232      | UDP              | 1     | 0     |  RAW  | F B        |  0         | 0x63        | 160         |
| 4264      | UDP              | 1     | 0     |  EU   | F B        |  0         | 0x6D        | 304         |
| 4264      | UDP              | 1     | 0     |  RAW  | F B        |  0         | 0x69        | 304         |
| - ANY -   | UDP              | 1     | 0     |  EU   | F B        |  0x40 (64) | 0x0A        | 348(*)      |
| - ANY -   | UDP              | 1     | 0     |  RAW  | F B        |  0x40 (64) | 0x0A        | 348(*)      |

(*) ZERO PADDED FOR MPS-4216 & MPS-4232

### FTP BINARY DATA FORMATTING MATRIX
| DEVICE ID | DATA DESTINATION | ENUDP | ENFTP | UNITS | SET FORMAT | SIM        | PACKET TYPE | PACKET SIZE |
|-----------|------------------|-------|-------|-------|------------|------------|-------------|-------------|
| 4216      | FTP              | 0     | 1     |  EU   | F B        |  0         | 0x5D        | 96          |
| 4216      | FTP              | 0     | 1     |  RAW  | F B        |  0         | 0x5B        | 96          |
| 4232      | FTP              | 0     | 1     |  EU   | F B        |  0         | 0x65        | 160         |
| 4232      | FTP              | 0     | 1     |  RAW  | F B        |  0         | 0x63        | 160         |
| 4264      | FTP              | 0     | 1     |  EU   | F B        |  0         | 0x6D        | 304         |
| 4264      | FTP              | 0     | 1     |  RAW  | F B        |  0         | 0x69        | 304         |
| - ANY -   | FTP              | 0     | 1     |  EU   | F B        |  0x40 (64) | 0x0A        | 348(*)      |
| - ANY -   | FTP              | 0     | 1     |  RAW  | F B        |  0x40 (64) | 0x0A        | 348(*)      |

(*) ZERO PADDED FOR MPS-4216 & MPS-4232

## Version Compatibility

| LIB VERSION | FIRMWARE VERSION | NOTABLE CHANGES                             |
| ----------- | ---------------- | ---------------                             |
| v0.1.0      | v4.01            | ---                                         |
| v0.1.1      | v4.01            | firmware version macro name                 |


## Changelog

### LIBARARY

#### v0.1.0
- Initial Release

#### v0.1.1
- updated macro `MPS_VERSION_STRING` in `scanivalve/mps-protocol` to `MPS_FIRMWARE_VERSION_STRING`

### FIRMWARE

#### v4.01
- `SET SVRSEL` removed: data destination (TCP Binary, UDP, FTP), and FORMAT determined through other means
- `SET OPTIONS` deprecated: not used by MPS4200, kept in v4.01 for backwards compatibility with MP4264Gen1
- `FORMAT S` (Statistical Binary Format) removed

