# Lab 03 — Interrupts: Personal Notes

## What I learned

This lab connected several things I had previously understood mostly at a software architecture level — interrupts, event handling, callbacks, execution contexts, and scheduling — to what actually happens inside a Cortex-M4 processor.

A valuable part was observing a real exception interrupting an ordinary CPU instruction sequence, examining the saved processor state in RAM, and watching execution resume where it had been interrupted.

I also gained a better understanding of the trade-offs between polling and interrupt-driven software, including an actual performance comparison.

## 1. GPIO polling versus external interrupts

The NUCLEO-F446RE USER button is connected to **PC13**, and the onboard LED LD2 to **PA5**.

On our board:

- Button released: PC13 is HIGH (`1`).
- Button pressed: PC13 is LOW (`0`).
- Pressing the button therefore produces a falling edge.

In polling mode, the superloop repeatedly reads the current GPIO state through:

`BSP_PB_GetState()` → `HAL_GPIO_ReadPin()` → `GPIOC->IDR`

The actual HAL read involves masking the GPIO input data register with `GPIO_PIN_13`, which equals `0x2000`.

`GPIOC` has base address `0x40020800`, and its `IDR` is at offset `0x10`.

A useful diagnostic expression is:

`(GPIOC->IDR >> 13) & 1U`

In EXTI mode, a falling edge on PC13 can cause an interrupt without the superloop repeatedly reading the physical pin.

- `GPIOC->IDR` tells me the *current* electrical input state.
- `EXTI->PR` tells me whether an external interrupt event is pending.

I observed the GPIO level returning HIGH after releasing the button while the EXTI pending bit was still set. The HAL interrupt handler then cleared the pending bit.

On this MCU, the EXTI pending register uses **write-one-to-clear (W1C)** semantics.

A pending flag is not a queue of every physical edge. Multiple events may be represented by a single pending bit.

## 2. EXTI, NVIC, vector tables, and weak handlers

The interrupt path for the USER button is:

1. A falling edge occurs on PC13.
2. EXTI line 13 detects the configured edge.
3. The interrupt request is handled through the NVIC.
4. The CPU enters the associated exception handler.
5. The generated handler calls the HAL EXTI handler.
6. The HAL code acknowledges the pending event and calls the application callback.

EXTI line 13 shares the IRQ with EXTI lines 10–15:

`EXTI15_10_IRQn = 40`

For an external IRQ, the Cortex-M exception number is:

`16 + IRQ number`

Therefore, our active exception number was **56**.

I verified this in the debugger by evaluating:

`SCB->ICSR & 0x1FFU`

The result was `56`.

This was direct evidence that the processor was executing in an exception, rather than merely calling a function named `IRQHandler`.

The startup assembly contains a vector-table entry for `EXTI15_10_IRQHandler`, along with a weak alias to `Default_Handler`.

A few things I want to remember:

- `.word EXTI15_10_IRQHandler` places a handler address in the vector table; it is not an executed instruction.
- The linker resolves the symbol to the actual handler implementation.
- A strong handler definition overrides the weak default.
- The CPU enters the interrupt handler through the exception mechanism, not through an ordinary C function call.
- Calls *within* the interrupt handler, such as calls into HAL, are ordinary function calls.

There are two separate weak-override mechanisms in our code:

- The generated strong `EXTI15_10_IRQHandler` replaces the weak startup default.
- Our own strong `HAL_GPIO_EXTI_Callback` replaces HAL's weak, empty callback.

The callback mechanism feels familiar from AUTOSAR, but here I could follow it all the way down to the CPU exception machinery.

## 3. What happens when an interrupt arrives

This was an important discovery in Lab 3.

I placed a breakpoint in the EXTI handler while the processor was executing polling code.

The debugger showed the interrupt had occurred **inside `HAL_GPIO_ReadPin()`**, between two machine instructions.

The relevant sequence was:

```asm
ldr  r2, [r3, #16]    ; Read GPIO input register
ldrh r3, [r7, #2]     ; Interrupted PC
ands r3, r2          ; Mask the input value
```

The CPU had already completed the GPIO register read, but had not yet performed the masking operation.

At the interrupt, the value of `R2` was `0x00000700`, where bit 13 was LOW.

Later, while the processor was stopped in the handler, PC13 had returned HIGH because the button had been released. The earlier register read has already happened. Its result is now part of the interrupted computation.

When the CPU resumes, it continues using that earlier value rather than re-reading the GPIO register.

### The automatic exception stack frame

On exception entry, Cortex-M automatically saves a basic register frame containing:

`R0, R1, R2, R3, R12, LR, PC, xPSR`

I inspected the actual memory with:

`x/8wx 0x2001ffb0`

For this particular debug session, the saved frame was:

| Register | Observed value | Meaning |
|---|---|---|
| R0 | `0x40020800` | GPIOC base address |
| R1 | `0x00002000` | PC13 bit mask |
| R2 | `0x00000700` | GPIO input value read before interruption |
| R3 | `0x40020800` | Working register |
| R12 | `0x00007A00` | Saved working register |
| LR | `0x080009E3` | Interrupted function's return address |
| PC | `0x08001030` | Instruction at which execution resumes |
| xPSR | `0x41000000` | Saved processor status, including Thread-mode IPSR |

The  the exception frame is real data in RAM.

We also observed:

`LR = 0xFFFFFFF9`

This is an **EXC_RETURN** value. It tells the CPU to perform an exception return to Thread mode using MSP, with the basic frame.

This is different from a normal LR value produced by `BL`.

I then used Step Return through the callback, HAL handler, and IRQ handler, eventually returning to the interrupted `HAL_GPIO_ReadPin()`.

That connected the exception mechanism, saved registers, stack memory, program counter and resumed execution.

## 4. Handler mode versus Thread mode

Ordinary application code runs in **Thread mode**.

Interrupt handlers execute in **Handler mode**.

While inspecting the EXTI handler, the active exception number in `SCB->ICSR` was 56. The saved xPSR showed that the interrupted context had been Thread mode.

A HAL callback called from the IRQ handler is still executing in Handler mode, even though the callback itself looks like an ordinary C function.

This matters for design: calling a normal-looking function does not automatically change its execution context.

The interrupt handler can invoke other C functions, and their regular function calls modify LR in the usual way. Eventually the outermost handler returns using the preserved EXC_RETURN mechanism.

We discussed interrupt priorities and nesting, but did not experimentally investigate nested interrupts or worst-case interrupt latency.

## 5. Keep event detection separate from application work

Our EXTI callback was deliberately short:

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_13)
    {
        button_interrupt_count++;
    }
}
```

The superloop checked the shared counter and toggled the LED when a new event had been recorded.

This gave us two distinct responsibilities:

- **ISR:** Record that the hardware event occurred.
- **Thread-mode application code:** Decide what to do about the event.

I recognized that the superloop still performs a kind of polling — but now it polls a RAM variable instead of repeatedly reading the GPIO peripheral.

This is a simple form of deferred processing, not an RTOS notification mechanism.

General design rules worth remembering:

- Use an interrupt when hardware needs prompt attention or a specific event must be captured.
- Perform time-critical acknowledgement and necessary data capture in the interrupt context.
- Keep lengthy or potentially blocking work in normal scheduled execution contexts.
- Choose ISR/task boundaries based on latency requirements, worst-case execution time, priorities, and system architecture.
- In a larger AUTOSAR ECU, these boundaries and priorities should be coordinated at ECU level rather than selected independently by each application team.

## 6. Event counting versus event coalescing

We deliberately injected an artificial counter jump using GDB:

`set variable button_interrupt_count = 2`

The original main-loop logic was:

```c
uint32_t snapshot = button_interrupt_count;

if (snapshot != last_handled_interrupt_count)
{
    last_handled_interrupt_count = snapshot;
    BSP_LED_Toggle(LED2);
}
```

If two events arrive before the main loop reacts, the LED toggles only once.

We demonstrated this using the injected value.

We then tested a version that processed every event individually:

```c
while (last_handled_interrupt_count != snapshot)
{
    BSP_LED_Toggle(LED2);
    last_handled_interrupt_count++;
}
```

This produced two toggles, with the LED returning to its original state. Neither implementation is automatically correct.

If the requirement is *one toggle for each recorded event*, the original `if` version loses the required event multiplicity.

If the requirement is *react once when one or more new events have occurred*, the original implementation is intentional **event coalescing**.

I chose the coalescing semantics in our example, because executing a burst of physical LED toggles has no useful purpose for our simple application.

Neither implementation is a proper time-based button debouncer.

## 7. `volatile` and compiler optimization

The shared interrupt counter was declared:

`volatile uint32_t button_interrupt_count`

The ISR updates it and the superloop reads it.

I already knew the general concurrency implications of `volatile`, but this lab made it interesting to examine the generated assembly.

At `-O0`, GCC used several instructions to load a global variable's address through a literal pool and then read the variable. Local variables were also more readily visible as stack slots.

At `-O2`, GCC kept addresses of global variables in registers and produced a faster superloop.

One particularly interesting result: temporarily removing `volatile` from our interrupt counter produced **identical optimized loop assembly** in this particular build.

That did not establish that `volatile` was unnecessary. It demonstrated that a source-level qualifier does not necessarily produce an observable assembly difference in every build.

The practical lesson for me is to distinguish the language-level contract from one observed compiler output. Generated assembly is excellent evidence of what a specific build does, but it does not define the guarantees of the source program.

## 8. AAPCS and reading function calls in assembly

I learned a useful piece of the Arm Procedure Call Standard:

- `R0`–`R3` carry the first four ordinary integer/pointer arguments.
- `R0` is commonly used for a simple function return value.
- `BL` performs an ordinary function call.
- `BX LR` can return to the caller.

For example:

```asm
movs r0, #0
bl   BSP_PB_GetState
```

Here the caller places `BUTTON_USER = 0` in `R0`.

Inside BSP, the arguments for `HAL_GPIO_ReadPin()` are then prepared:

- `R0`: pointer to GPIOC.
- `R1`: `GPIO_PIN_13`, or `0x2000`.

The optimized `HAL_GPIO_ReadPin()` was only six instructions long:

```asm
ldr   r0, [r0, #16]
ands  r0, r1
subs  r0, #0
it    ne
movne r0, #1
bx    lr
```

But the whole polling path involves additional BSP and LED function calls.

## 9. Measuring polling versus interrupt performance

I initially predicted that the interrupt-based superloop might be approximately 10 times faster than polling, and later revised my prediction to 30 times.

We tested this using a counter incremented on every superloop iteration.

Instead of checking the time in the loop itself, we sampled the counter from the existing SysTick interrupt.

The measurement code ran every nominal 1,000 SysTick ticks, using the difference between the current and previous loop counters.

This was an example of **software instrumentation**.

I learned why the measurement was structured this way:

- SysTick already supplied an approximately 1 ms timebase.
- Reading the time in every iteration would have increased the measured loop's workload.
- Sampling from SysTick kept most measurement logic outside the hot loop.
- Both implementations were measured with the same `-O2` optimization level and no button presses.

The observed results were:

| Implementation | Iterations per second |
|---|---:|
| GPIO polling | 1,499,100 |
| EXTI / interrupt-driven | 6,457,499 |
| Ratio | **4.31×** |

We obtained the same number in three samples for each implementation.

My predictions were wrong.

**What we measured:** The interrupt-based application superloop executed approximately 4.31 times as many iterations per second under our test conditions.

**What we did not measure:** Interrupt latency, absolute CPU utilization, power consumption, worst-case response time, or an intrinsic 4.31× speed advantage of interrupts over polling.

In particular, the polling implementation performed BSP/HAL GPIO reading and LED On/Off operations every iteration, even when the physical button state had not changed.

The interrupt version had a substantially shorter idle loop.

I also learned that instrumentation affects the system being measured. For more precise cycle measurements, Cortex-M's DWT cycle counter would be worth investigating.

## 10. SysTick and other hardware timers

In our STM32 HAL configuration, SysTick generates an exception approximately every millisecond. Its handler also calls `HAL_IncTick()` to maintain the HAL timebase.

This occurs even when our application is not explicitly waiting for a millisecond event.

SysTick is only one possible interrupt source. The MCU does not continuously run handlers for all its peripherals; interrupt sources must be configured and enabled.

We discussed two ways of scheduling future work:

- **Software counting from SysTick:** Simple and useful when an existing system tick offers sufficient resolution.
- **Dedicated hardware timers:** Can count independently and generate interrupts at configured periods or compare events.

STM32F446 has timer peripherals such as TIM2–TIM5. These are conceptually related to timer facilities in e.g. AURIX, although the implementation differs.

We did not configure a dedicated hardware timer in this lab.

## 11. Debugger techniques worth remembering

Useful STM32CubeIDE/GDB techniques from this lab:

| Task | Technique |
|---|---|
| Inspect the active exception | `SCB->ICSR & 0x1FFU` |
| Examine register values | `info registers` |
| Inspect the current stack frame | `info frame` |
| Read eight 32-bit words in RAM | `x/8wx <address>` |
| Disassemble from an address | `x/20i <address>` |
| Disassemble a function | `disassemble HAL_GPIO_ReadPin` |
| Change a variable for fault injection | `set variable name = value` |
| Navigate to an address in Disassembly | **Ctrl+G** or **Go to Address** |
| Follow returns from nested handlers | **Step Return / F7** |

GDB commands must be entered in the **Debugger Console**, not the ordinary build/output Console.

The **Debug** view shows the suspended call stack. Double-clicking frames navigates to their execution context; it does not itself change where the CPU is executing.

The disassembly view is especially valuable when a source-level debugger highlight is not precise enough to explain an interrupt boundary.

## 12. Project workflow and reproducibility

For this lab I kept the complete STM32CubeIDE project inside the Git repository:

`lab03-interrupts/firmware/lab03_interrupts/`

The `.ioc`, source code, HAL/BSP dependencies, startup code, linker scripts, and relevant IDE project configuration belong together.

Generated `Debug/` and `Release/` outputs should not be committed.

My workflow uses standalone STM32CubeMX to regenerate the project and STM32CubeIDE for building and debugging. Custom code must be placed in preserved `USER CODE` sections to survive regeneration.

Useful Git milestones included:

- `fd96486` — known-good GPIO polling baseline.
- `b73548c` — working EXTI implementation with SysTick performance instrumentation.

After the performance experiment, I restored the EXTI version, returned to `-O0` for easier debugging, verified correct behavior, and pushed the final changes.

The final debug build is not the same configuration as the `-O2` performance measurements.

## Final takeaways

If I return to these notes several months from now, these are the points I most want to remember:

1. **An interrupt is a hardware/CPU exception mechanism, not merely a callback.** The callback is ordinary software executing after exception entry.
2. **The interrupted CPU context is physically saved in RAM.** The saved PC, registers, and xPSR explain exactly where and how execution resumes.
3. **Separate event capture from application behavior.** ISR versus scheduled work is an architectural decision based on timing and responsibility.
4. **Inspect actual machine code when making claims about execution cost.** C code alone can conceal substantial call-chain overhead.
5. **Measure rather than guess.** Our 10× and 30× hypotheses became an observed 4.31× — for the particular code and conditions tested.
6. **Use the simplest instrumentation capable of answering the question**, while remembering that the instrumentation itself changes the program.

**Overall:** Lab 3 gave me a much more concrete mental model of how a physical event becomes a CPU exception, how execution context is preserved, and how that mechanism should influence embedded software architecture.