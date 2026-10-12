# Lab 01 — Debugger, CPU & Assembly

**From C source code to actual CPU execution on an STM32 Cortex-M4**

This is a hands-on introduction to embedded debugging using a real microcontroller, STM32CubeIDE, and an ST-LINK debug probe.

The purpose is not to build a useful application. Instead, we will deliberately use very small C programs to observe what the CPU actually does: execute instructions, modify registers, access RAM, call functions, and return from them.

By the end of this lab, you should be able to relate C source code to assembly instructions and use the debugger to investigate unexpected behavior.

## 1. Prerequisites

### Hardware

- **STM32 NUCLEO-F446RE** development board
- USB cable for the board's onboard ST-LINK connector
- Windows PC

The NUCLEO-F446RE contains an **STM32F446RE**, based on an **ARM Cortex-M4** processor.

The onboard ST-LINK acts as a debug probe. No separate JTAG/SWD probe is required.

### Software

- **STM32CubeIDE** — this lab was performed with version **2.2.0** on Windows 11
- STM32CubeIDE's bundled ARM GCC toolchain and GDB debugger
- ST-LINK drivers and firmware support, normally installed with STM32CubeIDE

Other CubeIDE versions may have slightly different menus or window layouts.

### Recommended background

You should understand basic C syntax, including functions, loops, and global variables.

No previous hands-on experience with microcontroller assembly or hardware debugging is required.

## 2. Learning objectives

After completing the lab, you should be able to:

1. Explain the difference between **building**, **flashing**, and **debugging** firmware.
2. Connect to the target MCU using ST-LINK and SWD.
3. Build, flash, run, suspend, and step through a program.
4. Inspect CPU registers, variables, and raw memory.
5. Identify how simple C statements become machine instructions.
6. Explain how PC, LR, and SP participate in program execution.
7. Follow nested function calls through the debugger's Call Stack view.
8. Diagnose a small logic defect using observations from the running target.

The emphasis is on making observations and explaining them, not memorizing processor instructions.

## 3. Connect the board

1. Connect the NUCLEO-F446RE to your PC using the **ST-LINK USB connector**.
2. Wait for Windows to recognize the board.
3. Verify that the ST-LINK is visible in Windows Device Manager or through STM32CubeIDE.
4. If CubeIDE requests an ST-LINK firmware update, follow the provided instructions before proceeding.

A new board may run a factory demonstration that blinks the green user LED (LD2).

That demonstration firmware will be replaced when you flash your own application. The LED may therefore stop blinking during this lab. This is expected.

### How the connection works

The development tools communicate with the MCU through this chain:

```text
STM32CubeIDE
    |
    v
ARM GDB / ST-LINK GDB Server
    |
    v
USB
    |
    v
Onboard ST-LINK
    |
    v
SWD
    |
    v
STM32F446RE (Cortex-M4)
```

The ST-LINK provides access to the target processor's debug interface. The processor itself executes the firmware.

## 4. Create a new STM32CubeIDE project

1. Start STM32CubeIDE.
2. Select or create a workspace.
3. Create a new **STM32 Project**.
4. In the MCU selector, search for **STM32F446RETx**.
5. Select the MCU and continue.
6. Name the project `lab01_debugger_cpu`.
7. Select **Empty Project**, if offered.
8. Finish the wizard.

The exact wizard screens depend on the IDE version.

Make sure the generated project includes the necessary STM32 startup code and linker script.

### Repository organization

The intended repository structure is:

```text
lab01-debugger-cpu/
├── firmware/      # STM32CubeIDE project / firmware source
├── images/        # Optional screenshots
├── notes.md       # Observations, results, and explanations
└── README.md      # This guide
```

You can create the CubeIDE project directly under `firmware/`, or work in a separate CubeIDE workspace and later add a reproducible copy of the project to the repository.

If you commit a CubeIDE project, include its source code, startup files, linker script, and necessary project configuration. Generated build outputs such as `Debug/` normally do not need to be committed.

## 5. Experiment A — Your first counter program

Open `Src/main.c`.

Use this minimal application:

```c
volatile int counter = 0;

int main(void)
{
    while (1)
    {
        counter++;
    }
}
```

Keep the project's generated startup code, linker script, and other supporting files.

### Why use `volatile`?

The variable is declared `volatile` so that accesses to it remain observable in the generated machine code.

Without `volatile`, an optimizing compiler might eliminate or substantially transform a counter loop whose result is never otherwise used.

`volatile` does **not** make an increment atomic or thread-safe.

### Build the application

1. Save `main.c`.
2. Select the **Debug** build configuration.
3. Build the project using **Ctrl+B** or the Build command.
4. Check the build output for errors.

The result should include an executable ELF file, typically:

`Debug/lab01_debugger_cpu.elf`

The ELF file contains firmware contents as well as information such as symbols and debugging metadata.

**Expected result:** The build completes without errors.

Depending on the project template, an FPU initialization warning may appear. The programs in this lab do not use floating-point operations.

### Build versus flash

At this point, you have built the application on your PC.

You have **not necessarily programmed the microcontroller yet**.

Building translates and links source code. Flashing transfers firmware contents to the MCU's nonvolatile memory.

## 6. Start the debugger and flash the MCU

1. Connect the board if it is not already connected.
2. Select **Debug** for the CubeIDE project.
3. If necessary, create a debug configuration using:
   - The project's Debug ELF executable
   - **ST-LINK (ST-LINK GDB server)**
   - **SWD** as the debug interface
4. Start the debug session.
5. Allow the IDE to program the target.
6. Use **Run to main**, if available.

The IDE should switch to a Debug perspective.

**Expected result:** Execution stops in or near `main()`.

You should now see the current execution state in the Debug view.

### Important distinction

The debugger is running on your PC. The firmware is running on the STM32.

The debugger can stop the processor, read its state, and instruct it to continue, but the STM32 is the device actually executing the program instructions.

## 7. Experiment B — Run, suspend, and inspect a variable

With the debugger stopped in `main()`:

1. Open the **Expressions** view.
2. Add the expression `counter`.
3. Observe its value.
4. Press **Resume (F8)**.
5. Let the program run briefly.
6. Press **Suspend**.
7. Inspect `counter` again.

Repeat this several times.

### Questions

- Does the value increase while the MCU is running?
- Does it remain stable while execution is suspended?
- What happens when you resume again?

**Expected observation:** `counter` increases while the CPU executes the loop and stops changing when the CPU is suspended.

You may also experiment with **Live Expressions**. Be aware that the displayed value can lag behind the actual processor state.

## 8. Experiment C — Find the variable in RAM

While the processor is suspended:

1. Add the expression `&counter`.
2. Record the address.
3. Open the **Memory** view.
4. Enter the observed address.
5. Compare the raw bytes with the value displayed by Expressions.

For example, one run of this lab observed:

```text
&counter = 0x2000001C

Memory bytes:
80 8D C2 00

32-bit value:
0x00C28D80 = 12,750,208
```

The Cortex-M4 uses **little-endian** data representation. For a 32-bit value, the least significant byte is stored at the lowest address.

Do not expect your variable address or counter value to match the example exactly. They depend on the firmware build and how long the CPU has been running.

### Questions

- What is the difference between `counter` and `&counter`?
- Why does the Memory view show four separate bytes?
- Can you reconstruct the integer value from those bytes?

The important result is that the typed variable in C corresponds to actual bytes at an address in RAM.

## 9. Experiment D — Execute one assembly instruction at a time

Open the **Disassembly** and **Registers** views.

In the Debug view, enable **Instruction Stepping Mode**. With this enabled, the normal stepping commands operate on individual machine instructions rather than C source lines.

Find the assembly instructions generated for `counter++`.

The exact instruction sequence and register choices depend on the compiler and build settings, but you should find operations resembling:

```asm
ldr  r3, [r3, #0]   ; Load the counter value from RAM
adds r3, #1         ; Increment the value in r3
str  r3, [r2, #0]   ; Store the updated value to RAM
```

Step through the sequence using **F5**.

Watch the following:

- **PC** — Program Counter
- **r2 / r3** — General-purpose registers used by the generated code
- **counter** — The value stored in RAM
- **Disassembly** — The next instruction to execute

### Key experiment

Pause immediately before the `adds` instruction.

Record the value of the working register and `counter`.

Execute `adds`.

Then compare the values again.

Next, execute the relevant `str` instruction.

### Expected observation

The addition changes the register first.

The value in RAM does not change until the store instruction executes.

This is one of the central lessons of the lab:

**CPU registers and RAM are separate forms of storage. A calculation in a register does not automatically update a C variable's memory location.**

### Additional observations

Look for:

- `ldr` — Load
- `str` — Store
- `adds` — Arithmetic
- `b.n` — Branch

You may also encounter PC-relative loads and literal pools. A disassembler can sometimes display literal data as if it were instructions; not every byte in a code region represents an instruction intended for execution.

## 10. Experiment E — Nested function calls

We will now use a slightly more interesting program.

Replace `main.c` with:

```c
volatile int counter = 0;

static void bar(void)
{
    counter++;
}

static void foo(void)
{
    bar();
}

int main(void)
{
    while (1)
    {
        foo();
    }
}
```

Save and rebuild the project.

Restart the debug session so that the modified firmware is programmed and execution begins from a known state.

### Set a breakpoint

1. Set a breakpoint on `counter++;` inside `bar()`.
2. Resume execution using **F8**.
3. Wait for the breakpoint to be reached.
4. Inspect the Debug view's **Call Stack**.

**Expected result:**

```text
bar()
foo()
main()
```

This means that the CPU is currently executing `bar()`, which was called by `foo()`, which was called by `main()`.

### What is a call stack?

The debugger's Call Stack view shows the active function-call hierarchy reconstructed from the processor state, stack contents, and debugging information.

It is **not** a literal array of function names stored in RAM.

The physical stack is memory used for purposes including saved registers, return information, and some local variables.

### Inspect the registers

Record:

- PC
- LR
- SP

Find the assembly around the function calls and returns.

You may encounter instructions such as:

```asm
push {r7, lr}
bl   bar
bx   lr
pop  {r7, pc}
```

Their roles are:

| Instruction | Meaning |
|---|---|
| `push {r7, lr}` | Save register values on the stack |
| `bl bar` | Call `bar()` and set return information in LR |
| `bx lr` | Return by branching to the address in LR |
| `pop {r7, pc}` | Restore a register and return directly by restoring PC |

### Follow a function return

Use Instruction Stepping Mode or **Step Return (F7)** to observe execution leaving `bar()`.

Then follow the return from `foo()` to `main()`.

Watch:

- The value of PC after each return
- The change in SP
- The frames disappearing from the Call Stack view

A particularly interesting pattern is `pop {r7, pc}`, which can both restore a saved register and return to the caller in one instruction.

### Questions

- Why must a function preserve its own return information before calling another function?
- Why might LR contain a return address into `foo()` while the processor is executing `bar()`?
- How does the CPU return to `main()` if a function restores PC directly from the stack?

## 11. Experiment F — Step Into versus Step Over

Disable or remove the breakpoint in `bar()` for this experiment.

In Disassembly, navigate to the `bl foo` instruction inside `main()`.

With **Instruction Stepping Mode enabled**, try:

### Step Into — F5

Execute the call instruction using Step Into.

**Expected result:** Execution enters `foo()`.

Continue stepping to inspect the nested call to `bar()`.

### Step Over — F6

Return to the `bl foo` instruction in `main()`.

Now execute **Step Over**.

**Expected result:** The debugger allows the call to complete and stops at the instruction following `bl foo`.

The counter should have increased because `foo()` and `bar()` still executed.

**Step Over does not skip a function's execution. It skips stopping at the internal steps of that execution.**

### Source-level stepping

Disable Instruction Stepping Mode and repeat some steps at the C source level.

Depending on the compiler's debug information and GDB stepping behavior, the highlighted C line may not always move as intuitively as you expect, especially around the infinite loop.

If something appears strange, inspect PC and Disassembly.

A source-level stepping command does not necessarily stop at every instruction executed by the CPU.

## 12. Experiment G — Debugging challenge

You now have the basic tools required to diagnose a small logic defect.

### Requirement

**Every completed invocation of `foo()` shall increase `counter` by exactly one.**

### Problem report

The counter initially behaves as expected, but after a few calls its value becomes larger than expected.

Your task is to determine:

- During which call does the first deviation occur?
- What was the counter value before and after that call?
- Which function or functions were executed?
- Which instructions explain the unexpected behavior?
- What is the root cause?

### Challenge firmware

Keep `bar()` and `main()` unchanged.

Replace only `foo()` with:

```c
static void foo(void)
{
    bar();

    if (counter == 3)
    {
        bar();
    }
}
```

Rebuild and start a new debug session.

### Suggested investigation

Begin by placing a breakpoint on the `foo()` call in `main()`.

Measure the counter at successive visits to the breakpoint.

Remember that a breakpoint placed on a function call normally stops **before** that call executes.

Then restart the program and use stepping, expressions, registers, the Call Stack, and Disassembly to investigate the first failing invocation.

Try to explain the behavior using evidence from the actual instruction stream.

For example, if the generated code contains `cmp` followed by a conditional branch such as `bne`, determine whether the branch was taken and why.

### Record your results

| Completed calls to foo() | Expected counter | Observed counter |
|---|---:|---:|
| 0 | 0 | |
| 1 | 1 | |
| 2 | 2 | |
| 3 | 3 | |
| 4 | 4 | |
| 5 | 5 | |

**Challenge completion criteria:** You can identify the first failing invocation, explain its cause, and support your explanation with debugger observations.

Avoid relying only on reading the C code. The point is to verify what the target processor actually did.

For the completed lab's investigation and explanation, see [notes.md](./notes.md). That document contains spoilers for this challenge.

## 13. Troubleshooting

### The board is not detected

- Verify that the USB cable supports data, not only charging.
- Connect through the board's onboard ST-LINK USB connector.
- Check Windows Device Manager.
- Ensure that ST-LINK drivers are installed.
- Check whether CubeIDE offers an ST-LINK firmware update.

### The debugger does not reach main()

- Confirm that the project built successfully.
- Check that the debug configuration points to the current ELF file.
- Verify that ST-LINK and SWD are selected.
- Confirm that the correct MCU is configured.
- Retry with a fresh debug session and target reset.

### The counter does not update in Expressions

- Check whether the CPU is suspended.
- Resume and suspend again.
- Confirm that you are observing the expected variable.
- Compare with the Memory view if the display seems stale.

### Step Into or Step Over behaves unexpectedly

- Check whether Instruction Stepping Mode is enabled.
- Check the actual PC value and the Disassembly view.
- Inspect active breakpoints.
- Remember that source-level stepping depends on debug information and may not expose every intermediate instruction.

### The Call Stack does not show foo() and bar()

- Place the breakpoint **inside `bar()`**.
- Confirm that execution actually stopped there.
- Ensure that you are debugging the current build.
- For the simplest assembly and source-level debugging experience, use an unoptimized Debug build.

### I see unexpected addresses or assembly

Exact addresses and generated instructions depend on the compiler, build configuration, startup code, and linker script.

Use the observed relationships between instructions and registers rather than relying on addresses copied from another build.

## 14. Completion checklist

Mark each item when you have demonstrated it on the actual target.

- [ ] I can explain the difference between build, flash, and debug.
- [ ] I have successfully flashed and debugged firmware on the STM32.
- [ ] I can use Resume, Suspend, and breakpoints.
- [ ] I can inspect a global variable through Expressions and Memory.
- [ ] I can interpret a little-endian 32-bit value from raw memory bytes.
- [ ] I have stepped through `ldr`, `adds`, and `str`.
- [ ] I can explain the roles of PC, LR, and SP.
- [ ] I have observed a nested call chain: `main()` → `foo()` → `bar()`.
- [ ] I have observed function returns and changes to the Call Stack.
- [ ] I understand the difference between Step Into and Step Over.
- [ ] I have reproduced and diagnosed the debugging challenge.

## 15. What comes next?

This lab deliberately focuses on execution and debugging rather than detailed memory organization.

Natural follow-up topics include:

- Flash and RAM memory layout
- Linker scripts and startup code
- `.text`, `.data`, and `.bss`
- Stack layout and heap allocation
- Interrupts and exceptions
- RTOS context switching
- Bootloaders and firmware updates

Understanding the CPU's execution behavior first provides a practical foundation for exploring these subjects.

---

## Additional resources

- [STM32CubeIDE — STMicroelectronics](https://www.st.com/en/development-tools/stm32cubeide.html)
- [NUCLEO-F446RE — STMicroelectronics](https://www.st.com/en/evaluation-tools/nucleo-f446re.html)
- [STM32F446RE product documentation — STMicroelectronics](https://www.st.com/en/microcontrollers-microprocessors/stm32f446re.html)
- [GNU Debugger (GDB) documentation](https://www.sourceware.org/gdb/documentation/)

## Final takeaway

The most important conceptual transition in this lab is moving from:

**“My C program executes this statement.”**

to:

**“The CPU executes these instructions, changes these registers, accesses these memory addresses, and follows this control flow.”**

That is the foundation of effective low-level embedded debugging.