# Lab 01 — Debugger, CPU & Assembly

**Board:** STM32 NUCLEO-F446RE  
**MCU:** STM32F446RETx, ARM Cortex-M4  
**Development environment:** STM32CubeIDE 2.2.0, Windows 11  
**Debug interface:** Onboard ST-LINK, SWD  
**Status:** Completed — Levels A, B and C achieved

## 1. Purpose

The goal of this lab was to develop practical intuition about what happens inside a microcontroller when it executes embedded C code.

In particular, I wanted to understand the relationship between:

- C source code and generated assembly instructions.
- CPU registers and physical memory.
- The program counter (PC), link register (LR) and stack pointer (SP).
- Function calls, return addresses and stack frames.
- Debugger commands and actual CPU execution.

## 2. Build, flash and debug

### Build

Building takes place on the development computer.

The compiler translates C source files into object files, and the linker combines these with the required runtime code and libraries into an executable ELF file.

The linker script influences where code and data are placed in the target address space.

The ELF file may contain both loadable program contents and information intended for development tools, such as symbols and DWARF debugging information.

### Flash

Flashing transfers the relevant firmware contents to the target microcontroller's nonvolatile flash memory.

For this lab, STM32CubeIDE used the onboard ST-LINK to program the STM32F446RE over SWD.

Not everything described by the ELF file is physically written to flash. Startup code is responsible for preparing relevant RAM regions before calling `main()`.

A debug probe is not the only way to program a microcontroller. Many MCUs have factory-programmed ROM bootloaders, and products may use custom bootloaders for firmware updates.

### Debug

Debugging means observing and controlling the execution of the target processor.

For this setup, the communication path is approximately:

**STM32CubeIDE → ARM GDB → ST-LINK GDB Server → USB → onboard ST-LINK → SWD → Cortex-M4**

The responsibilities:

- **GDB** provides debugging operations and understands symbols and source-level debug information.
- **ST-LINK GDB Server** connects GDB to the hardware probe.
- **ST-LINK** communicates with the microcontroller through SWD.
- **Cortex-M4** executes the firmware instructions.

The debugger can inspect registers and memory, control execution, set breakpoints and, where permitted, modify target state.

Once firmware is programmed, the MCU can execute it independently of an interactive debug session.

## 3. First experiment: a global counter

The initial program used a global volatile integer:

`volatile int counter = 0;`

Inside `main()`, an infinite loop repeatedly executed `counter++`.

The firmware built, flashed and ran successfully.

### Observations

The debugger allowed to:

- Stop execution and inspect the current CPU state.
- Resume execution and watch `counter` increase.
- Stop execution again and observe that the value remained stable.
- Locate `counter` in RAM through its address.
- Compare the value shown by Expressions with the bytes shown by the Memory view.
- Step through the assembly instructions implementing a single increment.

The `volatile` qualifier was useful in this experiment because it required the compiler to preserve observable accesses to the variable rather than simply optimizing away the counter loop.

## 4. Registers versus RAM

One useful observation was that the CPU can calculate a new value in a register before that value is stored back into RAM.

The generated assembly contained a sequence equivalent to:

| Instruction | Meaning |
|---|---|
| `ldr r3, [r3, #0]` | Read the current counter value from RAM into `r3` |
| `adds r3, #1` | Increment the value in CPU register `r3` |
| `str r3, [r2, #0]` | Store the result at the RAM address held in `r2` |

While stepping through these instructions, I observed that `r3` changed after `adds`, but the global `counter` value in memory did not change until `str` executed.

### Memory address and little-endian representation

During one execution, the address of `counter` was:

`0x2000001C`

The Memory view showed four consecutive bytes:

`80 8D C2 00`

Because Cortex-M4 uses little-endian data representation, these bytes represent the 32-bit integer:

`0x00C28D80 = 12,750,208`

The result matched the value reported by Expressions.

This was a demonstration of how a typed variable, its memory address and its underlying bytes relate to one another.

## 5. PC, LR and SP

Three CPU registers were particularly important during the lab.

### PC — Program Counter

PC determines which instruction the processor executes next.

Normally it advances through successive instructions, but branch and function-call instructions can change the execution path.

By observing PC in Disassembly, I could see exactly where execution went after branches, calls and returns.

### LR — Link Register

A `bl` (Branch with Link) instruction calls a function and records return information in LR.

On Cortex-M, the least significant bit of a function return address is associated with Thumb execution state. For example, an observed LR value of `0x08000229` corresponded to returning to the instruction at address `0x08000228`.

LR is not a history of every nested function call. If a function calls another function, it must preserve any return information it still needs.

### SP — Stack Pointer

SP points into the current stack in RAM.

In this program, the stack grows toward lower addresses. Pushing values decreases SP; popping values increases SP.

A useful observation occurred when executing `pop {r7, pc}`:

- SP before: `0x2001FFF0`
- SP after: `0x2001FFF8`
- PC after: `0x08000234`

Two 32-bit values were removed from the stack, increasing SP by 8 bytes, and execution returned to `main()`.

## 6. Nested function calls and the call stack

The program was extended to use the following calling sequence:

**main() → foo() → bar() → counter++**

With a breakpoint inside `bar()`, the debugger displayed:

| Call stack frame | Function |
|---|---|
| 0 | `bar()` |
| 1 | `foo()` |
| 2 | `main()` |

This is the debugger's reconstruction of the active function-call hierarchy.

It is related to the actual stack in RAM, but not the same.

### Function entry and return

The generated code used instructions including:

- `push {r7, lr}` — save registers, including return information.
- `bl bar` — call `bar()` and update LR.
- `bx lr` — return by branching to the address held in LR.
- `pop {r7, pc}` — restore a saved register and return by loading the saved address directly into PC.

The return from `foo()` was interesting.

Instead of executing `pop {r7, lr}` followed by `bx lr`, the compiler generated `pop {r7, pc}`. This restored the saved return address directly into PC and performed the return in one instruction.

I verified the resulting PC and SP changes using the debugger.

### Stack frames and local variables

A stack frame represents execution state associated with a particular invocation of a function.

Depending on the generated code, it may contain saved registers, return addresses, local variables and temporary values.

Not every local variable must be stored on the stack.

A compiler may keep values in registers, represent them as constants or optimize them away.

The exact memory layout and calling convention details are subjects for a later lab.

## 7. Source-level stepping versus instruction-level stepping

STM32CubeIDE provides several execution-control operations.

| Operation | Behaviour |
|---|---|
| Step Into (F5) | Enter a function or execute the next instruction, depending on stepping mode |
| Step Over (F6) | Execute a call without stopping at its internal instructions |
| Step Return (F7) | Continue until the current function returns |
| Resume (F8) | Continue execution until a breakpoint or another stop condition |
| Instruction Stepping Mode | Make stepping operate on individual assembly instructions |

### Unexpected source-level stepping behavior

Source-level stepping was not always intuitive in the infinite loop.

In one experiment, repeated F5 commands appeared to move from the end of `foo()` into the next invocation of `foo()` without visibly stopping in `main()`.

However, breakpoints and instruction-level stepping established that execution really did pass through `main()`.

The likely explanation ris how GDB uses source-line debugging information to select stopping locations. The exact GDB behavior was not investigated or understood.

**Practical lesson:** When source-level stepping seems confusing, inspect PC and Disassembly. The source-code highlight is an abstraction over the actual instruction stream.

## 8. Disassembly reveals information not visible in C

The Disassembly view showed details that are mostly hidden at the C level:

- Register loads and stores.
- Arithmetic instructions.
- Function prologues and epilogues.
- Explicit function calls and returns.
- Conditional and unconditional branches.
- PC-relative addressing.

I also encountered a **literal pool**, where constant data embedded near the program instructions was displayed as though it were executable instructions.

This is a reminder that interpreting machine-code bytes requires knowing whether an address contains code or data.

The program was built without optimization for easier debugging. Different optimization settings could produce significantly different assembly from the same C source.

## 9. Level C: debugging an unexpected counter value

### Requirement

Every completed call to `foo()` should increase `counter` by exactly one.

### Deliberately defective implementation
The challenge introduced this behavior:

1. Call `bar()`.
2. If `counter` is equal to 3, call `bar()` again.

Since `bar()` increments `counter`, the requirement is violated under a specific condition.

### Reproducing the defect

I placed a breakpoint at the `foo()` call in `main()` and recorded the counter after each completed invocation.

This established that the first deviation occurred during the third call.

### Finding the root cause

I restarted the debug session to restore a known initial state and used instruction stepping to reach the failing invocation.

After the first call to `bar()`, the counter reached 3.

The critical instructions were:

| Instruction | Effect |
|---|---|
| `cmp r3, #3` | Compare the counter value with 3 |
| `bne.n 0x08000234` | Branch if the values are not equal |
| `bl 0x08000204 <bar>` | Call `bar()` again if the branch was not taken |

When `counter` equals 3, the comparison sets the zero flag. Therefore, the **BNE branch is not taken**.

Execution falls through to the second `bl bar`, producing another increment within the same invocation of `foo()`.

This explains why the third invocation changes the counter from 2 to 4 rather than from 2 to 3.

### Debugging methodology

The important lesson was not the complexity of the defect, but the method used to establish it:

**Requirement → observed deviation → reproducible failure → targeted inspection → instruction-level evidence → root cause**

I also experimented with GDB breakpoint commands to count visits to `bar()`. The breakpoint was accepted, but the command-script approach was not completed successfully. It remains an optional area for exploration.

## 10. Other practical observations

### Debugger state and UI

STM32CubeIDE has several separate views and perspectives, including Debug, Expressions, Registers, Memory, Breakpoints and Disassembly.

Some observations were initially confusing because of source-level stepping behavior or temporary UI refresh problems.

The most reliable approach was to check the execution state and compare multiple independent observations rather than relying solely on one highlighted source line.

### Build artifacts

The build directory contained several outputs beyond the executable ELF file, including object files, dependency information, memory usage reports, linker maps and listings.

The map file and linker script will become more important later.

### Compiler and C-language details

The lab also reinforced several coding details:

- `int main(void)` is a suitable function signature.
- `void` in an empty parameter list explicitly indicates that a function takes no arguments.
- `static` at file scope gives functions internal linkage.
- `volatile` affects how memory accesses are generated but is not a general concurrency mechanism.
- Incrementing a signed `int` beyond its representable range would be undefined behavior in C. The counter loop was a deliberately simple debugging exercise.

The project had one recurring FPU-related warning from the generated template, but no build errors and no floating-point operations in the experiment.

## 11. What I can now do

After completing this lab, I can:

- Build and flash a simple C program onto a real STM32 microcontroller.
- Establish a debugging session through ST-LINK and SWD.
- Stop, resume and step through running firmware.
- Set and disable breakpoints.
- Inspect expressions, registers and raw RAM contents.
- Interpret little-endian memory representations.
- Connect C statements to their generated assembly instructions.
- Follow PC changes through branches, function calls and returns.
- Explain the roles of LR and SP in nested calls.
- Distinguish the physical stack from the debugger's Call Stack view.
- Investigate an unexpected value and prove its cause using debugger evidence.

## 12. Questions and topics for later labs

The following were  not explored:

- **Memory layout:** `.text`, `.data`, `.bss`, linker scripts and startup initialization.
- **Stack and heap:** Actual stack layout, automatic storage, allocation and `_sbrk`.
- **Interrupts and exceptions:** Interrupt entry, NMI, HardFault and exception-related register behavior.
- **RTOS:** Tasks, context switching and scheduler behavior.
- **Firmware updates:** ROM bootloaders, application bootloaders and Secure Boot.
- **Advanced debugging:** GDB scripting, trace, and tools such as Lauterbach TRACE32.
- **Automotive MCU documentation:** How to systematically read datasheets, reference manuals and errata, including material for Infineon AURIX devices.

These are follow-up learning opportunities.

---

## Final reflection

The most valuable part observing the same behavior at several abstraction levels:

**C source → assembly → CPU registers → RAM → debugger observations**

When these views agreed, they reinforced the mental model. When they seemed to disagree, investigating the discrepancy became part of the learning process.

**Lab 1 completed: Explain, Demonstrate and Debug achieved.**