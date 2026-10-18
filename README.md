# Embedded Hands-on

A long-term, hands-on learning programme for building **practical low-level embedded intuition** relevant to automotive ECUs and AUTOSAR platform software.

The goal is not STM32 specialization or production-level MCU-driver expertise. It is to **explain, observe and debug** mechanisms beneath higher-level software: CPU execution, memory, interrupts, communication, faults, RTOS, multicore coordination and system lifecycle.

There are **15 agreed core labs** in two phases, plus an optional playground. There is no deadline or required pace.

## Current state

**As of 2026-10-18**

- **Completed:** Lab 1 and Lab 2, both approved by the course coordinator.
- **Current:** Lab 3 — GPIO polling → interrupts. The separate instructor chat and detailed start prompt already exist. The IDE/board connection was confirmed ready on 2026-10-16; practical interrupt results have not yet been handed over.
- **Next core topic after approval of Lab 3:** Lab 4 — SPI polling, with two NUCLEO-F446RE boards and a logic analyzer.
- **No blocker:** The proven Windows / STM32CubeIDE / ST-LINK setup is sufficient for Lab 3. No Raspberry Pi, new shield or new MCU is required.

**NEXT:** Return to the existing **Lab 3 instructor chat** and follow its first small polling experiment. After completing Lab 3, bring `COURSE HANDOFF — LAB 3` to the course coordinator for an A/B/C review. Do not start Lab 4 merely because the setup works.

## Progress

| Lab | Topic | Status |
|---|---|---|
| [01](lab01-debugger-cpu/README.md) | Debugger, CPU and assembly | **Completed — A/B/C** |
| [02](lab02-memory-linker/README.md) | Flash/RAM, linker, startup, stack and heap | **Completed — A/B; C achieved with a tutor hint** |
| 03 | GPIO polling → interrupts | **Current — instructor/start prompt ready** |
| 04 | SPI polling | Planned |
| 05 | SPI interrupts and DMA | Planned |
| 06 | Faults/traps and MPU | Planned |
| 07 | RTOS and context switching | Planned |
| 08 | Watchdog, reset and crash analysis | Planned |
| 09–15 | Multicore boot, IPC, coherency, lifecycle, protection, fault isolation and mini-ECU | Planned — Phase 2 |

See [the full roadmap](docs/roadmap.md) for the **approved order and goals of all 15 labs**. The former Lab 9–14 version of Phase 2 is superseded.

## Learning method and division of responsibilities

Work in short cycles: **prediction → small experiment → observation → hypothesis/evidence → revised explanation**. Direct debugger observations are more important than merely completing instructions. Assembly is a microscope, not an end in itself.

Each lab is assessed at three levels:

- **A — Explain:** explain the mechanism in your own words.
- **B — Demonstrate:** show it on real hardware and tools; the central learning level.
- **C — Debug:** reproduce and diagnose a small deliberate defect, stating any tutor guidance honestly.

The **course coordinator** owns the roadmap, progression and approvals. A **separate instructor chat for each lab** conducts the experiments, works one small step at a time and produces a `COURSE HANDOFF — LAB N`. Handoffs are transferred explicitly; do not assume automatic memory between chats.

At natural stopping points, preserve a short `SESSION CHECKPOINT`, a **last known good** firmware state and a specific `NEXT:`. Keep documentation lightweight so returning after weeks or months is easy.

## Hardware and environment

Verified for Lab 1–2: **NUCLEO-F446RE / STM32F446RE (Arm Cortex-M4)**, onboard ST-LINK over SWD, **Windows 11**, **STM32CubeIDE 2.2.0**. Two Nucleo boards, an 8-channel USB logic analyzer and prototyping accessories were ordered for later experiments. The Raspberry Pi/Linux workstation remains an optional future idea, not a current prerequisite.

## Repository guide

- [`docs/roadmap.md`](docs/roadmap.md) — agreed core curriculum, rationale and progress.
- [`docs/future-ideas.md`](docs/future-ideas.md) — preserved deferred interests, including **Zig on STM32** and **six optional playground labs** (CAN, TFT, motors, IMU, Ethernet-over-SPI and servos).
- [Lab 1 README](lab01-debugger-cpu/README.md) / [notes](lab01-debugger-cpu/notes.md) — reproducible instructions / personal observations.
- [Lab 2 README](lab02-memory-linker/README.md) / [notes](lab02-memory-linker/notes.md) — reproducible instructions / personal observations.
- `labNN/firmware/` — actual firmware sources/configuration when added for that lab.
- [`docs/inventory.md`](docs/inventory.md) / [`docs/shopping-list.md`](docs/shopping-list.md) — equipment records; verify their freshness before using them as purchase status.

**Optional playground** is deliberately separate from Labs 1–15. It can be explored for fun, but never becomes a prerequisite for the next core lab.

## Restart in five minutes (when possible)

1. Read **Current state** and **NEXT** above.
2. Open the current instructor chat and the latest lab checkpoint (if any).
3. Reconnect the known-good board/setup; build, flash and verify a breakpoint or other baseline.
4. Do the next **single** experiment. Update `NEXT` only when needed.

Keep public repository content free of proprietary employer details, credentials and private personal material.
