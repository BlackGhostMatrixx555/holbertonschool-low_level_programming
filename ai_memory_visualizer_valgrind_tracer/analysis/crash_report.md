# Crash Report — `crash_example`

## 1. Crash Description

**Program:** `crash_example`
**Signal received:** `SIGSEGV` (signal 11 — Segmentation Fault)
**Exit code:** 139 (128 + 11)
**Observed output before crash:**
```
crash_example: deterministic NULL dereference (segmentation fault)
  requesting n=0
Segmentation fault
```

The program prints both lines successfully, then crashes before producing any
further output. The crash occurs between line 30 and line 34 of `crash_example.c`.

This crash is **fully deterministic**: given the same source code and the same
input (none — `n` is hardcoded to `0`), it will crash every time, on every
conforming platform. It does not depend on memory layout, ASLR, or timing.

---

## 2. Root Cause Analysis

### The invalid memory access

**Line 32:**
```c
nums[0] = 42;
```

At this point, `nums` holds the value `NULL` (`0x0`).

The expression `nums[0]` is equivalent to `*(nums + 0)` = `*(NULL)` = `*(0x0)`.
This attempts to **write 4 bytes** (one `int`) at virtual address `0x00000000`.

On Linux (and all POSIX-compliant systems), virtual page 0 (`0x0000–0x0FFF`) is
deliberately left **unmapped**. There is no physical memory backing that address.
The CPU's MMU raises a page fault, the kernel finds no mapping for the faulting
address, and delivers `SIGSEGV` to the process.

### The disassembly confirms the exact instruction

From `objdump -d crash_example`:

```asm
; Line 30: nums = allocate_numbers(n)  → returns NULL → stored in -0x8(%rbp)
1282:  mov    %rax,-0x8(%rbp)     ; nums = return value of allocate_numbers (NULL)

; Line 32: nums[0] = 42
1286:  mov    -0x8(%rbp),%rax     ; load nums into rax  →  rax = 0x0
128a:  movl   $0x2a,(%rax)        ; write 42 (0x2a) to address in rax = 0x0
                                  ; ← THIS IS THE FAULTING INSTRUCTION
```

The faulting instruction is `movl $0x2a, (%rax)` at offset `0x128a`.
`rax` contains `0x0` (NULL). Writing to address `0x0` raises `SIGSEGV`.

---

## 3. Full Causal Chain

### Step 1 — `n` is initialized to `0`

```c
int n = 0;   /* line 25 */
```

`n` is a local variable on `main`'s stack frame. Its value is `0`.
There is no user input, no runtime condition, no randomness. `n` is always `0`.

### Step 2 — `allocate_numbers(0)` returns `NULL`

```c
static int *allocate_numbers(int n)
{
    int *arr = NULL;
    int i = 0;

    if (n <= 0)       /* line 9: 0 <= 0 is TRUE */
        return NULL;  /* line 10: returns immediately */
    ...
}
```

`n = 0` satisfies the guard condition `n <= 0`. The function returns `NULL`
**before reaching `malloc`**. No heap allocation is performed.

This is correct, defensive behavior on the part of `allocate_numbers`. The function
correctly refuses to allocate zero bytes and signals failure by returning `NULL`.

### Step 3 — The return value is not checked

```c
nums = allocate_numbers(n);   /* line 30: nums = NULL */

nums[0] = 42;                  /* line 32: dereferences NULL — no check performed */
```

`main` stores the return value in `nums` without testing whether it is `NULL`.
This is the **programmer error**: the contract of `allocate_numbers` (and of
`malloc` in general) requires the caller to check the return value before use.

### Step 4 — NULL dereference

`nums[0]` with `nums == NULL`:

```
nums[0]
= *(nums + 0)
= *(NULL + 0)
= *(0x00000000)
→ write 4 bytes at virtual address 0x0
→ page 0 is unmapped (PROT_NONE on Linux)
→ MMU raises page fault
→ kernel: no VMA covers 0x0, fault is not resolvable
→ kernel sends SIGSEGV to the process
→ process terminates with signal 11, exit code 139
```

### Memory state at the time of the crash

```
STACK (main frame)
┌──────────────────────────────────────┐
│  int *nums = 0x0  (NULL)             │  ← stored at -0x8(%rbp)
│  int  n    = 0                       │  ← stored at -0xc(%rbp)
└──────────────────────────────────────┘

HEAP
  (empty — malloc was never called)

Attempted write: address 0x0000000000000000, size 4 bytes
Virtual memory mapping at 0x0: NONE (unmapped)
Result: SIGSEGV
```

---

## 4. Category of Undefined Behavior

The undefined behavior here is a **null pointer dereference** — specifically a
**write through a null pointer**.

Under the C standard (both C89 and C99), dereferencing a null pointer is undefined
behavior. The standard does not specify what happens; the implementation may do
anything. On Linux/x86-64, the practical result is always `SIGSEGV` because page 0
is never mapped, but this is a platform behavior, not a C guarantee.

This is **not** a use-after-free, a stack overflow, a buffer overflow, or an
uninitialized variable. It is a direct dereference of a null pointer that was never
initialized to point to valid memory.

The undefined behavior begins precisely at **line 32**, the moment `nums[0]` is
evaluated with `nums == NULL`. Lines 30 and below are well-defined.

---

## 5. AI-Proposed Explanations — Critical Evaluation

The following explanations were generated by an AI assistant and evaluated against
the source code and disassembly.

---

### AI claim 1 — Presented as correct

> "The crash is caused by passing `n=0` to `allocate_numbers`. The function
> calls `malloc(0)`, which is implementation-defined: it may return NULL or a
> unique non-NULL pointer. If it returns NULL, the subsequent `nums[0] = 42`
> crashes."

**Verdict: Incorrect.**

This explanation invents a code path that does not exist. `allocate_numbers`
does **not** call `malloc` when `n <= 0`. The guard condition on line 9 returns
`NULL` **before reaching line 12** where `malloc` is called.

```c
if (n <= 0)
    return NULL;   /* returns here — malloc never executed */

arr = (int *)malloc((size_t)n * sizeof(int));  /* never reached when n=0 */
```

The AI's mention of `malloc(0)` behavior is technically accurate in isolation
(C standard §7.22.3: implementation-defined whether it returns NULL or a
non-freeable pointer), but it is irrelevant here because `malloc` is never
called. The root cause is not implementation-defined: it is fully deterministic.
`NULL` is returned unconditionally for `n=0`, and the caller never checks it.

---

### AI claim 2 — Partially correct

> "The fix is to add a null check after `allocate_numbers` returns, before
> dereferencing `nums`."

**Verdict: Correct as a fix, but incomplete as an analysis.**

Adding a null check at line 31 would prevent the crash:

```c
nums = allocate_numbers(n);
if (!nums)           /* added check */
{
    fprintf(stderr, "allocation failed or n=0\n");
    return 1;
}
nums[0] = 42;
```

However, the AI framed this as a defensive coding improvement, implying the crash
is a possibility to guard against rather than a certainty. In this specific
program, with `n` hardcoded to `0`, the crash is **not probabilistic** — it is
guaranteed on every execution. The AI's framing obscures the deterministic nature
of the bug. A complete analysis must state that with `n=0`, `NULL` is always
returned, and the crash is unconditional.

---

### AI claim 3 — Incorrect

> "Valgrind would report this as an 'Invalid write of size 4' at the address
> returned by `malloc`."

**Verdict: Wrong on two counts.**

First, `malloc` is never called (see claim 1 above), so there is no address
"returned by malloc" involved.

Second, Valgrind would not report this crash at all in most cases. Valgrind's
Memcheck intercepts `malloc`/`free` and tracks heap block validity. A null
pointer dereference writing to address `0x0` — which is **never a heap block** —
falls outside the heap tracking model. Valgrind would instead report:

```
Invalid write of size 4
   at main (crash_example.c:32)
 Address 0x0 is not stack'd, malloc'd or (recently) free'd
```

The phrasing "not stack'd, malloc'd or recently free'd" is Valgrind's way of
indicating an access to an address it has no record of — i.e., a null or
completely invalid pointer, not a freed heap block. This is categorically
different from a use-after-free, which the AI's wording implied.

Furthermore, on many systems, the process crashes before Valgrind can complete
its report, since `SIGSEGV` is delivered synchronously by the hardware MMU.

---

## 6. Suggested Fix (labeled as such)

The crash has two independent root causes that should both be addressed:

**Root cause A — defensive gap in `main`:** The caller does not check the return
value of `allocate_numbers` before dereferencing it. Fix: add a null check.

**Root cause B — misleading call site:** `n` is hardcoded to `0`, which guarantees
`allocate_numbers` will return `NULL`. This means the program is structurally
broken by design. Fix: either use a meaningful value of `n`, or propagate an error
to the caller.

```c
/* Suggested fix — main only */
int main(void)
{
    int *nums = NULL;
    int n = 5;           /* use a valid size */

    printf("crash_example: deterministic NULL dereference (segmentation fault)\n");
    printf("  requesting n=%d\n", n);

    nums = allocate_numbers(n);
    if (!nums)                          /* added null check */
    {
        fprintf(stderr, "  allocation failed\n");
        return 1;
    }

    nums[0] = 42;
    printf("  nums[0]=%d\n", nums[0]);

    free(nums);
    return 0;
}
```

The fix does not modify `allocate_numbers`, which is correctly written.
The bug is entirely in `main`.

---

## Summary

| Field              | Value                                                  |
|--------------------|--------------------------------------------------------|
| Signal             | SIGSEGV (signal 11)                                    |
| Faulting address   | 0x0000000000000000                                     |
| Faulting instruction | `movl $0x2a, (%rax)` at offset 0x128a in `main`     |
| Source line        | `crash_example.c:32` — `nums[0] = 42`                 |
| UB category        | Null pointer dereference (write)                       |
| Memory region      | Neither stack nor heap — unmapped virtual address      |
| Root cause         | `n=0` → `allocate_numbers` returns NULL → null check missing in `main` |
| Determinism        | Fully deterministic — crashes on every execution       |
