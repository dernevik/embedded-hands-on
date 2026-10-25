# Board identities and ST-LINK debug mapping

**Updated:** 2026-10-25  
**Purpose:** Map physical NUCLEO-F446RE boards to their onboard ST-LINK debug probes, so the correct firmware is programmed to the correct board during two-board experiments.

## Physical board identities

| Physical label | Role in Lab 4 | Onboard ST-LINK S/N | Verification |
|---|---|---|---|
| **Slave** | `lab04_spi_slave` | `066DFF505487525067192137` | Found using **Scan** in STM32CubeIDE's **ST-LINK S/N** selector while the slave board was the only board connected (2026-10-25). The same value was seen earlier in STLinkUpgrade. |
| **Master** | `lab04_spi_master` | **To be recorded** | Identify by connecting only the master board, then scanning in its own debug configuration. Do not copy the slave S/N. |

These are **ST-LINK debug-probe serial numbers**, not STM32F446RE MCU unique device IDs. The ST-LINK probe is part of the NUCLEO board; its S/N is normally stable across target firmware flashes and ST-LINK firmware updates. Recheck if a board/probe is replaced or a tool reports a different ID.

## Recommended per-project STM32CubeIDE configuration

For each project, with **only the intended board connected over USB** during initial identification:

1. Open **Run → Debug Configurations…** and select that project's existing debug launch configuration.
2. In **Debugger**, select the ST-LINK GDB server with **SWD**.
3. Check **ST-LINK S/N**, press **Scan**, and select the detected S/N.
4. Click **Apply** to save it.
5. Once both board IDs are recorded, verify that the slave and master configurations have **different** S/N values.

| Debug launch configuration | Expected S/N |
|---|---|
| `lab04_spi_slave Debug` | `066DFF505487525067192137` |
| `lab04_spi_master Debug` | **To be recorded** |

Binding a launch configuration to a specific ST-LINK helps prevent accidentally debugging or flashing the other board: if the selected probe is not available, the launch should fail rather than silently targeting a different probe. This setting belongs to the **debug launch configuration**; it is **not a global lock** on all programming methods. Verify the selected target when using other launch configurations, programming tools or workspaces. The launch configuration may be stored locally in the CubeIDE workspace rather than automatically versioned in this repository.

## Lab 4 operating practice

- Each NUCLEO is powered/programmed via its own USB/ST-LINK; the boards share **GND** for SPI signalling.
- Keep the boards physically labeled **Master** and **Slave**.
- There is no requirement to run two simultaneous debug sessions for the initial SPI polling lab.
- The current master application performs its SPI exchange once shortly after startup. When powering/resetting master, arrange for the slave firmware to be **running** (not debugger-suspended) and armed for the transfer.
- Keep SPI pins disconnected when changing firmware or investigating an unknown GPIO configuration.

Related equipment list: [inventory.md](inventory.md). These S/N values uniquely identify physical debug hardware; avoid posting them in unrelated public logs/screenshots when not needed.
