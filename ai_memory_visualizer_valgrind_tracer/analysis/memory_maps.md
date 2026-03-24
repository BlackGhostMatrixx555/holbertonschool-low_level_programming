# Memory Maps — AI Memory Visualizer

## Overview

This document analyzes three C programs: `stack_example.c`, `aliasing_example.c`,
and `heap_example.c`. For each program, it provides step-by-step memory maps at key
execution points, distinguishes stack from heap memory, tracks pointer lifetimes and
aliasing, and documents one case where an AI-generated explanation was incorrect and
how it was corrected.

All addresses shown are real values captured from a single execution run.

---

## 1. `stack_example.c` — Recursive Stack Frames

### Program summary

`main` calls `walk_stack(0, 3)`, which calls itself recursively until `depth == 3`.
At each level, `dump_frame` is called on entry and exit. The goal is to observe how
the stack grows downward and how each frame is destroyed on return.

---

### Key execution points

#### Point A — Entry of `walk_stack(depth=0)`

A new stack frame is pushed for `walk_stack`. Then `dump_frame` is called, which
pushes its own frame on top.

```
HIGH ADDRESSES
┌─────────────────────────────────────────────┐
│  main frame                                 │
│    (no locals relevant here)                │
├─────────────────────────────────────────────┤
│  walk_stack(depth=0, max_depth=3) frame     │  ← pushed first
│    int marker = 0          @ 0x7ef28aea5a24 │
├─────────────────────────────────────────────┤
│  dump_frame("enter", depth=0) frame         │  ← pushed on top
│    int local_int = 100     @ 0x7ef28aea59d4 │
│    char local_buf[16]      @ 0x7ef28aea59e0 │  local_buf[0]='A'
│    int *p_local            @ (register/local)│  value = 0x7ef28aea59d4
└─────────────────────────────────────────────┘
LOW ADDRESSES  (stack grows downward →)
```

`p_local` holds the address of `local_int` within the **same frame**. Both live and
die together. No aliasing issue here.

---

#### Point B — Entry of `walk_stack(depth=1)` (first recursive call)

A second `walk_stack` frame is pushed below the first (lower address).

```
HIGH ADDRESSES
┌────────────────────────────────────────────────┐
│  main frame                                    │
├────────────────────────────────────────────────┤
│  walk_stack(depth=0) frame                     │
│    int marker = 0           @ 0x7ef28aea5a24   │  ← still alive
├────────────────────────────────────────────────┤
│  walk_stack(depth=1) frame                     │  ← new frame
│    int marker = 10          @ 0x7ef28aea59f4   │
├────────────────────────────────────────────────┤
│  dump_frame("enter", depth=1) frame            │
│    int local_int = 101      @ 0x7ef28aea59a4   │
│    char local_buf[16]       @ 0x7ef28aea59b0   │  local_buf[0]='B'
│    int *p_local             = 0x7ef28aea59a4   │
└────────────────────────────────────────────────┘
LOW ADDRESSES
```

Address difference between `marker` at depth=0 and depth=1:
`0x7ef28aea5a24 - 0x7ef28aea59f4 = 0x30 = 48 bytes` per `walk_stack` frame.

---

#### Point C — Maximum depth: `walk_stack(depth=3)`

All four `walk_stack` frames are simultaneously alive on the stack.

```
HIGH ADDRESSES
┌──────────────────────────────────────────────────┐
│  main frame                                      │
├──────────────────────────────────────────────────┤
│  walk_stack(depth=0)   marker @ 0x7ef28aea5a24   │
├──────────────────────────────────────────────────┤
│  walk_stack(depth=1)   marker @ 0x7ef28aea59f4   │
├──────────────────────────────────────────────────┤
│  walk_stack(depth=2)   marker @ 0x7ef28aea59c4   │
├──────────────────────────────────────────────────┤
│  walk_stack(depth=3)   marker @ 0x7ef28aea5994   │
├──────────────────────────────────────────────────┤
│  dump_frame("enter", depth=3)                    │
│    local_int = 103      @ 0x7ef28aea5944         │
│    local_buf[16]        @ 0x7ef28aea5950         │  local_buf[0]='D'
│    p_local              = 0x7ef28aea5944         │
└──────────────────────────────────────────────────┘
LOW ADDRESSES
```

---

#### Point D — Unwinding: return from `walk_stack(depth=3)`

When `walk_stack(depth=3)` returns, its frame is **popped**: the stack pointer moves
back up. The bytes at `0x7ef28aea5994` are no longer owned by any live variable.
They may still contain the value `30` physically in RAM, but accessing them would be
undefined behavior — the C standard gives no guarantee.

```
After return from walk_stack(depth=3):
  0x7ef28aea5994  → no longer a valid address for marker (frame destroyed)
  0x7ef28aea59c4  → walk_stack(depth=2).marker still alive, value=20 ✓
```

**Variable lifetime rule demonstrated here:**
A stack variable's lifetime begins when its enclosing function is entered and ends
when that function returns. There is no garbage collector; the memory is simply
reclaimed by the stack pointer moving.

---

### AI error documented — stack_example

**AI claim (generated during analysis):**

> "After `walk_stack(depth=3)` returns, the variable `marker` at address
> `0x7ef28aea5994` still holds the value `30` and can be safely read through a
> pointer if needed, because the stack memory has not been overwritten yet."

**Why this is wrong:**

This is the classic "it works by accident" fallacy. The statement confuses *physical
persistence* with *legal validity*.

1. The C standard (C89/C99) specifies that the lifetime of an automatic variable
   ends when its enclosing block exits. After that point, any access through a
   pointer to that variable is **undefined behavior**, regardless of what value
   happens to be in RAM.
2. The memory is reclaimed by the runtime as soon as the function returns: the
   stack pointer is moved back. The next function call (even `printf`) will reuse
   that exact memory for its own locals, silently overwriting whatever was there.
3. On a real system with a signal handler, interrupt, or concurrent thread, the
   bytes can be overwritten between the moment the function returns and the moment
   you read through the dangling pointer.

**Correction:**

A pointer to a local variable becomes a **dangling pointer** the instant the
function returns. The fact that the value may still be readable in a toy program
(no other calls occurred) does not make the access valid. Valgrind would report
this as an **invalid read** and compilers with AddressSanitizer would abort the
program with a clear stack-use-after-return error.

---

## 2. `aliasing_example.c` — Pointer Aliasing and Use-After-Free

### Program summary

`make_numbers(5)` allocates an `int[5]` on the heap and returns a pointer `a`.
Then `b = a` creates an alias. After `free(a)`, both `a` and `b` are dangling
pointers. The program then reads and writes through `b`, which is undefined behavior.

---

### Key execution points

#### Point A — After `a = make_numbers(5)`

```
STACK (main frame)
┌────────────────────────────────────────────┐
│  int *a  = 0x562948d782b0                  │  pointer on stack
│  int *b  = NULL                            │
│  int  n  = 5                               │
└────────────────────────────────────────────┘

HEAP
┌────────────────────────────────────────────┐
│  [0x562948d782b0]  int[5]                  │  20 bytes allocated
│   [0] = 0                                  │
│   [1] = 11                                 │
│   [2] = 22   ← a[2]                        │
│   [3] = 33                                 │
│   [4] = 44                                 │
└────────────────────────────────────────────┘

Owner of 0x562948d782b0: pointer `a` (sole owner)
```

---

#### Point B — After `b = a`

No new memory is allocated. `b` receives a **copy of the address** stored in `a`.
Both pointers now refer to the same heap block.

```
STACK (main frame)
┌────────────────────────────────────────────┐
│  int *a  = 0x562948d782b0  ─────┐          │
│  int *b  = 0x562948d782b0  ─────┤          │
└──────────────────────────────── │ ─────────┘
                                  │
HEAP                              ↓
┌────────────────────────────────────────────┐
│  [0x562948d782b0]  int[5]   ← both a and b │
│   [2] = 22                                 │
└────────────────────────────────────────────┘

Alias relationship: a == b (same address, same block, no copy)
```

This is confirmed by the program output:
`a=0x562948d782b0 b=0x562948d782b0 a[2]=22 b[2]=22`

---

#### Point C — After `free(a)`

`free(a)` releases the heap block back to the allocator. The block is now
**unowned**. The allocator may reuse it at any moment.

```
STACK (main frame)
┌────────────────────────────────────────────┐
│  int *a  = 0x562948d782b0  (DANGLING)      │  ← value unchanged, but invalid
│  int *b  = 0x562948d782b0  (DANGLING)      │  ← alias, equally invalid
└────────────────────────────────────────────┘

HEAP
┌────────────────────────────────────────────┐
│  [0x562948d782b0]  FREED — owned by malloc │
│   contents: UNDEFINED                      │  allocator may have modified them
└────────────────────────────────────────────┘
```

**Critical point:** `free(a)` does NOT set `a` to NULL and does NOT set `b` to
anything. Both pointers silently keep their old address value. The C standard
provides no mechanism to detect that a pointer has been freed.

---

#### Point D — `b[2]` read (use-after-free)

```c
printf("  reading b[2]=%d\n", b[2]);  /* line 42 */
```

The program printed: `reading b[2]=1405813855`

This is **undefined behavior**. The value `1405813855` is not `22` — the allocator
has already written its own bookkeeping data (free-list metadata) into that memory
region. This is exactly what Valgrind reports as an **invalid read of size 4**.

```
Valgrind would report:
  Invalid read of size 4
    at 0x... (main, aliasing_example.c:42)
  Address 0x562948d782b0 is 8 bytes inside a block of size 20 free'd
    at 0x... (free)
    by 0x... (main, aliasing_example.c:38)
```

---

#### Point E — `b[3] = 1234` (use-after-free write)

```c
b[3] = 1234;  /* line 44 */
```

This writes into freed memory. This is even more dangerous than a read: it silently
corrupts the allocator's internal free-list, which can cause a crash or security
vulnerability far later in the program, in a completely unrelated call to `malloc`
or `free`.

```
Valgrind would report:
  Invalid write of size 4
    at 0x... (main, aliasing_example.c:44)
  Address 0x562948d782b8 is 12 bytes inside a block of size 20 free'd
```

---

### Pointer lifetime summary

| Pointer | Born      | Valid until  | After `free(a)`   |
|---------|-----------|--------------|-------------------|
| `a`     | line 34   | line 38      | dangling          |
| `b`     | line 36   | line 38      | dangling (alias)  |
| heap block | malloc | free(a)     | unowned, invalid  |

---

## 3. `heap_example.c` — Struct Allocations and Deliberate Leak

### Program summary

`person_new` performs two `malloc` calls: one for the `Person` struct, one for the
`name` string. `person_free_partial` only frees the struct, not the name. `bob` is
freed correctly. `alice` is freed with `person_free_partial`, leaking `alice->name`.

---

### Key execution points

#### Point A — After `alice = person_new("Alice", 30)`

Two separate heap allocations are made:

```
HEAP
┌──────────────────────────────────────────────────────┐
│  [0x55b5567112b0]  Person struct (16 bytes)          │
│    name  = 0x55b5567112d0   (pointer to name string) │
│    age   = 30                                        │
├──────────────────────────────────────────────────────┤
│  [0x55b5567112d0]  char[6] = "Alice\0"               │
└──────────────────────────────────────────────────────┘

STACK (main)
  Person *alice = 0x55b5567112b0
```

Ownership chain: `alice` → `Person struct` → `name string`
Both heap blocks must be freed to avoid a leak.

---

#### Point B — After `bob = person_new("Bob", 41)`

Two more heap allocations, at different addresses:

```
HEAP
┌──────────────────────────────────────────────────────┐
│  [0x55b5567112b0]  alice Person struct               │
│    name = 0x55b5567112d0                             │
├──────────────────────────────────────────────────────┤
│  [0x55b5567112d0]  "Alice\0"                         │
├──────────────────────────────────────────────────────┤
│  [0x55b5567112f0]  bob Person struct (16 bytes)      │
│    name = 0x55b556711310                             │
│    age  = 41                                         │
├──────────────────────────────────────────────────────┤
│  [0x55b556711310]  char[4] = "Bob\0"                 │
└──────────────────────────────────────────────────────┘
```

No aliasing between `alice` and `bob`. Four independent heap blocks.

---

#### Point C — `free(bob->name)` then `free(bob)` (correct)

```
free(bob->name):  block at 0x55b556711310 → freed ✓
free(bob):        block at 0x55b5567112f0 → freed ✓
```

After these two calls, both of `bob`'s allocations are released. `bob` itself
(the pointer on the stack) still holds `0x55b5567112f0` but is dangling. It is not
used again, so no bug here.

---

#### Point D — `person_free_partial(alice)` (deliberate leak)

```c
static void person_free_partial(Person *p)
{
    if (!p)
        return;
    free(p);       /* frees the Person struct only */
                   /* p->name is NOT freed          */
}
```

```
free(alice):  block at 0x55b5567112b0 → freed
alice->name:  block at 0x55b5567112d0 → LEAKED (never freed)
```

The pointer `p->name` held the address `0x55b5567112d0`, but that value was only
accessible through the struct. Once the struct is freed, the address of the name
string is **lost forever**. There is no way to reach or free it.

```
HEAP state at program exit:
┌───────────────────────────────────────────┐
│  [0x55b5567112d0]  "Alice\0"  — LEAKED    │
│   6 bytes, never freed, unreachable       │
└───────────────────────────────────────────┘
```

Valgrind `--leak-check=full` would report:

```
LEAK SUMMARY:
   definitely lost: 6 bytes in 1 block
```

The 6 bytes correspond to `strlen("Alice") + 1 = 6`.

---

### AI error documented — heap_example

**AI claim (generated during analysis):**

> "In `person_free_partial`, freeing `p` also releases `p->name` because `p->name`
> is a field inside the struct, so it is part of the same allocation."

**Why this is wrong:**

This confuses a struct field with its value. `p->name` is a `char *` — it is a
pointer stored *inside* the struct. The struct occupies one heap block
(`sizeof(Person)` bytes). The string that `p->name` points to is a **completely
separate heap block**, allocated by a second `malloc` call inside `person_new`.

`free(p)` releases exactly the bytes of the struct itself (the `char *` and the
`int`). It does not follow the pointer stored in the `name` field. `free` has no
knowledge of what the bytes it is freeing represent. It only knows the address and
size of the block it was given.

**Memory layout that makes this concrete:**

```
free(p) releases:  [0x55b5567112b0 .. 0x55b5567112bf]  (the struct, 16 bytes)
NOT released:      [0x55b5567112d0 .. 0x55b5567112d5]  (the name string, 6 bytes)
```

These are two distinct regions of the heap. The string lives at a completely
different address. `free(p)` cannot and does not touch it.

**Correct mental model:**

Every `malloc` call requires exactly one matching `free` call on the same address.
When a struct contains pointer fields that were themselves dynamically allocated,
the programmer must free each one individually before (or instead of) freeing the
struct. The canonical correct free for this struct is:

```c
free(p->name);  /* free the string first */
free(p);        /* then free the struct  */
```

Freeing in reverse order of allocation is the safe pattern. Freeing the struct
first does not cause a crash in this specific case (the name pointer is not
accessed after) but it makes the name's address permanently unreachable, resulting
in a definite memory leak.

---

## Summary table

| Program          | Memory region | Bug type           | Valgrind category          |
|------------------|---------------|--------------------|----------------------------|
| stack_example    | Stack         | Dangling pointer   | Invalid read (if accessed) |
| aliasing_example | Heap          | Use-after-free     | Invalid read/write         |
| heap_example     | Heap          | Memory leak        | Definitely lost            |

---

## Key principles demonstrated

**Stack lifetime:** A local variable exists only for the duration of its enclosing
function call. The stack frame is created on function entry and destroyed on return.
Any pointer to a local variable becomes invalid the instant the function returns.

**Heap ownership:** Every `malloc` must be matched by exactly one `free`. When a
struct contains pointer fields pointing to separately allocated memory, each
allocation is an independent block that must be freed individually.

**Aliasing:** Assigning one pointer to another does not copy the pointed-to data.
Both pointers refer to the same block. Freeing through one pointer invalidates
all aliases simultaneously. C provides no language-level mechanism to detect this.

**Use-after-free:** Reading freed memory yields values written by the allocator
(e.g., free-list bookkeeping), not the original data. Writing to freed memory
silently corrupts the allocator's internal state and can cause crashes or
exploitable vulnerabilities far from the original bug site.
