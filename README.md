![](.github/images/screenshot.png)

# 2dxtra
[![Build-MSVC](https://github.com/aixxe/2dxtra/actions/workflows/Build-MSVC.yml/badge.svg)](https://github.com/aixxe/2dxtra/actions/workflows/Build-MSVC.yml)

A multi-hack for arcade beatmania IIDX

- In-game interface with support for keyboard and controller input
- Automatically blocks invalid scores from being sent to network
- Cheat modifiers such as auto-play, regular speed & CN type override
- Ahead of time Urafumen & All-Scratch chart generator based on 2dxplus
- Rate mod implementation from x0.5 to x3.0 chart speed, with optional pitch shift
- Custom chart loader with support for separate score saving via C API
- Novelty modes to combine notes, re-arrange keysounds & swap scratches
- Includes an updated version of [2dxAutoRetry](https://github.com/aixxe/2dxAutoRetry) with additional options
- Ability to increase or decrease each judgement timing window

## Supported versions

2dxtra only loads when `bm2dx.dll` matches one of the supported builds. The match is exact on the PE header triple (`SizeOfCode` / `AddressOfEntryPoint` / `SizeOfImage`):

| Generation | Version | `SizeOfCode` | `AddressOfEntryPoint` | `SizeOfImage` |
|---|---|---|---|---|
| IIDX 33 | `2026081900-010` | `0x0C8F800` | `0x0B1CCCC` | `0x0BB95000` |
| IIDX 33 | `2026081900-012` | `0x0BC0A00` | `0x0A4FD5C` | `0x0BA81000` |
| IIDX 34 | `2026091600-010` | `0x00C64200` | `0x00B02B3C` | `0x0C5FF000` |

The IIDX 34 entry targets the `010-20260916` build (the game reports build id `2026091700`). Supporting another build means adding one file under `src/2dxtra/offsets/`; see `docs/2026091600-010.md` for the method.