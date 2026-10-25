# Future Ideas & Commitments

**Updated:** 2026-10-25 · **Purpose:** a durable parking place for curiosity, *not* a backlog or shopping obligation. The agreed core is [Labs 1–15](roadmap.md); ideas here cannot become prerequisites unless explicitly agreed. Status terms: **Planned** = already part of core roadmap, **Deferred** = expressed interest kept for later, **Proposed** = suggestion not adopted as a commitment. Hardware choices require compatibility checks before purchase.

## Explicit future interest: Zig on STM32

**Deferred — explicit learner request, also recorded in Lab 2 handoff.** Try a small **bare-metal Zig firmware experiment on the existing NUCLEO-F446RE**, comparing directly with a C baseline. Questions to answer with generated artifacts and actual debug evidence:

- Cross-compilation, flash/debug route and target configuration.
- Linker script, reset/startup path, `.text/.rodata/.data/.bss` and section addresses.
- Compiled assembly and differences in runtime assumptions.
- Zig's explicit allocator model: fixed-buffer/arena allocation versus C/newlib-nano `malloc/free`.

**When:** after a stable C baseline, plausibly after much of Phase 1 or as a deliberately chosen short side experiment. **Not before Lab 3 by default.** Exact Zig version, MCU support and build workflow must be checked when attempted.

## Optional playground track — fun, independent experiments

**Proposed in the final SPI discussion, explicitly brought forward on 2026-10-18.** Arduino Uno V3 expansion connectors can help with shields; ST Morpho headers offer broader access to STM32 I/O. A shield fitting physically **does not guarantee electrical, pinout, voltage, library or firmware compatibility**. No shield listed below is confirmed purchased.

| Playground idea | What makes it interesting | Useful prior lab(s) | Physical/compatibility reminder |
|---|---|---|---|
| **CAN between two STM32 boards** | A concrete two-node ECU-like bus: CAN framing, filters, interrupts, arbitration, error counters and bus-off. | Lab 3; ideally Lab 4/5 for communication experience | STM32F446RE has an integrated **bxCAN controller**, but each node needs a compatible **external CAN transceiver**; check TX/RX mapping, 3.3 V logic, shared ground, bus termination and transceiver supply. A shield with its *own* CAN controller is another architecture. |
| **SPI TFT dashboard** | Make CPU load, IRQ counts, SPI traffic or heap measurements visible on a small screen; compare drawing methods and DMA. | Lab 5 | Prefer an explicitly STM32-compatible SPI breakout or verify shield SPI-header routing, voltage and display-controller support. |
| **DC motor / stepper control** | Physical motion and timing: timers/PWM, I2C driver, power domains and later encoder/feedback control. | Lab 3 or later | Motor driver/shield required; **do not power motors from Nucleo USB/GPIO**. Verify external motor supply, current limits and inductive-load protection. |
| **IMU sensor acquisition** | Actual periodic sampling, I2C/SPI, sensor IRQ, FIFO, calibration and timing. | Lab 3–5 | Check 3.3 V logic, breakout regulator/level shifting and MCU pin mapping. |
| **Ethernet over SPI** | STM32 → SPI Ethernet controller (e.g. W5500) → network; combine drivers, IRQ, buffers, protocol layers and throughput. | Lab 4/5 | Verify module/shield voltage levels, SPI and reset/interrupt pin mapping; Ethernet/network-stack setup is extra scope. |
| **External non-volatile memory via SPI (EEPROM or NOR flash)** | Connect a breadboard-friendly DIP-8 memory IC; write bytes, read back via SPI, inspect HAL buffers in the debugger, then power-cycle and verify persistence. Contrast normal SPI access (read via firmware into RAM) with optional later QUADSPI memory-mapped access. | After Lab 4 | **Proposed 2026-10-25 by learner**, not a purchase or new core lab. Candidate: Microchip 25LC256-I/P SPI EEPROM (32 KB, PDIP-8; check exact voltage, pinout and stock at purchase). Verify 3.3 V compatibility, WP/HOLD levels, decoupling and shared ground before wiring. |
| **Servo control** | Potentiometer/joystick → ADC → PWM or external I2C PWM driver → moving servo. | Lab 3 or later | A servo needs suitable separate power; check control-signal voltage, peak currents and common ground. |

**Ideas mentioned in the 2026-10-17 conversation:** CAN transceiver hardware is closest to automotive work; a little SPI TFT could become a reusable status display; a motor shield adds physical motion. Those were recommendations, **not a purchasing decision or agreed priority**. Buy only when a specific next-weekend experiment feels exciting.

**Playground rule:** a session can run in parallel with the core track at any time the learner chooses. It is never mandatory, never required for Lab N+1, and never expands the numbered core roadmap automatically. A very small self-contained goal (e.g. “a motor turns when a knob moves”) is sufficient.

## Platform and architecture interests

| Idea | Status / origin | Best revisiting point |
|---|---|---|
| **Cross-core boot, service ownership and lifecycle before/after OS/RTE** | **Planned** in Phase 2; prompted by real automotive architecture questions including CAN and Security Logger service ownership | Labs 9–12 and 15 |
| **Races, cache/coherency, memory barriers and why `volatile` is not synchronization** | **Planned** in accepted Phase 2 | Lab 11 |
| **MPU, isolation, private/shared memory, safety partitions and ASIL-related concepts** | **Planned** in Labs 6 and 13–14; user interest | Lab 6 and Phase 2; do not equate exercises with ASIL certification |
| **Heartbeat, fault detection vs containment, degraded operation and recovery** | **Planned** | Labs 14–15 |
| **Platform software architecture: `afw_core`, `vdb_core`, applications, configured/generated YAML interfaces, runtime dispatch, diagnostics/COM, MCAL and device drivers** | **Long-term learner interest; Phase 3 only Proposed** | After Phase 2, or small links at suitable earlier labs |
| **Walk data/control/fault/configuration paths end-to-end** | **Proposed** teaching approach for possible Phase 3 | Revisit after Phase 2 |
| **YAML/model → generated C/data → generic runtime framework** | **Proposed** architecture experiment | Possible Phase 3 |
| **Compare STM32 with Infineon AURIX TC39x/TC4x, traps and local/shared memory** | **Deferred**, reported in lab handoffs | Phase 2/3 or focused comparison |
| **Production bootloader/firmware updates, Secure Boot and hardware trust roots** | **Deferred**, reported after Lab 1 | After startup/reset/security foundations |
| **Understand MCAL/HSS/device driver layers against actual peripherals** | **Deferred** longer-term architecture aim | Phase 3 or a narrow post-SPI extension |

## Tools, environment and deeper mechanisms

| Idea | Status | Natural opportunity |
|---|---|---|
| **Raspberry Pi 5 + Argon ONE V3/NVMe as a dedicated lab host** | **Deferred possibility, not confirmed purchased**; current Windows setup works | When creating a permanent bench is worth the effort; never blocks an MCU lab |
| **VS Code + Arm GCC/GDB + OpenOCD/Cortex-Debug + PulseView on Linux** | **Proposed/Deferred** transparency-of-toolchain experiment | As part of optional Pi setup or later migration |
| **Minimal Linux toolchain “Lab 0”** | **Proposed**, not an extra required foundation lab | Only after a lab-host migration decision |
| **Systematic use of datasheets, reference manuals and errata** | **Deferred** as a standalone topic; useful ongoing practice | When pin mapping, peripheral status or fault investigation calls for it |
| **Lauterbach TRACE32 versus GDB/ST-LINK; optional J-Link** | **Deferred** comparison, no extra probe required or known available | After basic debugger skills; only buy on real need |
| **GDB command scripting / automated breakpoints** | **Deferred** after a non-blocking Lab 1 experiment | Optional GDB/OpenOCD exercise |
| **PulseView and real SPI signal decoding** | **Already planned** | Lab 4 |
| **Optional `__WFI()` and GPIO/logicanalyzer interrupt-response timing** | **Deferred**, not completed in Lab 3 | Optional future interrupt/timing experiment |
| **Hardware timer IRQs (TIM2/TIM3/TIM4/TIM5), cycle-accurate DWT timing and interrupt-latency/WCET exploration** | **Deferred after Lab 3**, not part of the measured 4.31× superloop-throughput comparison | A future timing-focused mini-lab, if wanted |
| **Button bounce/debounce with logic-analyzer evidence** | **Deferred after Lab 3**; not measured or implemented there | Optional peripheral/signal experiment |
| **Service-oriented communication / vsomeip comparison** | **Proposed after Lab 3**, not part of the core SPI/CAN plan | Future architecture discussion, if wanted |
| **Revisit local `static`, scope vs storage duration, stack/ABI/optimization** | **Deferred reinforcement** from observed Lab 2 misconception, corrected with evidence | Natural callback/task/buffer example; no repeat lab |
| **Allocation strategies, pools, fragmentation, lifetime and safety restrictions** | **Deferred** deeper mechanism | RTOS, Zig allocator comparison or platform software |
| **Extra USB-UART adapter / separate logging path** | **Proposed**, not required by existing ST-LINK setup | Only if a new experiment needs it |
| **ESD, labeled boards, reusable project boxes and a low-friction permanent bench** | **Agreed principle**; implementation varies | As actual cabling/projects grow |

## Maintaining this file

Record **who raised the idea**, whether it is a decided core lab or a suggestion, and **when it may be worth revisiting**. Do not convert an idea into a promise, new purchase or numbered core lab without an explicit decision. No deadlines. Keep private personal errands and employer-specific implementations out of this public repo.

If the course coordinator is replaced, this file plus the [roadmap](roadmap.md), [root current status](../README.md) and explicitly transferred `COURSE HANDOFF` reports carry the forward-looking context. They do **not** replace a detailed handoff of the actual pedagogical relationship.
