# SCANIVALVE MPS PROTOCOL

Core data structures, enumerations, and helper routines for working with [Scanivalve MPS-42xx](https://scanivalve.com/products/pressure-measurement/miniature-ethernet-pressure-scanners/mps4200/) pressure scanners. This single-header c-library is intended to be shared by both client and [emulator](https://github.com/csooriyakumaran/mps-emulator.git) applications (e.g., for simulating a virtual scanner to aid in development of the client applications), that need to construct, send, receive, and interpret MPS data packets. 

All multi-byte fields are defined according to the [Scanivalve Hardware, Software, and User Manual](https://scanivalve.com/wp-content/uploads/2026/03/MPS4200_v401_260304.pdf)

Multi-byte fields are transmitted in the device's native (little-endian) order. The library detects the byte order from each packet's leading type field and provides helpers (`mps_peek`, `mps_copy_packet`, `mps_byte_swap_inplace`) to convert to host order, so consumers do not have to hand-roll byte swapping. See [PARSING DATA](#parsing-data).

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
git checkout v0.1.4

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
    GIT_TAG v0.1.4
)
FetchContent_MakeAvailable(scanivalve-mps-protocol)

# ... #

target_link_libraries(your_app PRIVATE scanivalve-mps-protocol)
```

## PARSING DATA

The packet type and the wire byte order are constant for a given stream or file,
so call `mps_peek` **once** on the first frame to identify the packet (`info`),
its byte order, and whether a swap is needed versus the host. Cache that in an
`MpsStream` and reuse it for every packet.

Convert each frame to host order with either:

- `mps_copy_packet(dst, src, size_bytes, byte_swap)` — copies `src` into a
  separate, correctly aligned struct (leaving `src` untouched). Use this for
  read-only sources such as memory-mapped files or shared memory; with
  `byte_swap == 0` it is a plain `memcpy`. This is also the cleanest option
  generally, since the field reads afterward are well-defined.
- `mps_byte_swap_inplace(buf, size_bytes)` — swaps in place for buffers you own
  and may mutate (e.g. a socket receive buffer). Call it exactly once per frame.

### from a binary file

```c++
#include <cstdint>
#include <cstdio>

#include "scanivalve/mps-protocol.h"

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "Usage: %s frames.bin\n", argv[0]);
        return 1;
    }

    FILE* fp = std::fopen(argv[1], "rb");
    if (!fp) { std::perror("fopen"); return 1; }

    std::uint8_t buf[MPS_MAX_BINARY_PACKET_SIZE];
    MpsStream    stream{};   // populated on the first packet, reused thereafter

    for (;;)
    {
        // The leading 4 bytes are enough for mps_peek to identify the packet
        // and its byte order.
        std::size_t n = std::fread(buf, 1, 4, fp);
        if (n == 0) break;                                  // EOF
        if (n != 4) { std::fprintf(stderr, "short read (type)\n"); break; }

        // Peek once; type and byte order are constant for the rest of the file.
        if (stream.info == nullptr &&
            mps_peek(buf, 4, &stream.info, &stream.order,
                     &stream.byte_swap_needed) != MPS_PARSE_OK)
        {
            std::fprintf(stderr, "unrecognized packet / byte order\n");
            break;
        }

        // Read the remainder of the frame.
        std::size_t remaining = stream.info->size_bytes - 4;
        if (std::fread(buf + 4, 1, remaining, fp) != remaining)
        {
            std::fprintf(stderr, "short read (body)\n");
            break;
        }

        // Copy into an aligned, host-order struct (no-op swap on an LE host).
        switch (stream.info->type)
        {
        case MPS_PKT_LEGACY_TYPE:
        {
            MpsLegacyPacket pkt;
            mps_copy_packet(reinterpret_cast<std::uint8_t*>(&pkt), buf,
                            stream.info->size_bytes, stream.byte_swap_needed);

            std::printf("legacy frame %u  units=%d\n", pkt.frame, pkt.unit_index);
            if (pkt.unit_index == MPS_UNITS_RAW)
                std::printf("  P[0] = %d (counts)\n", pkt.counts[0]);
            else
                std::printf("  P[0] = %f\n", pkt.pressure[0]);
            std::printf("  T[0] = %f\n", pkt.temperature[0]);
            break;
        }

        case MPS_PKT_64_TYPE:
        {
            Mps64Packet pkt;
            mps_copy_packet(reinterpret_cast<std::uint8_t*>(&pkt), buf,
                            stream.info->size_bytes, stream.byte_swap_needed);
            std::printf("64 EU frame %u  P[0]=%f  T[0]=%f\n",
                        pkt.frame, pkt.pressure[0], pkt.temperature[0]);
            break;
        }

        // ... 16/32 EU and RAW cases follow the same pattern ...

        default:
            std::printf("unhandled type 0x%02X (size %u)\n",
                        stream.info->type, stream.info->size_bytes);
            break;
        }
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

| LIB VERSION (git tag) | FIRMWARE VERSION |
| --------------------- | ---------------- |
| v0.1.0 - v0.1.x       | v4.01            |


## Changelog

### LIBRARY

See [CHANGELOG.md](CHANGELOG.md)

### FIRMWARE

#### v4.01
- `SET SVRSEL` removed: data destination (TCP Binary, UDP, FTP), and FORMAT determined through other means
- `SET OPTIONS` deprecated: not used by MPS4200, kept in v4.01 for backwards compatibility with MP4264Gen1
- `FORMAT S` (Statistical Binary Format) removed

