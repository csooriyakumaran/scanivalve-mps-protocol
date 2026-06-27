# Design note: safe packet ingest API (`mps_parse_*`)

Status: **proposed, not implemented.** Target: implement before cutting **v0.1.5**.

## Motivation

The header currently *defines* the wire layout but provides no safe way to get
bytes into the structs. Every consumer is left to do the risky part, and the
README example does it the unsafe way:

```cpp
const MpsLegacyPacket* pkt = reinterpret_cast<const MpsLegacyPacket*>(buf);
```

Three hazards stack in that one line:

1. **Strict aliasing** — `buf` is a `uint8_t[]`; reading it through an
   `MpsLegacyPacket` lvalue is UB in C++ (dicey in C). Fix: decode into a real
   struct object.
2. **Alignment** — `uint8_t buf[...]` has no alignment guarantee, but the struct
   has `int32_t`/`float` members. Fine on x86, faults / slow on ARM.
3. **Endianness — actual contradiction in the docs.** The header says fields
   "may be transmitted in big-endian (network) byte order" and the caller is
   responsible for conversion. The README example reads `pkt->frame` directly,
   commenting "already little-endian." Both can't be the contract.

Owning (de)serialization in the library fixes all three at once and pins the
endianness question down in one audited place.

**Decision to record:** the wire is **little-endian** (matches the README and
how these scanners actually stream). Update the header doc comment accordingly
and drop the "big-endian/network order" caveat.

## Design decisions

- Out-parameters + a status return. No allocation, explicit ownership.
- Read fields from byte offsets via helpers, so decode is both
  alignment-safe and host-endianness-independent.
- EU vs RAW for the 16/32/64 packets is known from the type byte, so they get
  distinct entry points. For the legacy frame it is carried in `unit_index`.

## 1. Byte readers (the crux)

Read a little-endian field from any offset, produce a host-correct value
regardless of host endianness or buffer alignment.

```c
static inline uint32_t mps_rd_u32_le(const uint8_t* p)
{
    return  (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static inline int32_t mps_rd_i32_le(const uint8_t* p)
{
    return (int32_t)mps_rd_u32_le(p);
}

static inline float mps_rd_f32_le(const uint8_t* p)
{
    uint32_t u = mps_rd_u32_le(p);
    float f;
    memcpy(&f, &u, sizeof f);   /* the only standard-blessed type pun */
    return f;
}
```

## 2. Status enum + signatures

```c
typedef enum MpsParseStatus
{
    MPS_PARSE_OK = 0,
    MPS_PARSE_ERR_NULL,         /* buf or out was NULL */
    MPS_PARSE_ERR_SHORT,        /* len smaller than the packet needs */
    MPS_PARSE_ERR_UNKNOWN_TYPE  /* type field not a recognized packet */
} MpsParseStatus;

/* Peek the 4-byte type field without committing to a full parse. */
static inline MpsParseStatus mps_peek_type(const uint8_t* buf, size_t len,
                                           MpsBinaryPacketType* out_type);

static inline MpsParseStatus mps_parse_legacy(const uint8_t* buf, size_t len, MpsLegacyPacket* out);
static inline MpsParseStatus mps_parse_16    (const uint8_t* buf, size_t len, Mps16Packet*    out);
static inline MpsParseStatus mps_parse_16_raw(const uint8_t* buf, size_t len, Mps16RawPacket* out);
static inline MpsParseStatus mps_parse_32    (const uint8_t* buf, size_t len, Mps32Packet*    out);
static inline MpsParseStatus mps_parse_32_raw(const uint8_t* buf, size_t len, Mps32RawPacket* out);
static inline MpsParseStatus mps_parse_64    (const uint8_t* buf, size_t len, Mps64Packet*    out);
static inline MpsParseStatus mps_parse_64_raw(const uint8_t* buf, size_t len, Mps64RawPacket* out);
```

Chose a status enum over the existing `int` 1/0 convention because parsing has
distinct failure modes worth distinguishing. Collapse to `int` if consistency
with the rest of the header matters more.

## 3. Reference implementation — legacy packet

Legacy is the richest case: it has the union and decides EU-vs-RAW from
`unit_index`. The running offset `o` avoids hand-maintained magic offsets and
makes the layout self-checking against the size (ends at 348).

```c
static inline MpsParseStatus mps_parse_legacy(const uint8_t* buf, size_t len,
                                              MpsLegacyPacket* out)
{
    if (!buf || !out)               return MPS_PARSE_ERR_NULL;
    if (len < MPS_PKT_LEGACY_SIZE)  return MPS_PARSE_ERR_SHORT;

    size_t o = 0;

    out->type = mps_rd_i32_le(buf + o); o += 4;
    if (out->type != MPS_PKT_LEGACY_TYPE) return MPS_PARSE_ERR_UNKNOWN_TYPE;

    out->size            = mps_rd_i32_le(buf + o); o += 4;
    out->frame           = mps_rd_i32_le(buf + o); o += 4;
    out->serial_number   = mps_rd_i32_le(buf + o); o += 4;
    out->framerate       = mps_rd_f32_le(buf + o); o += 4;
    out->valve_status    = mps_rd_i32_le(buf + o); o += 4;
    out->unit_index      = mps_rd_i32_le(buf + o); o += 4;
    out->unit_conversion = mps_rd_f32_le(buf + o); o += 4;

    out->ptp_scan_start_time_s    = mps_rd_u32_le(buf + o); o += 4;
    out->ptp_scan_start_time_ns   = mps_rd_u32_le(buf + o); o += 4;
    out->external_trigger_time_us = mps_rd_u32_le(buf + o); o += 4;

    for (size_t i = 0; i < 8; ++i) { out->temperature[i] = mps_rd_f32_le(buf + o); o += 4; }

    /* EU vs RAW is carried in unit_index for the legacy frame. */
    if (out->unit_index == MPS_UNITS_RAW)
        for (size_t i = 0; i < 64; ++i) { out->counts[i]   = mps_rd_i32_le(buf + o); o += 4; }
    else
        for (size_t i = 0; i < 64; ++i) { out->pressure[i] = mps_rd_f32_le(buf + o); o += 4; }

    out->frame_time_s             = mps_rd_u32_le(buf + o); o += 4;
    out->frame_time_ns            = mps_rd_u32_le(buf + o); o += 4;
    out->external_trigger_time_s  = mps_rd_u32_le(buf + o); o += 4;
    out->external_trigger_time_ns = mps_rd_u32_le(buf + o); o += 4;

    /* o now equals MPS_PKT_LEGACY_SIZE; guard it in C too. */
    return MPS_PARSE_OK;
}
```

The type-discriminated parsers (`mps_parse_16`, etc.) are the same pattern minus
the union branch — EU vs RAW is decided by *which function the caller
dispatched to* off the type byte, not by an in-packet field.

## 4. Dispatcher (optional single entry point)

```c
typedef struct MpsAnyPacket
{
    MpsBinaryPacketType type;
    union {
        Mps16Packet p16;  Mps16RawPacket p16_raw;
        Mps32Packet p32;  Mps32RawPacket p32_raw;
        Mps64Packet p64;  Mps64RawPacket p64_raw;
        MpsLegacyPacket legacy;
    };
} MpsAnyPacket;

static inline MpsParseStatus mps_parse_packet(const uint8_t* buf, size_t len, MpsAnyPacket* out);
```

`mps_parse_packet` peeks the type, validates `len` against
`kMpsBinaryPacketInfoTable`, and calls the right typed parser into the matching
union member.

## Open decisions

- **memcpy fast-path vs field-by-field.** Field-by-field (above) is fully
  portable. If we commit to little-endian hosts only, a whole-struct `memcpy`
  still fixes aliasing + alignment but not endianness. Keep field-by-field; the
  packets are tiny, the cost is negligible, and it removes the host footgun.
- **Serialize symmetry for the emulator.** The emulator must *emit* these.
  Plan the mirror image now even if implemented later:
  `mps_wr_u32_le(uint8_t*, uint32_t)` etc. and
  `mps_write_legacy(uint8_t* buf, size_t cap, const MpsLegacyPacket*)`, so read
  and write share one layout definition and can't drift.

## Pre-0.1.5 checklist

- [ ] Decide status-enum vs `int` return convention.
- [ ] Add `mps_rd_*_le` helpers.
- [ ] Implement all `mps_parse_*` + `mps_peek_type`.
- [ ] (Optional) `MpsAnyPacket` + `mps_parse_packet` dispatcher.
- [ ] (Optional) mirror `mps_wr_*_le` + `mps_write_*` for the emulator.
- [ ] Fix the header doc comment: wire is little-endian; remove the
      big-endian/network-order caveat.
- [ ] Update the README parse example to use `mps_parse_*` instead of
      `reinterpret_cast`.
- [ ] Add C `_Static_assert` for packet sizes (currently C++ only) and a few
      `offsetof` checks so layout can't silently drift.
- [ ] Make `mps_stricpmp` `static inline` (unused-function warning).
- [ ] Add CHANGELOG entry under Unreleased.
```
