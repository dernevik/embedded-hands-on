# Lab 02 — Memory, Linker, Startup, Stack and Heap

## Purpose of these notes

These are my personal takeaways from Lab 2 on the STM32F446RE.

The goal was yo chek out how C code becomes a firmware image, how that image uses Flash and RAM, and how to verify the behavior using real hardware and a debugger.

One obvious lesson was that **the abstractions of the C language do not map one-to-one to physical memory locations**.

The compiler, linker, startup code, calling convention and runtime library all contribute to what eventually happens in memory.

## 1. The overall mental model


```text
C source
   |
   v
Compiler / assembler
   |
   v
Object files (.o)
   |
   v
Linker + linker script
   |
   v
ELF firmware image
   |
   v
Flash programming
   |
   v
Reset_Handler
   |
   +--> Initialize .data in RAM
   +--> Zero .bss in RAM
   +--> Initialize C runtime
   |
   v
main()
   |
   +--> Stack frames and registers
   +--> Static/global objects
   +--> Optional dynamic allocation
```

The linker decides where sections and symbols belong. Startup prepares the RAM state expected by C. The generated machine code determines how functions, registers and runtime memory are actually used.

## 2. Memory regions and linker sections

The STM32F446RE project uses:

| Physical region | Address range | Size |
|---|---|---|
| Flash | `0x08000000`–`0x0807FFFF` | 512 KiB |
| SRAM | `0x20000000`–`0x2001FFFF` | 128 KiB |

The address `0x20020000` is the end of this SRAM region.

Important sections:

| Section | Typical contents | Runtime location |
|---|---|---|
| `.text` | Executable instructions | Flash |
| `.rodata` | Read-only constants | Flash |
| `.data` | Initialized writable static/global data | RAM |
| `.bss` | Zero-initialized writable static/global data | RAM |

These are conventions implemented by the compiler and linker script.

My first program used:

```c
int initialized_global = 42;
int uninitialized_global;
const int constant_global = 123;
volatile int result;
```

I observed:

| Object | Address | Initial value |
|---|---|---|
| `initialized_global` | `0x20000000` | 42 |
| `uninitialized_global` | `0x20000020` | 0 |
| `constant_global` | `0x080002EC` | 123 |
| `result` | `0x20000024` | 0 |

These are **build-specific addresses**. They changed as the firmware grew.

### Little-endian representation

In the Memory view:

```text
42  = 0x0000002A --> 2A 00 00 00
123 = 0x0000007B --> 7B 00 00 00
165 = 0x000000A5 --> A5 00 00 00
```

Little-endian describes the order of bytes within a multi-byte value. It does not reverse the bits within each byte.

## 3. The linker script and map file

The linker script specifies memory regions, section placement and important linker symbols.

The generated `.map` file tells me where the linker actually placed things.

A useful approach is to search for:

- The `Memory Configuration` section.
- `Linker script and memory map`.
- A particular symbol, such as `initialized_global`.
- The `.data`, `.bss`, `.text` or `.rodata` output section.

**input sections** comes from individual object files, **output sections** comes from the linker.

In the map file there are also discarded sections or library-loading records that are not part of the final memory layout.

### VMA and LMA

GNU linker terminology:

- **VMA (Virtual Memory Address):** Where a section is addressed when the program runs.
- **LMA (Load Memory Address):** Where its initial image is stored.

The word "virtual" in VMA does not imply the STM32 uses an MMU.

My original `.data` section demonstrated the distinction:

```text
.data runtime address: 0x20000000
.data load address:    0x080002F8
```

The initialized value 42 was stored in the Flash image but had to end up at `0x20000000` in RAM.

### Useful linker symbols

```text
_sidata  Source of .data initial values in Flash
_sdata   Start of .data in RAM
_edata   End of .data in RAM

_sbss    Start of .bss
_ebss    End of .bss

_end     Start of the area available to the heap runtime
_estack  Initial top of the stack / end of RAM
```

A symbol defined by a linker script is not necessarily an ordinary C variable stored in memory. For absolute linker symbols, it is the symbol's **address value** that matters.

## 4. Startup really does initialize RAM before main()

### Copying `.data`

I stopped in `Reset_Handler`, at the instruction:

```asm
str r4, [r0, r3]
```

The relevant register values were:

```text
r0 = 0x20000000    RAM destination
r1 = 0x20000004    End of .data
r2 = 0x080002F8    Flash source
r3 = 0             Byte offset
r4 = 42            Value being copied
```

Before the store, RAM did not contain the initialized value.

After stepping that single instruction, the destination contained:

```text
2A 00 00 00
```

This proved that the startup code copies its initial contents from Flash.

### Clearing `.bss`

The initial `.bss` boundaries were:

```text
_sbss = 0x20000004
_ebss = 0x20000028
```

That is `0x24`, or 36 bytes.

The startup loop performed nine 32-bit zero stores.

I placed nonzero patterns in RAM with GDB, then stepped through the zeroing code. The patterns were cleared as the stores executed.

This demonstrates the implementation of C's zero-initialization guarantee for objects with static storage duration.

### Reset and startup are separate from main()

On a Cortex-M, the reset vector mechanism establishes initial execution state, including SP and PC. `Reset_Handler` then performs platform and runtime initialization before calling `main()`.

 `main()` is not the first code executed after reset.

## 5. Scope, storage duration and physical placement are different concepts

Consider:

```c
void example(void)
{
    unsigned char automatic_buffer[256];
    static unsigned char static_buffer[256];
}
```

Both identifiers have local scope.

But they have different **storage durations**.

### Automatic local variable

```c
unsigned char automatic_buffer[256];
```

- The object has automatic storage duration.
- Its contents are not automatically initialized.
- A stack frame is a typical implementation when storage is required.
- The compiler may optimize the object away if its behavior permits.

### Static local variable

```c
static unsigned char static_buffer[256];
```

- The object has static storage duration.
- It exists for the entire execution of the program.
- Without an explicit initializer, its bytes are guaranteed to start at zero.
- In our linker configuration, it is typically placed in `.bss`.
- It retains its value across calls to the function.

The fact that a declaration appears inside a function does not mean the object lives on the stack.

This was new to me: a local `static` buffer without an explicit initializer is still guaranteed to be zero-initialized by C.

### The other meaning of static

For a file-scope function:

```c
static int helper(void);
```

`static` instead gives the function name *internal linkage*.

C uses the same keyword for several related but distinct language concepts.

### volatile

`volatile` affects how accesses to an object must be treated by the compiler. It does **not** mean:


For a pointer declaration:

```c
const volatile int *p;
```

the pointed-to `int` is const-qualified and volatile-qualified through `p`. The pointer variable itself is not const-qualified and can be reassigned.

## 6. Stack frames are addressable memory, not just push/pop

The stack is not a strict last-in-first-out structure.

A function can reserve an entire region of the stack and access individual locations inside it.

A typical unoptimized function prologue looked like this:

```asm
push {r7, lr}
sub  sp, #8
add  r7, sp, #0
```

Meaning:

1. Save registers needed later.
2. Decrease SP to reserve local storage.
3. Use `r7` as a frame pointer.

The function can then access its frame directly:

```asm
str r0, [r7, #0]
ldr r3, [r7, #0]
```

These are direct memory accesses. They are not push or pop operations.

The stack grows toward **lower memory addresses** on this target.

I prefer to draw memory with lower addresses at the top and higher addresses at the bottom, as in the debugger's Memory view. In that representation, stack growth visually moves upward.

### The frame pointer is a reference point

In our unoptimized build, `r7` was set equal to SP after the stack space had been reserved.

Local variables were then accessed through fixed offsets from `r7`.

I observed `read_constant` at `0x2001FFF4` in one version of the program.

### Registers are not C variables

Stepping an instruction that changes `r3` from:

```text
0x080002EC
```

to:

```text
0x20000000
```

leaves the variable unchanged at its stack address.

The register is temporary working storage. Changing its value does not automatically change the C variable whose value had previously passed through that register.

## 7. Function calls and the ARM calling convention

I learned a rule from **AAPCS**, the Arm Procedure Call Standard:

> On our target, a function returning a normal 32-bit `int` places the return value in `r0`.

For example:

```c
static int simple_local(void)
{
    int x = 42;
    return x;
}
```

In the unoptimized code, the value was eventually moved into `r0` before the function returned.

In `main()`:

```c
int local_result = simple_local();
```

the compiler emitted an instruction to store the return value into its stack frame:

```asm
bl  simple_local
str r0, [r7, #0]
```

Later the value was loaded again:

```asm
ldr r3, [r7, #0]
```

An important discovery: **the called function does not know anything about `local_result`.** It only fulfills the ABI contract by returning an `int` in `r0`.

A function that calls another function may need to save LR because a `bl` instruction updates LR.

## 8. Compiler optimization can eliminate local storage completely

I compared the same function with and without optimization.

Without optimization, the compiler created a stack frame, stored 42, loaded it back and returned it.

I then used GCC-specific attributes:

```c
__attribute__((noinline, optimize("O1")))
static int simple_local(void)
{
    int x = 42;
    return x;
}
```

The resulting implementation was only:

```asm
movs r0, #42
bx   lr
```

No stack frame. No memory reads. No memory writes.

**The compiler eliminated the need for a separately represented object `x`.**

C specifies the program's required behavior, not a requirement that every source-level variable have a permanent physical memory location.

Assembly and debug information can change considerably with optimization.

## 9. Literal pools and disassembly

Some 32-bit constants or addresses are stored in Flash near the machine code.

The compiler can load them using PC-relative addressing:

```asm
ldr r3, [pc, #offset]
```

The word fetched might itself be an address, such as:

```text
0x20000000
```

When disassembling a whole function, GDB may display such literal-pool bytes as apparent instructions like `movs` or `lsls`.

Those are not instructions the CPU executes. They are **data interpreted as code by the disassembler**.

A useful verification is to inspect the location directly as a 32-bit word:

```gdb
x/wx 0x08000248
```

I need to distinguish:

1. The instruction loading a word from the literal pool.
2. The address value stored in that pool.
3. The object or value found at the loaded address.

## 10. Heap allocation: what malloc actually does

This was largely unfamiliar to me.

The key distinction is:

```c
unsigned char *heap_ptr = malloc(16);
```

**two separate things**:

- `heap_ptr`: a C pointer variable, which happened to live in `main()`'s stack frame in our build.
- The 16-byte block: memory managed by the heap allocator, at the address stored in `heap_ptr`.

In one build I observed:

```text
heap_ptr   = 0x200001D0
&heap_ptr  = an address near 0x2001FFEC
*heap_ptr  = 0x5A after write
```

### newlib-nano and _sbrk

The project uses newlib-nano as its C runtime library.

Its `malloc()` implementation can request more heap space through the project's `_sbrk()` implementation in `Src/sysmem.c`.

`_sbrk()` tracks the current end of the heap area and adjusts it. It is not itself a complete allocator.

In our firmware:

```text
_end             = 0x200001C8
_estack          = 0x20020000
_Min_Stack_Size  = 0x400
```

The software heap limit calculated by `_sbrk()` was:

```text
0x20020000 - 0x400 = 0x2001FC00
```

This reserves a stack margin according to the implementation, but it is **not MPU-enforced protection**.

`_Min_Heap_Size` in the linker script participates in the linker layout and minimum-space checks. It is not simply the maximum number of bytes that `malloc()` may allocate.

### malloc does not initialize its bytes

After `malloc(16)`, the returned memory contained old bytes.

I then wrote:

```c
heap_ptr[0] = 0x5A;
```

and confirmed that the first byte changed to `0x5A`.

Use `calloc()` when zero-initialized allocated storage is required.

### Observed allocator metadata

Two calls to `malloc(16)` returned:

```text
heap_ptr  = 0x200001D0
heap_ptr2 = 0x200001E8
```

The starting addresses differed by 24 bytes, even though each request was for 16 usable bytes.

The eight bytes preceding each user block were:

```text
18 00 00 00 FC FF FF FF
```

Interpreting the first word as little-endian gives `0x18`, or 24 bytes.

This is strong evidence of per-block allocator metadata in this experiment.

We did not establish the exact meaning of every metadata field.

### free does not erase memory

After:

```c
free(heap_ptr);
```

the preceding metadata changed:

```text
Before: 18 00 00 00 FC FF FF FF
After:  18 00 00 00 00 00 00 00
```

But the first user-data byte still contained `0x5A`.

The pointer variable itself retains its previous address until we explicitly write:

```c
heap_ptr = NULL;
```

A later:

```c
heap_ptr3 = malloc(16);
```

returned the same RAM address as the original first allocation:

```text
heap_ptr3 = 0x200001D0
```

Before writing anything new, GDB still showed `0x5A` at that address. After writing `0x3C`, it showed `0x3C`.

This demonstrated allocator reuse and the danger of stale data and dangling pointers.

The essential rules are:

- Check the result of `malloc()` before dereferencing it.
- `free()` returns ownership of the block to the allocator.
- `free()` does not automatically reset the caller's pointer.
- Do not dereference a pointer after freeing its block.
- Do not rely on data left over from a previous allocation.
- Setting a pointer to `NULL` does not invalidate other aliases to the same block.

## 11. Level C discovery: a local static buffer moved the heap

I added:

```c
static volatile unsigned char diagnostic_buffer[256];
diagnostic_buffer[0] = 0x7E;
```

inside `main()`.

My initial prediction was wrong: I thought the buffer would consume roughly 256 extra stack bytes because it was declared inside a function.

The map file showed:

```text
.bss.diagnostic_buffer.0
    0x20000078    0x100    ./Src/main.o
```

So the buffer occupied 256 bytes in `.bss`, not in `main()`'s stack frame.

The observed changes were:

| Item | Before | After |
|---|---|---|
| `_end` | `0x200001C8` | `0x200002C8` |
| First `malloc(16)` | `0x200001D0` | `0x200002D0` |

Both addresses moved by exactly `0x100`, or 256 bytes.

That gave me a clear cross-layer chain:

```text
Local declaration with static storage duration
                  |
                  v
          .bss grows by 256
                  |
                  v
        _end increases by 256
                  |
                  v
     First malloc address increases
```

This connects the C keyword `static` directly to linker layout and runtime heap behavior.

## 12. Useful GDB commands to remember

| Command | Meaning |
|---|---|
| `p/x expression` | Evaluate an expression and print it in hexadecimal |
| `p/d expression` | Print in decimal |
| `p/t expression` | Print in binary |
| `p/x &variable` | Print the address of a variable |
| `x/16xb address` | Examine 16 bytes, displayed in hex |
| `x/4wx address` | Examine four 32-bit words, displayed in hex |
| `info registers sp r7 r0` | Inspect selected CPU registers |
| `disassemble /r main` | Disassemble a function, including instruction encodings |
| `set $old_block = heap_ptr` | Save an address in a GDB convenience variable |
| `set {unsigned int}0x20000000 = 0` | Write a 32-bit value directly to target RAM |

The two `x` characters in `x/16xb` mean different things:

- First `x`: *examine memory*.
- Second `x`: *hexadecimal output format*.
- `b`: byte-sized units.

### Debugger timing matters

I repeatedly learned the importance of stopping at the **right instruction**.

If a breakpoint is at:

```c
unsigned char *heap_ptr = malloc(16);
```

the statement usually has not executed yet. Inspecting `heap_ptr` at this point does not give a valid malloc result.

Similarly, after:

```c
free(heap_ptr);
heap_ptr = NULL;
```

seeing a null pointer tells me nothing about the earlier success of `malloc()`.

For reliable experiments:

1. Predict what should change.
2. Stop immediately before the relevant operation.
3. Record the current state.
4. Execute the operation.
5. Record the state again.
6. Compare.

The Expressions view, GDB console, Memory view, disassembly and map file are complementary tools.

### Build versus Clean

A normal **Build Project** is sufficient after most source changes. The build system should recompile changed files and relink as needed.

**Clean Project** is useful when I suspect stale generated artifacts or have changed substantial build configuration, but it is not required before every build.

## 13. What I want to retain

The most important long-term takeaways are:

1. **Startup code is essential to the C runtime model.** It prepares initialized and zero-initialized objects before `main()`.
2. **A stack frame is an addressable memory region.** It is not just a sequence of push/pop operations.
3. **Registers are working storage, not physical representations of C variables.**
4. **AAPCS defines how independently compiled functions communicate**, including returning a 32-bit `int` in `r0`.
5. **Optimization can eliminate variable storage altogether.**
6. **`malloc()` manages ownership of RAM.** It does not initialize bytes.
8. **`free()` changes allocation state, not the memory contents.**
9. **An uninitialized local `static` object is guaranteed to be zero-initialized.**
10. **The linker map and debugger let me test explanations.**

## 14. Connections to future work

These lessons should be relevant when exploring:

- AUTOSAR memory mapping and compiler/linker sections.
- Stack allocation and stack monitoring in RTOS tasks.
- Memory protection versus mere linker placement.
- Local versus shared memory on multicore MCUs.
- Allocation policies in safety-related embedded software.

I am also interested in exploring **Zig on the NUCLEO-F446RE** in the future, especially its explicit allocator interfaces, fixed-buffer allocators, bare-metal build configuration and generated machine code.

That would provide an interesting comparison with the C/newlib experiments from this lab.