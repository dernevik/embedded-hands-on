# Lab 03 — GPIO Polling, External Interrupts and Exception Debugging

**A self-contained, hands-on lab for the STM32 NUCLEO-F446RE (Arm Cortex-M4).**

In this lab you first read the onboard USER button by polling GPIO, then change the same project to use an external interrupt (EXTI). You inspect the complete path from the physical pin to the interrupt handler, observe the automatic exception stack frame, pass events from an ISR to the main loop, and deliberately inject an event-handling fault. An optional final experiment measures the performance of the two implementations.

**No AI tutor is required.** The steps include expected observations, debugger commands, explanations and troubleshooting.

## 1. Learning objectives

After completing the lab you should be able to:

1. Explain the difference between a GPIO **level** and a hardware **edge event**.
2. Configure the NUCLEO USER button for polling and then EXTI.
3. Trace an interrupt through EXTI, NVIC, the vector table, the generated IRQ handler and the HAL callback.
4. Confirm that the CPU is in Handler mode and inspect the interrupted Thread-mode context.
5. Observe exception entry, the saved register frame and exception return in a debugger.
6. Move application work out of an ISR and into the main loop.
7. Explain the difference between processing *every* event and *coalescing* multiple events.
8. Compare two implementations using reproducible instrumentation without confusing loop throughput with interrupt latency.

This lab assumes basic C and familiarity with building and debugging firmware. You do **not** need previous experience with STM32 interrupt registers.

## 2. Hardware and software

### Hardware

- One **ST NUCLEO-F446RE** board (STM32F446RE, Arm Cortex-M4).
- USB data cable connected to the board's **onboard ST-LINK** connector.
- A development PC.

No jumper wires, breadboard, additional button or external LED are needed.

| Board resource | MCU connection | Purpose |
|---|---|---|
| USER push-button (B1) | **PC13** | Input / EXTI line 13 |
| Green user LED (LD2) | **PA5** | Visible output |
| Onboard ST-LINK | SWD | Programming and debugging |

The verified hardware reports **PC13 = 1 when released** and **PC13 = 0 when pressed**. Thus a press causes a **falling edge**. These are observations for this board/project; always verify the signal on your hardware.

### Software

This guide was developed with:

- Windows 11 and PowerShell 7.
- **STM32CubeIDE 2.2.0** (including GCC, GDB and ST-LINK debugging).
- **Standalone STM32CubeMX 6.18.1**.
- STM32CubeF4 firmware package **FW_F4 1.28.3**.
- Git.

Other versions may work, but screens, generated files and build behavior can differ. The verified workflow uses standalone CubeMX because the CubeIDE version above does not provide the older integrated CubeMX editing workflow.

## 3. Obtain and open the firmware project

Repository: [dernevik/embedded-hands-on](https://github.com/dernevik/embedded-hands-on)

Project directory:

```text
lab03-interrupts/
├── README.md
├── notes.md
└── firmware/
    └── lab03_interrupts/
        ├── lab03_interrupts.ioc
        ├── .project / .cproject
        ├── Core/
        ├── Drivers/
        ├── STM32F446RETX_FLASH.ld
        └── ...
```

The Git repository includes the actual CubeIDE project, HAL/BSP sources, MCU startup assembly and linker script. Generated build products (`Debug/`, `Release/`) are intentionally excluded.

### Option A — Quickly run the completed EXTI application

Use the current `main` branch:

```powershell
git clone https://github.com/dernevik/embedded-hands-on.git
cd embedded-hands-on
```

Continue with **Import and build** below. The finished application is interrupt-based; it is a useful reference or recovery point.

### Option B — Reproduce the experiments from the original polling baseline (recommended)

Clone the repository, then create a separate exercise branch **from the known-good polling commit**:

```powershell
git clone https://github.com/dernevik/embedded-hands-on.git
cd embedded-hands-on
git switch -c lab03-walkthrough fd96486
```

Commit `fd96486` contains the tested polling implementation before the move to EXTI. Working on a separate branch protects `main`. You may keep this README open in your browser because the historical commit predates its addition.

**Important:** Switch branches *before* importing the firmware project into the IDE; do not switch source trees while CubeIDE or CubeMX has files open.

### Import and build

1. Connect the board to the PC using the ST-LINK USB connector.
2. Open STM32CubeIDE.
3. Choose **File → Import → General → Existing Projects into Workspace**.
4. Select the existing folder `lab03-interrupts/firmware/lab03_interrupts` under your clone.
5. Select the detected `lab03_interrupts` project.
6. **Uncheck "Copy projects into workspace".** The repository should remain the project's actual location.
7. Select the **Debug** build configuration and build (usually **Ctrl+B**).
8. Start a Debug session using the onboard ST-LINK/SWD connection. Allow CubeIDE to flash the generated ELF file.
9. Resume execution (**F8**) when the debugger initially stops at `main()`.

**Expected:** Successful build and flashing, with no separate debug probe or external wiring. If working from Option A, see the interrupt behavior in Section 7; for Option B, continue in order.

Keep custom changes inside CubeMX-generated `USER CODE BEGIN ...` / `USER CODE END ...` regions when possible. Otherwise regenerating the project can overwrite them.

## 4. Experiment A — Read the button by polling

**Starting point:** Option B, commit `fd96486`. The board button is initialized with:

```c
BSP_LED_Init(LED2);
BSP_PB_Init(BUTTON_USER, BUTTON_MODE_GPIO);
```

The infinite loop contains:

```c
button_state = BSP_PB_GetState(BUTTON_USER);

if (button_state != 0U)
{
    BSP_LED_On(LED2);
}
else
{
    BSP_LED_Off(LED2);
}

background_counter++;
```

`button_state` and `background_counter` are global `volatile uint32_t` variables in this baseline.

### Run the experiment

1. Build, flash and resume.
2. Observe LD2 with the button released and pressed.
3. Suspend the CPU. Add `button_state` and `background_counter` to **Expressions**.
4. Resume, press/release the button, suspend again and inspect both values.
5. Add this expression to inspect the physical PC13 bit independently:

   `(GPIOC->IDR >> 13) & 1U`

**Expected:** Button released gives pin value 1; pressed gives 0. The polling program turns the LED **on when released** and **off while pressed**. The background counter grows while execution runs.

When the CPU is suspended, the GPIO level may change if you operate the button, while the LED and `button_state` still reflect the *last completed loop iteration*. That is not a hardware contradiction: the application is not running while suspended.

### Follow the read through the drivers

Find these functions using CubeIDE's Open Declaration or search:

- `BSP_PB_GetState` in `Drivers/BSP/STM32F4xx-Nucleo/stm32f4xx_nucleo.c`.
- `HAL_GPIO_ReadPin` in the STM32F4 HAL GPIO driver.

The underlying GPIO check involves:

```c
GPIOx->IDR & GPIO_Pin
```

For PC13, `GPIO_Pin` is `0x2000`. In this MCU, `GPIOC` begins at `0x40020800`, and `IDR` is at offset `0x10` (`0x40020810`).

**Questions:** What is the difference between reading `GPIOC->IDR` once and detecting a falling edge? What CPU work is repeated even while nobody touches the button?

## 5. Experiment B — Configure an external interrupt

**Keep the polling loop temporarily.** This allows you to catch an interrupt that pre-empts ordinary GPIO polling code.

1. Stop debugging and close the `.ioc` editor in CubeIDE.
2. Open `firmware/lab03_interrupts/lab03_interrupts.ioc` in **standalone STM32CubeMX**.
3. In the board/BSP configuration, change the **USER BUTTON** mode from **GPIO** to **EXTI**.
4. Generate the project code for STM32CubeIDE. Keep the existing project location and preserve user code.
5. Return to CubeIDE, refresh the project if necessary, and inspect the generated changes.

Verify that `main.c` now initializes the button with:

```c
BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
```

Verify that `Core/Src/stm32f4xx_it.c` contains a generated handler equivalent to:

```c
void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(USER_BUTTON_PIN);
}
```

The board BSP configures a **falling-edge** interrupt for the button and enables the corresponding NVIC line.

**Expected:** Pressing USER requests the shared `EXTI15_10_IRQn` interrupt. EXTI line 13 is one of lines 10–15 handled by that vector.

Do not write a separate custom `EXTI15_10_IRQHandler` in `main.c`: the generated project already owns the actual IRQ handler. We will later implement the HAL **callback** in a preserved user-code region.

## 6. Experiment C — Enter the actual IRQ handler

1. Rebuild and flash the EXTI-enabled firmware.
2. Place a breakpoint at the beginning of `EXTI15_10_IRQHandler`, before the call to `HAL_GPIO_EXTI_IRQHandler`.
3. Resume and press USER once.
4. When halted, open **Debug**, **Registers** and **Expressions**.
5. Inspect the current exception number:

   `SCB->ICSR & 0x1FFU`

**Expected:** `56`.

The Cortex-M exception number for an external IRQ is `16 + IRQ number`. Here `EXTI15_10_IRQn` is IRQ **40**, giving **56**. This proves that an exception is active.

### Understand vector-table binding

Open:

`Core/Startup/startup_stm32f446retx.s`

Find the vector entry and weak definition for `EXTI15_10_IRQHandler`:

```asm
.word EXTI15_10_IRQHandler
.weak EXTI15_10_IRQHandler
.thumb_set EXTI15_10_IRQHandler,Default_Handler
```

The `.word` directive puts a handler address in the vector table; it is not a CPU instruction executed on every interrupt. The linker resolves the strong generated C handler instead of the weak default.

### Current pin level versus pending interrupt

While halted **before** HAL acknowledges the EXTI request, examine:

```c
GPIOC->IDR & GPIO_PIN_13
EXTI->PR & GPIO_PIN_13
```

Release the physical button if necessary.

It is entirely possible to observe **PC13 HIGH** (the button is now released) while **EXTI pending = 1** (the earlier falling edge has not yet been acknowledged).

Step into `HAL_GPIO_EXTI_IRQHandler`. Find `__HAL_GPIO_EXTI_CLEAR_IT` and watch the pending bit clear. On this MCU, writing a 1 to the relevant `EXTI->PR` bit acknowledges it (**W1C**).

**Expected:** The EXTI pending indication is cleared; the GPIO input register remains a reading of the present physical level.

The EXTI pending bit is not a lossless history of every button edge. Mechanical bounce is possible and is **not characterized** in this lab.

## 7. Experiment D — Inspect exception entry and return

This is the central debugger experiment. Continue using the temporarily combined **polling + EXTI** firmware from Sections 4–6.

### Explore the interrupted call stack

While stopped in or inside the HAL IRQ handler:

1. Expand the suspended thread in the CubeIDE **Debug** view.
2. Inspect the call stack. It may contain:

   ```text
   HAL_GPIO_EXTI_IRQHandler
   EXTI15_10_IRQHandler
   <signal handler called>
   HAL_GPIO_ReadPin
   BSP_PB_GetState
   main
   ```

3. Your exact interrupted function can vary: an interrupt may arrive at **any instruction boundary**, including during `main()` or another enabled exception. Do not assume a particular source line.
4. Open **Disassembly**. Use the view's **Go to Address** command or **Ctrl+G** to navigate; alternatively use GDB's `x/20i <address>`.

The synthetic `<signal handler called>` entry marks the exception boundary in the debugger. It is not a real C function call.

### Inspect the saved exception frame

A normal Cortex-M4 basic exception frame contains eight automatically stacked words, in this order:

| Offset from frame start | Saved register |
|---|---|
| +0x00 | R0 |
| +0x04 | R1 |
| +0x08 | R2 |
| +0x0C | R3 |
| +0x10 | R12 |
| +0x14 | LR |
| +0x18 | PC |
| +0x1C | xPSR |

Open the **Console** view if it is hidden (for example, via **Window → Show View → Console**). Use the console-selection dropdown to choose the **Debugger Console / GDB console**, rather than the build/output console; the exact label can vary between CubeIDE versions.

In that Debugger Console, try:

```gdb
info registers
info frame
```

Select the interrupted frame in the Debug view and use `info frame` to locate its saved-register information. Once you have identified the actual frame start address, read its eight words:

```gdb
x/8wx 0xYOUR_FRAME_ADDRESS
```

Replace the placeholder with the **actual RAM address in your own session**; the text above is not itself a valid GDB address.

In one reference run, the exception frame began at `0x2001FFB0` and contained:

| Register | Reference value |
|---|---|
| R0 | `0x40020800` |
| R1 | `0x00002000` |
| R2 | `0x00000700` |
| R3 | `0x40020800` |
| R12 | `0x00007A00` |
| LR | `0x080009E3` |
| PC | `0x08001030` |
| xPSR | `0x41000000` |

**These values are examples, not required results.** Even the interrupted function and frame location may differ between runs.

In that reference run the saved `PC` was `0x08001030`, inside `HAL_GPIO_ReadPin`. The GPIO register read had already loaded a value into `R2`, but the bit mask had not yet executed. `R2 = 0x00000700` meant PC13 was LOW *at the time of the read*. When execution resumed, the program used this saved result even if the button had since been released.

### Observe EXC_RETURN

Near exception-handler entry, inspect LR. You may find a special value such as:

`0xFFFFFFF9`

This is an **EXC_RETURN** encoding: it directs the processor to return to Thread mode using MSP and restore the basic exception frame. It is not an ordinary code address. Once ordinary functions are called within the handler, LR may instead contain normal return addresses, so inspect the appropriate frame.

Use **Step Return (F7)** to return through the HAL code and generated IRQ handler. The final return crosses the hardware exception boundary and resumes the interrupted instruction stream.

**Completion check:** Explain why `PC` and the general registers resume with their saved values even though the physical GPIO input may have changed while the handler ran.

## 8. Experiment E — Record events in the ISR; handle them in the main loop

Now remove physical GPIO polling from the superloop. Keep the generated EXTI configuration and IRQ handler.

In `Core/Src/main.c`, in `USER CODE BEGIN PV`, use:

```c
volatile uint32_t background_counter = 0U;
volatile uint32_t button_interrupt_count = 0U;
static uint32_t last_handled_interrupt_count = 0U;
```

In `USER CODE BEGIN 4`, provide the strong HAL callback:

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13)
    {
        button_interrupt_count++;
    }
}
```

Replace the application code inside the `while (1)` loop's `USER CODE BEGIN 3` region with:

```c
uint32_t snapshot = button_interrupt_count;

if (snapshot != last_handled_interrupt_count)
{
    last_handled_interrupt_count = snapshot;
    BSP_LED_Toggle(LED2);
}

background_counter++;
```

Do not remove the surrounding CubeMX-generated structure or its `USER CODE` markers.

Build and flash, then remove the IRQ breakpoint and run normally.

**Expected:** The LED starts off, toggles after an accepted falling-edge event, and toggles again on the next accepted event. The callback increments `button_interrupt_count` and the superloop performs the actual LED operation.

You can watch `button_interrupt_count` and `background_counter` in Expressions while suspended. Record the effect of several deliberate, well-separated presses. In the original verified runs, five presses produced five callback increments; mechanical contact bounce and exact event counts are **not guaranteed** for every board or attempt.

### Design interpretation

- EXTI/NVIC detect and deliver the event.
- HAL's IRQ helper clears the pending flag and calls the callback.
- The callback records minimal application state.
- The main loop decides what to do with that information.

The main loop still polls a **RAM counter**. It no longer polls the GPIO peripheral. This is a deliberately small example of deferred processing.

## 9. Experiment F — Inject an event-coalescing fault

Start a fresh debug session, with the CPU stopped at the start of the main loop **after LED initialization but before the first event check**. Verify the event counters are zero.

In the **Debugger Console**, enter:

```gdb
p button_interrupt_count
set variable button_interrupt_count = 2
p button_interrupt_count
```

Confirm `2` in Expressions or Memory. Remove the loop breakpoint and resume.

**Observed/expected:** With the `if` implementation, two recorded events lead to **one LED toggle**. The main loop sees that at least one new event exists and advances `last_handled_interrupt_count` to the snapshot.

Now test a different requirement: **exactly one LED toggle for every recorded event**. Replace the event check temporarily with:

```c
uint32_t snapshot = button_interrupt_count;

while (last_handled_interrupt_count != snapshot)
{
    BSP_LED_Toggle(LED2);
    last_handled_interrupt_count++;
}

background_counter++;
```

Rebuild, restart from a known initial state and repeat the exact injection.

**Expected:** Two toggles occur, leaving the LED in its original state. Because they happen very quickly, verify the final LED state and handled counter rather than relying on visibly detecting both transitions.

**Key question:** Was the original `if` implementation a bug? Only if the application requirement demanded one action per event. If one action for *one or more pending events* is desired, the `if` behavior is intentional **event coalescing**.

Neither version implements time-based button debounce.

Restore the `if` version afterward: it is the behavior selected for this reference application.

## 10. Optional Experiment G — Measure loop throughput

This experiment is optional and changes the build configuration. Preserve a known-good EXTI version before starting.

### Instrument the existing SysTick timebase

**If you are working from the completed `main` branch, the final firmware already contains this SysTick instrumentation. Inspect the existing code and do not add duplicate definitions.** If you started from the polling baseline and built up the application yourself, add the following.

In `main.c`, add to `USER CODE BEGIN PV`:

```c
volatile uint32_t loops_per_second = 0U;
```

Keep the existing `background_counter++` at the end of *each* loop iteration.

In `Core/Src/stm32f4xx_it.c`, add inside `USER CODE BEGIN 0`:

```c
extern volatile uint32_t background_counter;
extern volatile uint32_t loops_per_second;
```

Inside `SysTick_Handler()`, **after** the existing `HAL_IncTick()` call, in `USER CODE BEGIN SysTick_IRQn 1`, add:

```c
static uint32_t elapsed_ticks = 0U;
static uint32_t previous_count = 0U;

elapsed_ticks++;

if (elapsed_ticks >= 1000U)
{
    uint32_t current_count = background_counter;
    loops_per_second = current_count - previous_count;
    previous_count = current_count;
    elapsed_ticks = 0U;
}
```

The default HAL SysTick configuration in this project provides nominal 1 ms ticks, so 1,000 ticks is approximately one second. The unsigned subtraction is valid across a 32-bit counter wraparound.

### Measure both versions

1. Change the **Debug C compiler optimization level** to `-O2` in CubeIDE's project build settings. Confirm the actual compiler flag in the build output; the checked-in debug default is `-O0`.
2. Build/flash the EXTI version.
3. With no button presses and no breakpoints during sampling, run for several seconds, suspend, and read `loops_per_second`. Repeat three times.
4. Record the result.
5. Change USER BUTTON back to **Mode GPIO** in standalone CubeMX and regenerate. Verify `BUTTON_MODE_GPIO` in `main.c`.
6. Replace only the superloop body with the polling code from Section 4, preserving `background_counter++` and the same SysTick instrumentation.
7. Build/flash at **the same `-O2` level**, take three measurements under the same no-button conditions, and compare.

The reference experiment yielded:

| Version | Sample 1 | Sample 2 | Sample 3 |
|---|---:|---:|---:|
| EXTI, main loop checks RAM event count | 6,457,499 | 6,457,499 | 6,457,499 |
| Polling GPIO, LED On/Off each iteration | 1,499,100 | 1,499,100 | 1,499,100 |

That is approximately **4.31 times as many main-loop iterations** in the interrupt-based implementation under the reference test conditions. Exact results may differ.

**Interpretation:** This is a comparison of **two application loops**, not proof that interrupt handling is intrinsically 4.31× faster than polling. The polling loop repeatedly calls BSP/HAL GPIO and LED functions; the idle EXTI loop mostly reads and compares RAM counters. This does not measure interrupt latency, CPU utilization, worst-case execution time or energy consumption. Measurement instrumentation also imposes some overhead.

To finish, restore the known-good **EXTI** source and `.ioc` configuration, return the Debug build to **`-O0`**, rebuild and verify the button/LED behavior.

## 11. Troubleshooting

| Symptom | What to check |
|---|---|
| CubeIDE cannot import the project | Select `firmware/lab03_interrupts`, the folder containing `.project`. Keep "Copy projects into workspace" unchecked. |
| Build fails after CubeMX regeneration | Refresh the CubeIDE project, check the generated BSP/HAL files, and inspect errors rather than replacing the project. |
| Debugger cannot connect | Use the onboard ST-LINK USB port, check the USB cable/driver, and confirm the ST-LINK debug configuration targets the correct MCU. |
| Button value never changes | Confirm the USER button is PC13, that BSP initialization ran, and inspect `(GPIOC->IDR >> 13) & 1U`. |
| No breakpoint hit in `EXTI15_10_IRQHandler` | Check `BUTTON_MODE_EXTI`, the falling-edge BSP configuration, NVIC enablement, and whether the generated IRQ handler is linked. |
| Handler runs but LED does not change | Check the strong `HAL_GPIO_EXTI_Callback` definition, `GPIO_Pin == GPIO_PIN_13`, and the main-loop snapshot comparison. |
| Unexpected number of button events | Consider mechanical bounce and EXTI pending-bit coalescing; neither is controlled by this exercise. |
| `x/8wx` or `set variable` is rejected | Switch to **Debugger Console**, not the ordinary Console. |
| Saved exception PC differs from this README | Expected. The edge can interrupt a different instruction on every run. |
| Breakpoints behave strangely at `-O2` | Rebuild with `-O0` for instruction-by-instruction investigation. |
| CubeMX removes an edit | Put application edits inside `USER CODE` regions and use CubeMX to manage generated initialization. |

## 12. Completion checklist

Before considering the lab complete, verify that you can:

- [ ] Read PC13's bit in `GPIOC->IDR` and distinguish it from the button-edge event.
- [ ] Make the LED react using **GPIO polling**.
- [ ] Configure **falling-edge EXTI** and stop inside `EXTI15_10_IRQHandler`.
- [ ] Explain why IRQ 40 is **exception number 56**.
- [ ] Observe a pending EXTI event and its W1C acknowledgement.
- [ ] Identify the weak vector alias and strong linked IRQ handler.
- [ ] Inspect a real exception stack frame and find its saved `PC`.
- [ ] Distinguish Thread mode, Handler mode, ordinary function return and EXC_RETURN.
- [ ] Record events in `HAL_GPIO_EXTI_Callback` and process them in the main loop.
- [ ] Inject a two-event condition and explain the event-counting versus coalescing behavior.
- [ ] *(Optional)* Measure and correctly interpret the polling/EXTI loop-throughput comparison.
- [ ] End with the **working EXTI firmware** and an easy-to-debug `-O0` configuration.

## 13. Further reading and reference files

- [Personal Lab 3 notes](notes.md) — deeper explanations and observations from the original run.
- [Original polling baseline commit](https://github.com/dernevik/embedded-hands-on/commit/fd96486) — convenient starting point for the full exercise.
- [Working EXTI milestone commit](https://github.com/dernevik/embedded-hands-on/commit/b73548c) — reference interrupt implementation with SysTick measurement.
- `Core/Startup/startup_stm32f446retx.s` — vector table and weak handler aliases.
- `Core/Src/stm32f4xx_it.c` — generated exception/IRQ handlers and SysTick.
- `Core/Src/main.c` — application loop and EXTI callback.
- `Drivers/BSP/STM32F4xx-Nucleo/stm32f4xx_nucleo.c` — board-level GPIO initialization.
- STM32F446 reference manual and Cortex-M4 architectural documentation — EXTI/GPIO registers, NVIC, exception entry and EXC_RETURN.

**Suggested next topic:** SPI polling between two STM32 boards, followed later by SPI interrupts/DMA. Those are separate labs, not prerequisites for this one.
