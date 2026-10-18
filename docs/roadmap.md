# Embedded Hands-on — Roadmap

**Agreed core curriculum:** 15 labs in two phases. **Status:** 2026-10-18. Lab 1 and 2 approved; Lab 3 is next/current. Topics and ordering are agreed, but future pin mappings, hardware choices and exact implementations are not necessarily validated.

## Goal and teaching contract

Build **hands-on intuition beneath automotive/AUTOSAR platform software**, rather than becoming an STM32 specialist. Work with real MCU evidence, not only conceptual explanations.

Every lab follows **prediction → experiment → observation → evidence → explanation**, one small experiment at a time, and aims to achieve:

- **A — Explain:** articulate the mechanism in your own words.
- **B — Demonstrate:** show it with actual hardware/debugger/map/measurement (the most important level).
- **C — Debug:** diagnose a deliberately introduced small fault; explicitly state any hints or assistance.

The course coordinator owns progression and final acceptance; an individual instructor chat owns each lab's hands-on flow and returns a `COURSE HANDOFF — LAB N`. Keep `SESSION CHECKPOINT`, `NEXT:` and a known-good baseline at natural pauses. Do not turn an optional idea into a new prerequisite. Short, restartable lab sessions have priority over administration.

## Phase 1 — MCU Foundations (Labs 1–8)

**Hardware baseline:** STM32 NUCLEO-F446RE, Arm Cortex-M4; Windows 11, STM32CubeIDE 2.2.0 and onboard ST-LINK/SWD verified in Labs 1–2.

| Lab | Topic | Planned observation / learning goal | Status |
|---|---|---|---|
| 1 | Debugger, CPU & assembly | Build/flash/debug; registers vs RAM, PC/SP/LR, instructions, nested calls and a controlled logic defect. | **Completed, A/B/C achieved.** |
| 2 | Flash/RAM, stack, heap & linker | Linker sections/map, VMA/LMA, `Reset_Handler`, actual `.data` copy/`.bss` clear, stack, optimization, `malloc/free`, layout-change diagnosis. | **Completed, A/B achieved; C achieved with tutor hint.** |
| 3 | GPIO polling → interrupts | USER button polling baseline → real EXTI/NVIC; vector table, actual IRQ handler, pending/enabled/active, hardware exception frame, IPSR, LR/EXC_RETURN, deferred work, fault challenge. | **Current; prompt and instructor chat ready.** No reported interrupt results yet. |
| 4 | SPI polling | Two NUCLEO-F446RE boards as host/target; SCK/MOSI/MISO/CS/GND; observe transactions and full-duplex timing through PulseView. | Planned |
| 5 | SPI interrupts + DMA | Reuse Lab 4 hardware and protocol; progress polling → IRQ → DMA; compare CPU workload, completion and complexity. | Planned |
| 6 | Faults/traps + MPU | Provoke and inspect fault conditions via PC/SP/LR, fault status/address and the offending instruction; distinguish linker placement from hardware access protection. | Planned |
| 7 | RTOS & context switching | Two tasks, stacks, scheduler/context switch, ISR → task, mutex/races/priority inversion. | Planned |
| 8 | Watchdog, reset & crash analysis | Intentional hang, watchdog reset, reset cause, startup and recovery/diagnostic information. | Planned |

**Lab 3 readiness:** Windows/CubeIDE/ST-LINK chain is known good. An existing detailed Lab 3 instructor prompt has already been delivered and should be reused. The physical EXTI pin mapping and actual implementation must be verified during Lab 3, not assumed here. After the lab, the coordinator reviews the handoff before issuing the Lab 4 start prompt.

## Phase 2 — Multicore, IPC, isolation and ECU-like complexity (Labs 9–15)

**Agreed sequence — supersedes the older Labs 9–14 plan.** The seven-lab structure intentionally separates cache/coherency from basic IPC and separates lifecycle coordination from basic boot. A dual-core STM32H745/H747 (Cortex-M7/M4) was discussed as an *option*, not purchased or qualified for these labs.

| Lab | Topic | Planned experiment / understanding |
|---|---|---|
| **9** | Dual-core boot | Start M7/M4 under control; shared `BOOTING → READY`, boot order, ownership and readiness before OS/RTE. |
| **10** | Shared-memory IPC | Shared RAM mailbox → request/reply, sequence numbers, notification/IRQ/HSEM, timeout and peer reset. Shared memory is not itself a protocol. |
| **11** | Races, cache & coherency | Trigger stale reads/races; examine atomics, critical sections, barriers, cache clean/invalidate and non-cacheable regions. `volatile` is not multicore synchronization. |
| **12** | Startup & shutdown coordination | Explicit cross-core state machines, dependent services, acknowledgements, timeout and communication before RTE/OS or after runtime shutdown. |
| **13** | Partitioning & memory protection | Privileged/unprivileged domains and private/shared regions; observe access faults; distinguish actual MPU reach from broader hardware isolation. |
| **14** | Fault isolation & degraded operation | Hangs, illegal access, missed heartbeat, detection and degraded/recovery behavior; distinguish detection, containment and recovery. |
| **15** | Integrated mini-ECU | Integrate boot, IPC, coherence, ownership, isolation, lifecycle and faults. Follow `RESET → EARLY_INIT → CORE_SYNC → SERVICES_START → RUN → SHUTDOWN → FINALIZE`. |

**Engineering caveat:** This is a conceptual teaching plan, **not** a validated multicore board configuration, AUTOSAR implementation or ASIL-certified safety design. Before Phase 2 hands-on work, verify the chosen board's memory map, cache, HSEM, debugger and peripheral ownership. A per-core MPU does not necessarily isolate other cores or bus masters.

## Possible Phase 3 — Platform Software Architecture

**Proposed only; not an agreed third mandatory phase.** A future sequence might connect generic runtime/framework components, modeled/YAML-generated interfaces and data tables, OS task/runnable dispatch, communication and diagnostics, drivers/MCAL and physical hardware. Four useful perspectives are the **data path, control path, fault path and configuration path**. No lab numbers, scope or hardware are committed.

## Optional playground

A parallel, **non-prerequisite** playground was suggested in the original SPI discussion and welcomed for future exploration. Candidates: **CAN between two STM32 boards, SPI TFT dashboard, DC motor/stepper control, IMU sensor acquisition, Ethernet over SPI and servo control.** These are **not Labs 16–21** and do not alter the core sequence. See [future ideas and hardware cautions](future-ideas.md).

## Decisions worth preserving

- Original SPI demonstration expanded into a foundations-first course: **debugger and memory before SPI**, to reveal the mechanism beneath an API.
- **15 labs, not 8 or 14:** Phase 1 remains Labs 1–8; consolidated Phase 2 is Labs 9–15.
- Main aim is practical comprehension and evidence-based diagnostics, **not** low-level specialist speed or implementing AUTOSAR from scratch.
- Lab 1 was accepted at **A/B/C**; Lab 2 at **A/B and guided C**, with a corrected `static`-storage prediction. There is no requirement to repeat them.
- **Assembly is an observational microscope**, not a course in memorizing instruction sets.
- The repo is the durable course reference: each lab's `README.md` helps another person reproduce the lab; `notes.md` captures personal observations. A separate tutor chat supplies a `COURSE HANDOFF` that the learner brings back to the coordinator.
- **Low restart friction:** short checkpoints, an exact `NEXT:`, firmware baseline, tool versions and wiring pictures where useful; no heavyweight process or deadlines.
- Toolchain migration, Zig, playground hardware and architecture expansion are **optional/deferred**, never blockers for the current lab.

## Durable references

- [Project overview and current NEXT](../README.md)
- [Future ideas and commitments](future-ideas.md)
- [Lab 1 reproducible instructions](../lab01-debugger-cpu/README.md) and [learning notes](../lab01-debugger-cpu/notes.md)
- [Lab 2 reproducible instructions](../lab02-memory-linker/README.md) and [learning notes](../lab02-memory-linker/notes.md)

**Updating this file:** Change the agreed curriculum only when the learner and course coordinator explicitly decide. Update statuses after evidence-based lab approval; record interesting but uncommitted tangents in `future-ideas.md`. Keep the root README's `NEXT` current. 
