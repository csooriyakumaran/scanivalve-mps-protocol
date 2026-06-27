# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Note: the library version is tracked separately from the MPS firmware version
that the protocol targets (currently firmware v4.01).

## [Unreleased]

### Fixed
- Corrected ordering and transcription bugs in the `MpsUnits` enum and the
  `kMpsUnitConversion` / `kMpsUnitStrings` tables. The enum order now matches
  the manual's binary "Units index" (Appendix A), so a `unit_index` read from a
  packet indexes the conversion and string tables correctly.
- Fixed the `MMHG` conversion factor (was 10x too large).

### Changed
- Unit conversion factors now match Appendix A of the manual verbatim,
  including the manual's own rounding quirks, to preserve fidelity with the
  device's internal conversions. Added a provenance comment documenting this
  policy.

## [0.1.4] - 2026-04-08

### Changed
- `MpsSimFlags` reworked into `enum MpsSimFlags_` with a `typedef int MpsSimFlags`
  alias, so flag values can be OR-combined without enum-type warnings.
- All indexing casts (e.g. from enum values) changed to `size_t`.
- Cleaned up doc strings for better Markdown rendering.

## [0.1.3] - 2026-04-07

### Fixed
- Added a `default` case to `mps_packet_size_from_type`.
- Typo fix.

## [0.1.2] - 2026-04-07

### Added
- `MPS_PKT_UNDEFINED = 0` sentinel for the binary packet type enum.
- `mps_packet_size_from_type` to look up a packet size from its type.
- `mps_labview_packet_size` to look up a LabVIEW packet size from the pressure
  channel count.

### Changed
- README updates.

## [0.1.1] - 2026-04-07

### Changed
- Renamed the firmware version string macro.

## [0.1.0] - 2026-04-07

Initial release.

### Added
- `scanivalve/mps-protocol.h`: core data structures, enumerations, and helper
  routines for the Scanivalve MPS-42xx pressure scanner protocol (firmware
  v4.01), shared by the emulator and client applications.
- Valve state enums and helpers.
- Library-level semantic versioning (separate from firmware version), with a
  generated `mps-protocol-version.h` produced by CMake.
- CMake `INTERFACE` library target and `FetchContent` usage instructions in the
  README.
- MIT license.

### Changed
- Removed struct field initialization to maintain C compatibility.

[Unreleased]: https://github.com/csooriyakumaran/scanivalve-mps-protocol/compare/v0.1.4...HEAD
[0.1.4]: https://github.com/csooriyakumaran/scanivalve-mps-protocol/compare/v0.1.3...v0.1.4
[0.1.3]: https://github.com/csooriyakumaran/scanivalve-mps-protocol/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/csooriyakumaran/scanivalve-mps-protocol/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/csooriyakumaran/scanivalve-mps-protocol/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/csooriyakumaran/scanivalve-mps-protocol/releases/tag/v0.1.0
