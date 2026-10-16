# Lab 02 — MCU Memory, Linker, Startup, Stack and Heap

A hands-on embedded C lab for understanding where firmware lives, how RAM is initialized, how function calls use the stack, and how dynamic memory allocation works on a bare-metal microcontroller.

**Target:** STM32 NUCLEO-F446RE (STM32F446RE, Arm Cortex-M4)  
**Development environment:** STM32CubeIDE 2.2.0, GNU Arm toolchain, ST-LINK debugger  
**Language:** C, with a small amount of ARM assembly inspection  
**Project:** `lab02-memory-linker`

## 1. What you will learn

By completing this lab, you will be able to:

- Locate code, constants, initialized variables and zero-initialized variables in Flash and RAM.
- Read a linker script and find symbols in the generated linker map.
- Explain why initialized RAM values are also stored in the Flash image.
- Observe startup code copying `.data` and zeroing `.bss` before `main()`.
- Understand stack growth, stack frames and the role of a frame pointer.
- Trace a function return value through the Arm calling convention.
- See how compiler optimization can eliminate local-variable storage.
- Observe `malloc()`, `free()` and heap metadata in raw memory.
- Predict how a C declaration can affect the linker layout and heap addresses.

The lab emphasizes **observation, prediction and verification**.

You do not need to know ARM assembly beforehand. We will inspect only the instructions relevant to the experiments.

## 2. Requirements

### Hardware

- ST NUCLEO-F446RE development board.
- USB cable supporting data and debugging.
- A computer capable of running STM32CubeIDE.

### Software

- STM32CubeIDE (tested with version 2.2.0).
- GNU Arm toolchain included with the IDE.
- ST-LINK drivers/debugger support.

### Previous knowledge

You should be comfortable with:

- Basic C declarations, functions and pointers.
- Building and debugging a simple embedded project.
- Setting breakpoints and stepping through code.
- Viewing CPU registers and memory in a debugger.

Knowledge of linker scripts or assembly is not required.

## 3. Create the project

1. Open STM32CubeIDE.
2. Create a new **STM32 Empty Project** targeting `NUCLEO-F446RE` / `STM32F446RE`.
3. Name the project `lab02-memory-linker`.
4. Use the normal Debug configuration.
5. Build the project and start a debug session to verify communication with the board.

The generated project should contain files similar to:

```text
lab02-memory-linker/
├── Src/
│   ├── main.c
│   ├── syscalls.c
│   └── sysmem.c
├── Startup/
│   └── startup_stm32f446retx.s
├── STM32F446RETX_FLASH.ld
└── Debug/                    # Generated during build
    ├── lab02-memory-linker.elf
    └── lab02-memory-linker.map
```

Exact paths and generated filenames may vary slightly with the IDE version.

**Important:** Keep the linker script and startup assembly generated for your target. This lab is about inspecting their behavior, not replacing them.

A normal **Build Project** is sufficient after most source changes. You do not need to clean the entire project every time.

## 4. Experiment A — Where do C objects live?

Start with this minimal `Src/main.c` application, preserving any required project-specific boilerplate:

```c
#include <stdint.h>

int initialized_global = 42;
int uninitialized_global;
const int constant_global = 123;

volatile int result;

int main(void)
{
    const volatile int *read_constant = &constant_global;

    result = initialized_global
           + uninitialized_global
           + *read_constant;

    while (1)
    {
    }
}
```

The `const volatile int *` allows us to observe an actual read of the constant in this experiment. `volatile` is not a general substitute for synchronization or memory protection.

### Step A1 — Make a prediction

Before debugging, predict which objects will be stored in:

- `.text`
- `.rodata`
- `.data`
- `.bss`

Also predict which objects should reside in Flash and which should reside in RAM during execution.

### Step A2 — Inspect addresses

Build and start a debug session. Stop at `main()` before the final result assignment.

Add the following to the Expressions view:

```text
&initialized_global
&uninitialized_global
&constant_global
&result
read_constant
*read_constant
```

Addresses beginning with `0x080...` are in the configured Flash region. Addresses beginning with `0x200...` are in SRAM.

In the original reference build, the addresses were:

| Object | Example address | Section |
|---|---|---|
| `initialized_global` | `0x20000000` | `.data` |
| `uninitialized_global` | `0x20000020` | `.bss` |
| `constant_global` | `0x080002EC` | `.rodata` |
| `result` | `0x20000024` | `.bss` |

**Do not rely on these exact addresses.** They can change when code or libraries change.

### Step A3 — Inspect the bytes

Use the Memory view or GDB:

```gdb
x/4xb &initialized_global
x/4xb &constant_global
x/4xb &uninitialized_global
```

For a 32-bit little-endian integer, you should expect:

```text
42  = 0x0000002A  ->  2A 00 00 00
123 = 0x0000007B  ->  7B 00 00 00
```

Continue execution to the infinite loop.

The expected result is:

```text
42 + 0 + 123 = 165
```

Inspect `result` to verify this.

### Questions

- Why is `initialized_global` in RAM when its initial value is known at build time?
- Why does `constant_global` normally remain in Flash?
- Why does `uninitialized_global` start at zero?
- What does little-endian mean for a multi-byte integer?

## 5. Experiment B — Read the linker script and map file

Open:

```text
STM32F446RETX_FLASH.ld
```

Find the `MEMORY` definition.

For this device, the relevant configuration is:

```text
FLASH origin: 0x08000000
FLASH length: 512 KiB

RAM origin:   0x20000000
RAM length:   128 KiB
```

Then locate the output sections:

```text
.text
.rodata
.data
.bss
```

Pay particular attention to the `.data` placement, usually expressed with a construct such as:

```ld
.data :
{
    _sdata = .;
    *(.data*)
    _edata = .;
} >RAM AT>FLASH

_sidata = LOADADDR(.data);
```

This says that `.data` executes or resides at RAM addresses, while its initial contents are stored in the Flash image.

### VMA versus LMA

GNU linker terminology:

- **VMA:** The address used for the section at runtime.
- **LMA:** The address where the section's initial image is loaded or stored.

A microcontroller does not need an MMU for GNU ld to use the term VMA.

### Inspect the map file

After building, open:

```text
Debug/lab02-memory-linker.map
```

Search for:

```text
Linker script and memory map
initialized_global
constant_global
_sidata
_sdata
_edata
_sbss
_ebss
```

Record the runtime address and Flash load address of `.data`.

You should be able to explain how a value stored in Flash eventually appears in writable RAM.

## 6. Experiment C — Watch startup initialize RAM

This experiment happens **before `main()`**.

Open:

```text
Startup/startup_stm32f446retx.s
```

Find `Reset_Handler`.

Look for code that:

1. Sets or restores the stack pointer.
2. Copies initialized data from Flash into RAM.
3. Clears `.bss`.
4. Calls C runtime initialization.
5. Calls `main()`.

### Step C1 — Stop before the .data copy

Configure the debugger to stop in startup code rather than automatically running straight to `main()`. Set a breakpoint at the data-copy store instruction or its corresponding assembly label.

The relevant instruction may resemble:

```asm
ldr r4, [r2, r3]
str r4, [r0, r3]
```

Inspect:

```gdb
info registers r0 r1 r2 r3 r4
```

Identify:

- RAM destination address.
- Flash source address.
- Byte offset.
- Value about to be copied.

Read the destination RAM before the store, single-step the store, and read it again.

In the reference experiment, a single store caused RAM at `0x20000000` to contain:

```text
2A 00 00 00
```

The important observation is the **change caused by the startup instruction**, not the initial arbitrary RAM contents.

### Step C2 — Stop before .bss zeroing

Find the zeroing loop, which uses an instruction similar to:

```asm
str r3, [r2]
```

In the reference build, `.bss` occupied:

```text
0x20000004 through 0x20000027
```

Before the zeroing store, temporarily write test patterns into a few words using GDB:

```gdb
set {unsigned int}0x20000004 = 0x11223344
set {unsigned int}0x20000008 = 0xFFFFFFFF
set {unsigned int}0x2000000C = 0x55555555
```

**Use addresses from your own linker map.** These writes are intended only for the controlled RAM experiment.

Step the zeroing instruction and observe which word changes.

Then allow the remainder of the `.bss` initialization loop to run.

### Questions

- What initializes writable C objects before `main()`?
- Why does `.data` need Flash storage and RAM storage?
- Why does `.bss` not require an equally sized initialized image in Flash?
- What would happen if startup skipped either initialization loop?

## 7. Experiment D — Stack frames and function return values

Add this function above `main()`:

```c
__attribute__((noinline))
static int simple_local(void)
{
    int x = 42;
    return x;
}
```

Inside `main()`, add:

```c
int local_result = simple_local();
```

Include the return value in the sum:

```c
result = initialized_global
       + uninitialized_global
       + *read_constant
       + local_result;
```

The expected result is now:

```text
42 + 0 + 123 + 42 = 207
```

### Step D1 — Inspect the unoptimized function

Build in Debug configuration and inspect:

```gdb
disassemble /r simple_local
```

In the reference unoptimized build, the function:

- Saved a register.
- Reserved stack space.
- Stored 42 in its stack frame.
- Read the value back.
- Placed the return value in `r0`.
- Restored the stack and returned.

Useful instructions to recognize:

```asm
push
sub sp, #...
add r7, sp, #...
str
ldr
mov
bx lr
```

### Step D2 — Inspect main()

Run:

```gdb
disassemble /r main
```

Find:

```asm
bl simple_local
```

Look immediately after the function call.

According to the Arm AAPCS used here, a normal 32-bit `int` return value is passed back in `r0`.

In the reference build, `main()` stored `r0` in its own stack frame and later loaded it for the calculation.

Inspect:

```gdb
info registers sp r7 r0
p/x &local_result
p/x &read_constant
```

### Questions

- Why does `main()` save LR when it calls another function?
- What is a function prologue?
- Does every local variable necessarily have an address in RAM?
- How does `simple_local()` communicate its return value to `main()` without knowing the name `local_result`?

## 8. Experiment E — Compiler optimization

Change the helper function to:

```c
__attribute__((noinline, optimize("O1")))
static int simple_local(void)
{
    int x = 42;
    return x;
}
```

The attributes are GCC-specific and are used only to create a controlled comparison.

Build again and run:

```gdb
disassemble /r simple_local
```

In our reference build, the optimized function became:

```asm
movs r0, #42
bx   lr
```

Compare this with the earlier version.

### Questions

- Where is `x` stored now?
- Does the function need a stack frame?
- Did the compiler preserve the function's behavior?
- What does this tell us about the relationship between a C variable and its physical representation?

The key lesson is that compiler optimization can eliminate the need for a separately stored local object.

## 9. Experiment F — Heap and dynamic allocation

Add:

```c
#include <stdlib.h>
```

Inside `main()`, before the result calculation:

```c
unsigned char *heap_ptr = malloc(16);

if (heap_ptr != NULL)
{
    heap_ptr[0] = 0x5A;
}

unsigned char *heap_ptr2 = malloc(16);

if (heap_ptr2 != NULL)
{
    heap_ptr2[0] = 0xA5;
}
```

### Step F1 — Read the platform heap implementation

Open:

```text
Src/sysmem.c
```

Find:

```c
_sbrk(...)
```

This function supports the C runtime by extending the heap area available to its allocator.

Look for the symbols:

```text
_end
_estack
_Min_Stack_Size
```

Inspect them in the debugger:

```gdb
p/x (unsigned int)&_end
p/x (unsigned int)&_estack
p/x (unsigned int)&_Min_Stack_Size
```

The implementation uses a software-calculated boundary to avoid growing the heap into a reserved stack margin. This is **not hardware-enforced stack/heap separation**.

### Step F2 — Inspect the pointers

Set a breakpoint **after both `malloc()` calls and their test writes**, but before any `free()` call.

Run:

```gdb
p/x heap_ptr
p/x heap_ptr2
p/x &heap_ptr
p/x (unsigned int)&_end
```

Notice the difference between:

- `heap_ptr`: address of the allocated block.
- `&heap_ptr`: address of the pointer variable itself.
- `*heap_ptr`: first byte of the block.

Examine the two blocks:

```gdb
x/16xb heap_ptr
x/16xb heap_ptr2
```

If both allocations succeeded, the first byte of each should contain its test value.

### Step F3 — Investigate allocator overhead

Compare the block addresses.

In the reference newlib-nano build:

```text
heap_ptr  = 0x200001D0
heap_ptr2 = 0x200001E8
```

The starting addresses were 24 bytes apart, even though each request was for 16 bytes.

The eight bytes immediately preceding each block contained structured data consistent with allocator metadata.

You can inspect nearby raw memory using GDB, for example:

```gdb
x/8xb heap_ptr-8
x/8xb heap_ptr2-8
```

This is debugger inspection of allocator-managed memory. **Do not write C code that accesses outside the 16 bytes returned by `malloc()`.**

The exact allocator layout is an implementation detail. Do not assume a universal eight-byte overhead.

### Step F4 — Free and reuse a block

After the two allocations, add:

```c
free(heap_ptr);
heap_ptr = NULL;

unsigned char *heap_ptr3 = malloc(16);

if (heap_ptr3 != NULL)
{
    heap_ptr3[0] = 0x3C;
}
```

Use breakpoints to observe:

1. The first block's contents and nearby metadata before `free()`.
2. The same addresses immediately after `free()`.
3. The address returned by the third `malloc()`.
4. The first byte of that new block **before** writing `0x3C`.
5. The same byte after writing `0x3C`.

In the reference experiment, the third allocation reused the first block's address.

The previous byte value was still present until application code overwrote it.

### Important rules

- `malloc()` can return `NULL`.
- `malloc()` does not initialize the allocated bytes.
- `free()` does not necessarily erase memory.
- `free()` does not automatically set the caller's pointer to `NULL`.
- Accessing a block through an old pointer after it has been freed is invalid.
- A later `malloc()` may reuse the same address.
- Use `calloc()` if zero-initialized allocated storage is required.

This lab investigates allocator behavior. It does not recommend dynamic allocation for all embedded applications.

## 10. Level C challenge — A local buffer changes the memory layout

You have now investigated static sections, startup, stack frames and the heap.

Try this challenge **before reading the explanation below**.

Add the following inside `main()`, before the first `malloc()`:

```c
static volatile unsigned char diagnostic_buffer[256];
diagnostic_buffer[0] = 0x7E;
```

### Predict before building

What will happen to:

1. The address of `diagnostic_buffer`?
2. The value of `_end`?
3. The stack pointer after `main()`'s prologue?
4. The address returned by the first `malloc(16)`?

Which memory section will contain the buffer?

Write down your hypothesis before building.

### Investigate

Build and use GDB:

```gdb
p/x &diagnostic_buffer
p/x (unsigned int)&_end
info registers sp r7
```

Locate the buffer in the `.map` file.

Also inspect the buffer contents before and after:

```c
diagnostic_buffer[0] = 0x7E;
```

Finally, stop just after the first `malloc()`:

```gdb
p/x heap_ptr
```

Compare these addresses with your previous build.

### Explanation and reference result

The keyword `static` gives the local object **static storage duration**, even though its name has local scope.

An uninitialized local static buffer is guaranteed by C to be zero-initialized before normal program execution.

In our linker configuration, the buffer was placed in `.bss`:

```text
.bss.diagnostic_buffer.0
    0x20000078    0x100    ./Src/main.o
```

The section grew by 256 bytes.

We observed:

| Symbol / expression | Before buffer | After buffer |
|---|---|---|
| `_end` | `0x200001C8` | `0x200002C8` |
| `heap_ptr` | `0x200001D0` | `0x200002D0` |

Both shifted by exactly 256 bytes.

The buffer did not require an additional 256-byte automatic allocation in `main()`'s stack frame.

This is a direct example of how a C declaration affects the linker layout and, indirectly, dynamic allocation.

## 11. Common debugging pitfalls

### Looking at a variable before its initialization

If the debugger stops on:

```c
unsigned char *heap_ptr = malloc(16);
```

the allocation may not have executed yet.

The debugger can sometimes show stale bytes from the variable's future stack location. That is **not a valid malloc return value**.

Stop on the next source line before reading `heap_ptr`.

### Inspecting a pointer after setting it to NULL

If the code includes:

```c
free(heap_ptr);
heap_ptr = NULL;
```

then inspecting `heap_ptr` afterward will show `NULL`, even if the previous allocation succeeded.

Inspect it earlier, or save the old address in a GDB convenience variable:

```gdb
set $old_block = heap_ptr
```

### Confusing the map file with source code

A map file contains output section placement, input sections, linker symbols, library objects and discarded sections.

Search for the real output section or symbol rather than treating every line as an object occupying runtime memory.

### Mistaking literal-pool data for instructions

Some disassemblers render words from a literal pool as apparent instructions.

Compare the disassembly with the program counter and inspect suspected literal-pool addresses as data using `x/wx`.

### Assuming addresses are permanent

Adding a function, static buffer or library call can change the ELF layout.

Measure addresses in your **current build** instead of relying on values from this README.

### Confusing software checks with hardware protection

A linker script can reserve space. `_sbrk()` can check a heap limit. Neither automatically configures a Memory Protection Unit.

Linker layout, allocation policies and hardware-enforced access protection are separate topics.

## 12. Quick GDB reference

| Command | Meaning |
|---|---|
| `p/x variable` | Print value in hexadecimal |
| `p/d variable` | Print value in decimal |
| `p/x &variable` | Print variable address |
| `x/16xb address` | Examine 16 bytes, hexadecimal |
| `x/4wx address` | Examine four 32-bit words, hexadecimal |
| `info registers sp r7 lr` | Display selected CPU registers |
| `disassemble /r main` | Show assembly and instruction encodings |
| `set $saved = expression` | Save a value for later debugger inspection |
| `set {unsigned int}ADDRESS = VALUE` | Write a 32-bit word to target memory |

Be careful when modifying target memory directly. Use only controlled test addresses within valid RAM.

## 13. Completion checklist

You have completed the lab when you can explain and demonstrate the following:

- [ ] Locate one object each in `.rodata`, `.data` and `.bss`.
- [ ] Identify the `.data` runtime and load addresses.
- [ ] Observe startup copying one initialized value into RAM.
- [ ] Observe startup clearing `.bss`.
- [ ] Explain how a function reserves and accesses its stack frame.
- [ ] Trace a 32-bit `int` return value through `r0`.
- [ ] Show how optimization removes unnecessary stack accesses.
- [ ] Distinguish a pointer variable from the heap block it points to.
- [ ] Observe two allocations, a `free()` and an allocation reuse.
- [ ] Explain why a local `static` buffer can occupy `.bss` and move `_end`.

## 14. Further exploration

This lab provides a foundation for more advanced embedded topics:

- Interrupt entry, exception return and stack usage.
- RTOS task stacks and memory allocation strategies.
- Linker-defined memory partitions and MPU configuration.
- AUTOSAR memory mapping.
- Multicore systems with local and shared memories.
- Bare-metal development using languages with explicit allocator interfaces, such as Zig.

For the original learner's detailed observations and personal takeaways, see [`notes.md`](notes.md).

---

**Main takeaway:** A C declaration is only the beginning of the story. To understand where an object really lives and how it is initialized or accessed, examine the generated firmware, the linker map, startup code and the running MCU together.