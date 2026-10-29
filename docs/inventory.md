# Lab equipment inventory

**Updated:** 2026-10-29

The **2026-09-30 DigiKey order has been received in full**, as confirmed by the learner. The list below groups small accessories rather than counting individual jumper wires, leads or resistors.

**Received** means physically delivered and available. It does **not** imply that every item has been powered on, configured or function-tested.

## Development boards

| Item | Quantity | Status / notes |
|---|---:|---|
| ST NUCLEO-F446RE (STM32F446RE / Cortex-M4) | 2 | Received and both used successfully as SPI1 master/slave in Lab 4. ST-LINK/debugger connections were identified per board; see [board-identities.md](board-identities.md). |

## Measurement and debugging

| Item | Quantity | Status / notes |
|---|---:|---|
| SparkFun TOL-18627 8-channel, 24 MHz USB logic analyzer | 1 | Received, configured and successfully used with PulseView/sigrok for 16 MHz-sampled SPI1 traces in Lab 4; actual captures saved in [lab04-spi-polling/captures/](../lab04-spi-polling/captures/). Digital logic capture does not establish analog signal integrity. |
| Digital multimeter | 1 | Received. Useful for wiring and continuity checks. |
| USB-A → Mini-B cables | 2 | Received. Intended for the two Nucleo boards and their onboard ST-LINK connections. |
| Mini-grabber test leads | Set | Received. Useful for logic-analyzer probing. |

## Prototyping and components

| Item | Quantity | Status / notes |
|---|---:|---|
| 830-point breadboard | 1 | Received. |
| Dupont jumper wires (M/M, M/F and F/F) | Assorted sets | Received; individual wires not inventoried. |
| Adafruit Parts Pal | 1 kit | Received; individual components not inventoried. |
| SparkFun 500-piece resistor kit | 1 kit | Received; individual resistors not inventoried. |

## Workbench and storage

| Item | Quantity | Status / notes |
|---|---:|---|
| ESD work mat | 1 | Received. |
| Ledsavers LED magnifying lamp (570 lm) | 1 | Purchased from Kjell & Company and in use as of 2026-10-24. Illuminated, arm-mounted magnifier for reading fine board markings and handling small connectors during Lab 4. |
| Component organizer boxes | 2 | Received. |

## Status and scope

- **Outstanding items from the 2026-09-30 DigiKey order:** None reported.
- **Known-good development setup:** Windows 11, STM32CubeIDE 2.2.0, STM32CubeMX 6.18.1, both NUCLEO-F446RE boards with onboard ST-LINK and SparkFun logic analyzer/PulseView were used successfully in Lab 4.
- **Next hardware milestone:** Reuse the already working two-board SPI wiring and PulseView captures for **Lab 5 — SPI interrupts & DMA**, preserving the polling reference. No new purchases are required.
- **Practical wiring notes:** See [hardware-safety.md](hardware-safety.md) for Lab 4 observations about Morpho connector orientation, apparent back-powering and instrument grounding. These are practical recommendations, not new course requirements.
- **Not listed as owned:** Optional Raspberry Pi/Linux bench, future CAN/TFT/motor/IMU/Ethernet/servo shields, or the proposed dual-core Phase 2 board. These remain ideas, not confirmed purchases.

The original order checklist remains in [shopping-list.md](shopping-list.md). This inventory is the authoritative record of **equipment physically received**.
