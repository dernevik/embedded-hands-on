# Lab equipment inventory

**Updated:** 2026-10-29

The **2026-09-30 DigiKey order has been received in full**, as confirmed by the learner. The list below groups small accessories rather than counting individual jumper wires, leads or resistors.

**Received** means physically delivered and available. It does **not** imply that every item has been powered on, configured or function-tested.

## Development boards

| Item | Quantity | Status / notes |
|---|---:|---|
| ST NUCLEO-F446RE (STM32F446RE / Cortex-M4) | 2 | Received and both used successfully in the Lab 4 master/slave SPI1 experiment. Board identities and debugger bindings are recorded in [board-identities.md](board-identities.md). |

## Measurement and debugging

| Item | Quantity | Status / notes |
|---|---:|---|
| SparkFun TOL-18627 8-channel, 24 MHz USB logic analyzer | 1 | Received, configured on Windows with PulseView/sigrok and successfully used at 16 MHz sampling in Lab 4. Verified SPI captures are saved in [Lab 4 captures](../lab04-spi-polling/captures/). Digital capture does not establish analog signal integrity. |
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
- **Known-good development setup:** Windows 11, STM32CubeIDE 2.2.0, STM32CubeMX 6.18.1, two NUCLEO-F446RE/ST-LINK targets and SparkFun logic analyzer with PulseView were used successfully in **Lab 4**.
- **Next hardware milestone:** Reuse the verified two-board SPI wiring and capture setup for **Lab 5 — SPI interrupts and DMA**. The exact analog mechanism underlying Lab 4's earlier analyzer-correlated bit-shift fault was not established; see [hardware-safety.md](hardware-safety.md) and [Lab 4 notes](../lab04-spi-polling/notes.md).
- **Not listed as owned:** Optional Raspberry Pi/Linux bench, future CAN/TFT/motor/IMU/Ethernet/servo shields, or the proposed dual-core Phase 2 board. These remain ideas, not confirmed purchases.

The original order checklist remains in [shopping-list.md](shopping-list.md). This inventory is the authoritative record of **equipment physically received**.
