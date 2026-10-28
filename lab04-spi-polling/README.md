# Lab 04 — SPI Polling Between Two STM32 Boards

**A reproducible, hands-on experiment using two NUCLEO-F446RE boards, STM32 HAL and an optional USB logic analyzer.**

The goal is to understand the complete path from application buffers through a polling SPI driver and peripheral hardware to actual SCK/MOSI/MISO signals. The final firmware implements a two-transaction **request → process → response** exchange. It runs **once per reset**, intentionally; this is not yet a continuously operating communication stack.

The repository includes both working CubeIDE projects, their CubeMX `.ioc` configurations, and real PulseView captures. You do not need to write the firmware from scratch to reproduce the results.

## 1. Learning objectives

By the end, you should be able to:

1. Connect two STM32 MCUs as SPI master and slave, with correct CS/NSS and common ground.
2. Explain SPI mode 0, full-duplex shifting, and the relationship between bytes and SCK pulses.
3. Observe blocking HAL polling, `TXE`/`RXNE` and `TxXferCount`/`RxXferCount` in a debugger.
4. Distinguish data already handed to the SPI peripheral from bits actually transferred on the wires.
5. Verify two transactions in both MCU memory and an independent logic-analyzer capture.
6. Explain why a computed response must be scheduled after the slave has processed the request, and what a real protocol would need for synchronization and error handling.

Prerequisites: basic C, building/flashing an STM32 project, and basic debugger familiarity. No prior SPI experience is necessary.

## 2. Hardware and tools

### Required

- **2 × ST NUCLEO-F446RE** (STM32F446RE): one master, one slave.
- Two USB data cables to power/program the boards via their onboard ST-LINK interfaces.
- Female/male jumper leads suitable for the Arduino headers.
- A development PC with **STM32CubeIDE** and the STM32CubeF4 HAL firmware package.
- Git.

### Recommended for direct signal verification

- A **3.3 V-compatible, four-channel-or-more USB logic analyzer**.
- **PulseView** (sigrok) with an SPI protocol decoder.

The reference setup used Windows 11, STM32CubeIDE **2.2.0**, standalone STM32CubeMX **6.18.1**, STM32CubeF4 **1.28.3** and a SparkFun **TOL-18627** 8-channel logic analyzer. Other versions and compatible analyzers may work, but the details of import, drivers and triggering may differ.

You can complete the firmware/debugger part without a logic analyzer. The saved captures are included for offline examination.

## 3. Wiring — two NUCLEO-F446RE boards

**Power down both boards while making or changing connections.** Label them **MASTER** and **SLAVE**. Each board is normally powered via its **own USB/ST-LINK** connection; connect the grounds, **not the boards' 3.3 V or 5 V power rails**.

Use these exact MCU signals. On this board, the SPI signals are conveniently available on the Arduino headers.

| Signal | Master MCU / Arduino | Connect to slave MCU / Arduino | Purpose |
|---|---|---|---|
| **SCK** | `PA5` / **D13** | `PA5` / **D13** | Master-generated clock |
| **MOSI** | `PA7` / **D11** | `PA7` / **D11** | Master → slave |
| **MISO** | `PA6` / **D12** | `PA6` / **D12** | Slave → master |
| **CS / NSS** | `PB6` / **D10** | `PA4` / **A2** | Master GPIO output → slave hardware NSS input |
| **GND** | Arduino POWER **GND** | Arduino POWER **GND** | Common electrical reference |

Do **not** cross MOSI and MISO by their names: the STM32 master and slave pin functions already define their directions. In this mapping, MOSI connects to MOSI and MISO to MISO.

**Important details:**

- On the master, `PB6` is a **manually controlled GPIO chip select**, not SPI hardware NSS. It idles HIGH.
- On the slave, `PA4` is configured as a **hardware NSS input**, active LOW.
- `PA5` is also shared with the onboard green LD2 user LED; in this lab it belongs to **SPI1_SCK**, not an independently controlled LED.
- Avoid powering one MCU's I/O from another MCU while that MCU is unpowered. Verify the actual connector positions against the NUCLEO pinout.

See [hardware wiring and measurement guidance](../docs/hardware-safety.md) and the official ST NUCLEO-64 documentation for physical pin identification. These are practical precautions, not additional graded lab gates.

## 4. Obtain and import the projects

From the repository root:

```bash
git clone https://github.com/dernevik/embedded-hands-on.git
cd embedded-hands-on
```

If you already have the repository, use `git pull` to get the latest firmware and captures.

The relevant layout is:

```text
lab04-spi-polling/
├── README.md
├── captures/
│   ├── spi_two_bytes_verified.sr
│   └── spi_request_response_verified.sr
└── firmware/
    ├── lab04_spi_master/
    │   ├── lab04_spi_master.ioc
    │   ├── .project / .cproject
    │   ├── Core/
    │   └── Drivers/
    └── lab04_spi_slave/
        ├── lab04_spi_slave.ioc
        ├── .project / .cproject
        ├── Core/
        └── Drivers/
```

In **STM32CubeIDE**:

1. Choose **File → Import → General → Existing Projects into Workspace** (menu wording may vary).
2. Select the repository's `lab04-spi-polling/firmware` directory as the root to search, or import each project directory individually.
3. Import both **`lab04_spi_master`** and **`lab04_spi_slave`**.
4. Check that the STM32F4 firmware package is installed. If prompted to migrate project metadata or regenerate code, review the changes; the committed sources are the reference implementation.
5. Select each project and run **Project → Clean**, then **Build Project**. Both should build without errors.

The projects are already configured and contain application code in CubeMX `USER CODE` regions. Opening the `.ioc` files in standalone CubeMX is useful for inspecting the configuration, but **regeneration is not required** for this reproduction.

## 5. Verify the SPI configuration

Both projects use **SPI1**, two-wire full duplex, **8-bit frames**, MSB first, **mode 0** (`CPOL=0, CPHA=0`) and no SPI CRC.

| Setting | Master | Slave |
|---|---|---|
| Mode | `SPI_MODE_MASTER` | `SPI_MODE_SLAVE` |
| Direction | `SPI_DIRECTION_2LINES` | `SPI_DIRECTION_2LINES` |
| Data size | `SPI_DATASIZE_8BIT` | `SPI_DATASIZE_8BIT` |
| Polarity / phase | LOW / first edge | LOW / first edge |
| NSS | `SPI_NSS_SOFT` + manual `PB6` | `SPI_NSS_HARD_INPUT` on `PA4` |
| First bit | MSB | MSB |
| SPI1 clock | APB2 at 84 MHz; prescaler 256 | Provided externally by master |
| HAL timeout | 100 ms per transfer | `HAL_MAX_DELAY` |

The nominal SCK frequency is **84 MHz / 256 = 328.125 kHz**. Each two-byte transfer uses **16 clock pulses**, taking about **49 µs** of clock activity. The GPIO CS idle state on the master is HIGH.

## 6. Program both boards and perform the experiment

### 6.1 Make sure each IDE project targets the intended board

With two identical ST-LINK probes attached, take care not to program the wrong board.

In **Run → Debug Configurations → [project] → Debugger**, use **ST-LINK S/N → Scan** and select the correct probe for each physical board. Identifying each probe with only its board connected is a useful first-time check. Do not copy another person's S/N values.

The original lab's board identities are recorded in [`docs/board-identities.md`](../docs/board-identities.md); your own boards will have different serial numbers.

### 6.2 Flash and arm the slave first

1. Build/program **`lab04_spi_slave`** to the SLAVE board.
2. Start it or press its RESET button, and let it **run without being suspended**.
3. The slave enters its first blocking `HAL_SPI_TransmitReceive()` and waits for the master to clock the request.

The slave's application executes **two** blocking SPI calls, in sequence:

- Receive the request while transmitting predefined test bytes `A5 5A`.
- Compute a response from the received request, then wait in a second SPI call to transmit that response.

### 6.3 Trigger the master

1. Build/program **`lab04_spi_master`** to the MASTER board.
2. Make sure the slave is already running and waiting.
3. Reset or resume the master so it executes the application. If you are using the debugger, let the whole transaction pair run without an intervening breakpoint.
4. Suspend the master **after** the exchanges to inspect its buffers.

The master performs the following sequence **once after reset**:

1. Initialize GPIO and SPI1.
2. Explicitly enable SPI using `__HAL_SPI_ENABLE(&hspi1)` and wait **1 ms** before asserting CS. This ordering was important in the verified setup.
3. Assert CS LOW; transmit `01 37` while receiving `A5 5A`; deassert CS HIGH.
4. Wait **1 ms** for the slave to compute and prepare its response.
5. Assert CS LOW again; send dummy bytes `00 00` to generate 16 more SCK pulses; receive `81 37`; deassert CS HIGH.
6. Remain in the infinite application loop. **No further transactions occur until another reset.**

The 1 ms gap is an experimental timing margin, **not a ready/busy handshake**. If one transfer fails, reset the slave and arm it again before repeating the master exchange. Resetting only the master can leave it communicating with a slave that has already completed its one-shot sequence.

## 7. Expected result — check both MCUs

In CubeIDE, add the following variables to the **Expressions** view, or inspect them while the CPU is suspended after the transfers.

### Master

| Variable | Expected value |
|---|---|
| `master_tx[0..1]` | `01 37` |
| `master_rx[0..1]` | `A5 5A` |
| `dummy_tx[0..1]` | `00 00` |
| `response_rx[0..1]` | `81 37` |
| `spi_status` | `HAL_OK` |
| `response_status` | `HAL_OK` |
| `GPIOB->ODR & GPIO_PIN_6` | `0x40` (CS HIGH again) |

### Slave

| Variable | Expected value |
|---|---|
| `slave_tx[0..1]` | `A5 5A` |
| `slave_rx[0..1]` | `01 37` |
| `response_tx[0..1]` | `81 37` |
| `response_rx[0..1]` | `00 00` |
| `spi_status` | `HAL_OK` |
| `response_status` | `HAL_OK` |

The slave computes the response only if its first transfer returned `HAL_OK` and the command byte was `0x01`:

```c
response_tx[0] = slave_rx[0] | 0x80U;
response_tx[1] = slave_rx[1];
```

For a failed/unsupported first request, the committed slave application instead prepares `FF 00`. The master application does not implement application-level retries or response validation.

**Important:** The SLAVE's `response_status` is updated only **after** its second `HAL_SPI_TransmitReceive()` returns. While still suspended *inside* that function, the variable retains its previous initialized value. That alone does not indicate a failed transfer.

## 8. Observe the physical exchange with PulseView (recommended)

A logic analyzer connects as a **high-impedance observer**, sharing the circuit ground. In the verified setup, it probed the **MASTER** board.

### 8.1 Analyzer wiring

| Analyzer channel | Signal | Master pin |
|---|---|---|
| **D0** | CS | `PB6` |
| **D1** | SCK | `PA5` |
| **D2** | MOSI | `PA7` |
| **D3** | MISO | `PA6` |
| **GND** | Ground reference | A verified board GND |

The original setup used the **inner (odd-numbered) row of master ST Morpho CN10**:

| Connection | CN10 pin |
|---|---|
| GND | **9** |
| SCK / PA5 | **11** |
| MISO / PA6 | **13** |
| MOSI / PA7 | **15** |
| CS / PB6 | **17** |

**Pin 7 on this header is AVDD, not GND.** Check the physical header orientation and ST schematic rather than relying only on row counting.

For the SparkFun TOL-18627, pay attention to the analyzer's own GND labeling: the position adjacent to **CH6** has a reported connector issue on some units. The verified setup used the ground position next to **CH7**, after checking orientation. Details: [hardware-safety.md](../docs/hardware-safety.md).

### 8.2 PulseView settings

1. Select your supported analyzer driver/device (the original FX2-based SparkFun unit worked as **Saleae Logic** with a suitable Windows USB driver).
2. Enable digital channels **D0–D3**.
3. Set sample rate to **16 MHz**. **8 MHz** is also sufficient for this ~328 kHz SPI bus.
4. Start with **100k samples** (about **6.25 ms** at 16 MHz), which covers our ~1 ms gap; increase the recording window if needed.
5. Configure a **falling-edge trigger on D0 (CS)**.
6. Add an **SPI protocol decoder**:
   - Clock = **D1**
   - MOSI = **D2**
   - MISO = **D3**
   - CS = **D0**, active LOW
   - **CPOL = 0, CPHA = 0**, MSB first, **8 bits** per frame.
7. Arm PulseView acquisition, **reset the slave**, and then reset/resume the master, letting it run through both transactions uninterrupted.

The order matters: the analyzer should be armed when CS falls, and the slave must be ready. If a breakpoint halts the CPU between CS assertion and the clock pulses, the capture can end before a complete transfer.

### 8.3 Expected decoded traffic

| CS-low transaction | MOSI (master → slave) | MISO (slave → master) |
|---|---|---|
| **1 — request** | `01 37` | `A5 5A` |
| **2 — read response** | `00 00` | `81 37` |

Expect **two CS-low windows**, each containing **16 SCK pulses**, with CS HIGH between them. SPI mode 0 samples on the rising edge; the first bit must be present before the first rising edge.

### 8.4 Open the committed reference captures

The repository contains:

- [`captures/spi_two_bytes_verified.sr`](captures/spi_two_bytes_verified.sr) — the original single two-byte full-duplex exchange.
- [`captures/spi_request_response_verified.sr`](captures/spi_request_response_verified.sr) — both request and computed-response transactions.

Open these files in PulseView to inspect the recordings independently of your own hardware. If the saved decoder annotations are not shown, add an SPI decoder with the mappings above.

## 9. An optional debugger experiment: what does polling actually do?

With the slave running and armed **before** the master transmits, pause the slave *inside* its first or second `HAL_SPI_TransmitReceive()`.

Before any new SCK pulses, a useful observation from the original lab was:

```text
hspi1.TxXferCount = 1
hspi1.RxXferCount = 2
```

Why isn't `TxXferCount` still 2? HAL has already written the first byte to the peripheral's data register; the actual clocked exchange need not have started. The receive counter remains 2 because no data frame has been received yet.

Look at SPI status flags such as:

- `TXE`: transmit buffer ready for more data.
- `RXNE`: a received data frame can be read.
- `BSY`: ongoing communication activity.
- `SPE`: peripheral enabled.

These represent *peripheral state*, while the HAL counters represent *software-managed transfer progress*.

**Reset/re-arm both sides before returning to the normal experiment** if your debugger actions interrupted timing. Don't place breakpoints inside the CS-low communication window when trying to verify the unmodified timing.

## 10. Troubleshooting

| Symptom | Likely explanation / next check |
|---|---|
| Master receives `00 00` or a status is not `HAL_OK` | Check both boards are powered, the correct master/slave firmware was flashed, wiring is correct, and the slave is running **before** master reset. Check which status belongs to which transaction. |
| First transaction succeeds but second response is unexpected (e.g., `5A 5A` in an earlier trial) | Slave may already have completed its one-shot call sequence or have been reset at the wrong time. Reset/re-arm the slave, then run the master once. |
| Wrong board enters the debugger | Identify the correct ST-LINK serial number in each CubeIDE Debug Configuration. |
| PulseView shows no useful SPI data | Check analyzer selection, trigger on D0 falling, enabled channels, and **sample rate**. PulseView was once restarted at **20 kHz**, far too slow for 328 kHz SPI. |
| PulseView only shows a short CS pulse or no SCK | Make sure the CPU is not suspended between setting CS LOW and generating clock pulses. Verify trigger and capture window. |
| Bytes appear shifted (e.g., `4A B4` on MISO, `80 9B` at slave RX) | Check the committed master's **enable SPI → delay → assert CS** sequence, NSS/SCK wiring and analyzer connections. Such corruption was reproduced in the lab; the exact electrical root cause was **not** proven. |
| Analyzer connection changes communication behavior | Measurement equipment can change loading/ground paths. Compare controlled tests with and without the analyzer and consult its pinout. A digital capture cannot reveal very narrow analog glitches. |
| ST-LINK reports target not found after attaching analyzer leads | Verify the physical CN10 numbering, especially **GND pin 9 vs AVDD pin 7**, and check power/wiring as described in the hardware guidance. |

### A lesson from the original investigation

The master initially sent `01 37`, but with one analyzer connection state the slave received `80 9B` and MISO showed `4A B4` instead of `A5 5A`. The bit patterns were consistent with an unintended clock edge.

Explicitly enabling master SPI **before** asserting CS, and allowing SCK to settle for 1 ms, eliminated the failure in the tested setup. Because these were two changes introduced together, and no analog glitch was captured, the precise root cause remains a **hypothesis**, not a proved signal-integrity diagnosis.

## 11. Scope, limitations and next steps

This reference application intentionally uses **blocking/polling HAL**, performs **one request and one response per reset**, and assumes the slave can prepare the response within the 1 ms inter-transaction delay.

It does **not** implement:

- Continuous transactions or a slave command-processing state machine.
- READY/BUSY handshaking, request IDs, retries or robust timeout recovery.
- Interrupt- or DMA-driven SPI.
- Application-layer integrity checks or a production communication protocol.

Possible extensions include polling a BUSY/READY response, adding a READY GPIO, repeated transfers in a loop, or replacing blocking calls with interrupt/DMA APIs in a later lab.

The main educational result is the end-to-end correlation of **application RAM → HAL polling and SPI registers → physical wires → received RAM**, with independent verification in the debugger and PulseView.
