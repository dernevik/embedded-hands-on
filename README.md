# Embedded Hands-on

A long-term, hands-on learning programme for building **practical low-level embedded intuition** relevant to automotive ECUs and AUTOSAR platform software.

The goal is not STM32 specialization or production-level MCU-driver expertise. It is to **explain, observe and debug** mechanisms beneath higher-level software: CPU execution, memory, interrupts, communication, faults, RTOS, multicore coordination and system lifecycle.

There are **15 agreed core labs** in two phases, plus an optional playground. There is no deadline or required pace.

## Current state

**As of 2026-10-29**

- **Completed:** **Labs 1–4**, approved by the course coordinator after review of instructor handoffs. Lab 4: **A/B achieved; C achieved in a guided debugging investigation**, without proof of the exact electrical mechanism of the observed bit-shift fault.
- **Next core topic:** **Lab 5 — SPI Interrupts & DMA**, reusing the working two-board SPI1 setup and comparing asynchronous completion/CPU involvement with the Lab 4 polling baseline.
- **Lab 4 verified reference:** [master/slave firmware and guide](lab04-spi-polling/README.md), [personal notes](lab04-spi-polling/notes.md), and two captured PulseView exchanges in [`lab04-spi-polling/captures/`](lab04-spi-polling/captures/) (including `.sr` data and `.pvs` session settings). The final one-shot protocol uses two CS-low transfers: request `01 37` / initial reply `A5 5A`, then dummy `00 00` / computed response `81 37`.
- **Tools and hardware:** both NUCLEO-F446RE boards and the SparkFun 8-channel analyzer have now been used in real SPI experiments. Verified pin mapping, board identity workflow and practical wiring/measurement guidance are documented. No additional purchase or repeat lab is required before Lab 5.

**NEXT:** When ready, request the **Lab 5 — SPI Interrupts & DMA start prompt** from the course coordinator. Keep the completed Lab 4 polling firmware and PulseView captures as a last-known-good baseline; do not overwrite them while introducing asynchronous transfers.

## Progress

| Lab | Topic | Status |
|---|---|---|
| [01](lab01-debugger-cpu/README.md) | Debugger, CPU and assembly | **Completed — A/B/C** |
| [02](lab02-memory-linker/README.md) | Flash/RAM, linker, startup, stack and heap | **Completed — A/B; C achieved with a tutor hint** |
| [03](lab03-interrupts/README.md) | GPIO polling → interrupts | **Completed — A/B; guided, requirement-specific C** |
| [04](lab04-spi-polling/README.md) | SPI polling, two-board full-duplex and PulseView | **Completed — A/B; guided C with unproven exact electrical cause** |
| 05 | SPI interrupts and DMA | **Next — start prompt pending** |
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

**Hardware wiring guidance:** Before powering a new board-to-board or instrument connection, it is prudent to check the official pinout and connector orientation, validate GND with an unpowered continuity test, and avoid driving unpowered boards. The learner owns the hardware decisions; these are recommendations, not additional lab requirements. See [Hardware wiring and measurement safety](docs/hardware-safety.md), including the Lab 4 CN10/AVDD and back-powering lessons.

Verified through Lab 4: **two NUCLEO-F446RE boards / STM32F446RE (Arm Cortex-M4)**, onboard ST-LINK over SWD, **Windows 11**, **STM32CubeIDE 2.2.0**, standalone **STM32CubeMX 6.18.1**, STM32CubeF4 1.28.3, and **SparkFun TOL-18627 logic analyzer with PulseView**. Two-board SPI and digital captures are demonstrated, not merely planned. The Raspberry Pi/Linux workstation and new shields remain optional future ideas.

## Repository guide

- [`docs/roadmap.md`](docs/roadmap.md) — agreed core curriculum, rationale and progress.
- [`docs/future-ideas.md`](docs/future-ideas.md) — preserved deferred interests, including **Zig on STM32** and **six optional playground labs** (CAN, TFT, motors, IMU, Ethernet-over-SPI and servos).
- [Lab 1 README](lab01-debugger-cpu/README.md) / [notes](lab01-debugger-cpu/notes.md) — reproducible instructions / personal observations.
- [Lab 2 README](lab02-memory-linker/README.md) / [notes](lab02-memory-linker/notes.md) — reproducible instructions / personal observations.
- [Lab 3 README](lab03-interrupts/README.md) / [notes](lab03-interrupts/notes.md) — external interrupts, exception entry and deferred event work.
- [Lab 4 README](lab04-spi-polling/README.md) / [notes](lab04-spi-polling/notes.md) / [captures](lab04-spi-polling/captures/) — reproducible two-board SPI transfers, insights and actual PulseView evidence.
- [Hardware wiring and measurement guidance](docs/hardware-safety.md) and [board identities](docs/board-identities.md) — practical notes for using two identical boards and connecting instruments.
- `labNN/firmware/` — actual firmware sources/configuration when added for that lab.
- [`docs/inventory.md`](docs/inventory.md) / [`docs/shopping-list.md`](docs/shopping-list.md) — equipment records; verify their freshness before using them as purchase status.

**Optional playground** is deliberately separate from Labs 1–15. It can be explored for fun, but never becomes a prerequisite for the next core lab.

## Restart in five minutes (when possible)

1. Read **Current state** and **NEXT** above.
2. Open the current instructor chat and the latest lab checkpoint (if any).
3. Reconnect the known-good board/setup; build, flash and verify a breakpoint or other baseline.
4. Do the next **single** experiment. Update `NEXT` only when needed.

Keep public repository content free of proprietary employer details, credentials and private personal material.
