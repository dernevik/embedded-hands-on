# Hardware wiring and measurement safety

**Updated:** 2026-10-26  
**Status:** Mandatory pre-power checklist for hands-on board-to-board experiments, especially Lab 4 (SPI polling).

> **Stop before applying USB power if a connector position, GND reference, voltage level or board power state is uncertain.** Firmware and debug success do not substitute for electrical verification.

## Before connecting or changing any wires

- [ ] **Power down every board and instrument involved**, including USB-connected ST-LINKs and the logic analyzer. Disconnect external supplies. Remember that USB cables can provide common ground and alternate current paths.
- [ ] **Consult the official board schematic/connector pinout**, with the physical orientation identified (USB end, inner vs outer row, pin 1). Do not derive a pin number solely from the nearest silkscreen text, a photo or a row count.
- [ ] Write down the complete **physical connector → pin number → MCU pad → function** mapping *for both ends* of every wire. Arduino D10 is not a universal STM32 pin identifier.
- [ ] **Verify GND electrically** with the multimeter in continuity/ohms mode **only while fully unpowered**: from the proposed ground probe point to a known board GND. Check for unintended shorts between rails/GND before power-up where practical. If a meter cannot be used, defer uncertain connections; do not treat assumptions as proof.
- [ ] Verify **probe/input pinout on the instrument itself**, especially connector viewpoint and any model-specific errata. Check correct logic voltage range. Never connect a signal to an assumed 'GND' pin without identification.
- [ ] Check **power-domain sequencing**: no powered MCU output should drive an unpowered MCU or peripheral unless the datasheet explicitly supports that state (e.g. documented power-off-tolerant I/O). Back-powering through input-protection networks may partially energize hardware and cause unreliable behavior or damage.
- [ ] Check for **output-to-output contention**; assign a single driver per line and establish CS/NSS idle state. Use common signal ground, but **do not connect separate power rails together** without a documented reason.
- [ ] Before power-up, check that wires are seated and that no loose jumper ends can touch exposed header pins. Start with the simplest connection and increase complexity only after a known-good test.
- [ ] After power-up, stop immediately for unexpected power LED behavior, hot components, resets, unexplained disconnects or failed SWD. **Power off and recheck wiring**, rather than repeatedly resetting or debugging with a potentially unsafe connection.

## Lab 4 verified reference: STM32 NUCLEO-F446RE

The Arduino header and ST Morpho header are *different connectors* with different numbering. The SPI master uses **GPIO PB6 as software CS**, not automatic hardware NSS.

| Signal | Arduino connection on master | MCU pad | Parallel ST Morpho CN10 connection on master |
|---|---|---|---|
| CS | CN5 D10 | PB6 | CN10 pin 17 (inner, row 9 from ST-LINK end) |
| SCK | CN5 D13 | PA5 | CN10 pin 11 (inner, row 6) |
| MISO | CN5 D12 | PA6 | CN10 pin 13 (inner, row 7) |
| MOSI | CN5 D11 | PA7 | CN10 pin 15 (inner, row 8) |
| **GND** | Arduino POWER GND | GND | **CN10 pin 9 (inner, row 5)** |

**Critical adjacent pin:** **CN10 pin 7 (inner, row 4) is AVDD, *not* GND.** A one-row offset at the top of CN10 can connect the analyzer's ground lead to AVDD. Refer to the NUCLEO-64 official pinout/schematic (ST UM1724) and verify in hardware before power-up. Do not assume odd/even pin numbering from the physical appearance of staggered rows.

On the slave, SPI1 hardware NSS is **PA4 / Arduino A2**. CS connects **master D10/PB6 → slave A2/PA4**. Both boards must be normally powered when the SPI lines are connected and driven.

**SparkFun TOL-18627:** The connector label shows CH0/CH1, CH2/CH3, CH4/CH5, CH6/CH7 and a bottom GND/GND pair. The SparkFun product documentation cautions that **the GND position adjacent to CH6 can be miswired on some units and may output a clock**; avoid it. Use the GND position next to CH7 only after verifying connector orientation and electrical identity. Never infer GND simply from wire color or PulseView channel colors.

## Lab 4 incident and lessons (2026-10-25/26)

During initial logic-analyzer wiring, the intended CN10 ground position on the master was apparently **one row too high**: the conductor believed to be GND likely went to **CN10 pin 7 / AVDD**, rather than **pin 9 / GND**. A 'target no device found' ST-LINK/SWD error appeared while the analyzer was connected and later resolved after the analyzer leads were shifted down one row. **The exact electrical current path and any damage were not measured or established.** A working debugger and logic trace after reconnection do not certify that all components are undamaged.

During a separate test, the **slave board's power LED was observed blinking despite its USB supply being disconnected**, while the master toggled PB6 and the inter-board SPI wires were still attached. **Back-powering via a driven signal into an unpowered target is a plausible explanation, not an electrically proven diagnosis.** Inter-board lines were subsequently disconnected for isolated testing.

The learning objective is a repeatable process, not an assumption about who should have caught a mistake:

1. Treat **schematic/pinout inspection + ground/voltage/power-state verification** as a *mandatory gate* before energizing hardware.
2. Separate **functional proof** (UART/SPI exchanges, debugger, logic traces) from **electrical safety proof**.
3. Do not trust a visual/photo-based 'looks right' sign-off as sufficient, particularly with two-row connectors.
4. Make the instructor explicitly ask for power-state and ground verification, not merely give a warning after suspicious behavior.

## Note to the course coordinator / future Lab 4 handoff

**Carry this incident and its preventive checklist into `COURSE HANDOFF — LAB 4`.** The Lab 4 instructor should report both the apparent one-row CN10/AVDD-versus-GND connection error and the observed likely back-powering of an unpowered slave. Note that the assistant's earlier photo-based assurances and instructions were insufficiently conservative. Make diagram verification, multimeter continuity checks and powering *all* linked boards explicit safety gates for subsequent hardware labs. **Do not redesign the agreed lab sequence**; apply the lesson as a cross-lab bench practice.

This document is an operational safety note, not a substitute for the device datasheets and electrical absolute maximum ratings.
