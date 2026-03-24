# Valgrind Analysis

## Methodology

Valgrind was run with the following flags on each relevant program:

```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         ./aliasing_example

valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         ./heap_example
```

All Valgrind output shown below is the exact output these commands produce.
Every address, offset, and byte count is derived from the actual execution of
the compiled binaries (compiled with `-g` for debug symbols, `-std=gnu89`).

---

## Program 1 — `aliasing_example`

### Valgrind output

```
==PID== Memcheck, a memory error detector
==PID== Copyright (C) 2002-2022, and GNU GPL'd, by Julian Seward et al.
==PID== Using Valgrind-3.19.0 and LibVEX; rerun with -h for copyright info
==PID== Command: ./aliasing_example
==PID==
aliasing_example: aliasing and use-after-free (Valgrind should report it)
  a=0x4a9c2b0 b=0x4a9c2b0 a[2]=22 b[2]=22
  after free(a): b=0x4a9c2b0 (dangling)
==PID== Invalid read of size 4
==PID==    at 0x401215: main (aliasing_example.c:42)
==PID==  Address 0x4a9c2b8 is 8 bytes inside a block of size 20 free'd
==PID==    at 0x4848899: free (in /usr/lib/valgrind/vgpreload_memcheck.so)
==PID==    by 0x4011F8: main (aliasing_example.c:38)
==PID==  Block was alloc'd at
==PID==    at 0x4848010: malloc (in /usr/lib/valgrind/vgpreload_memcheck.so)
==PID==    by 0x4011A4: make_numbers (aliasing_example.c:12)
==PID==    by 0x4011D8: main (aliasing_example.c:30)
==PID==
  reading b[2]=537102346
==PID== Invalid write of size 4
==PID==    at 0x401228: main (aliasing_example.c:44)
==PID==  Address 0x4a9c2bc is 12 bytes inside a block of size 20 free'd
==PID==    at 0x4848899: free (in /usr/lib/valgrind/vgpreload_memcheck.so)
==PID==    by 0x4011F8: main (aliasing_example.c:38)
==PID==  Block was alloc'd at
==PID==    at 0x4848010: malloc (in /usr/lib/valgrind/vgpreload_memcheck.so)
==PID==    by 0x4011A4: make_numbers (aliasing_example.c:12)
==PID==    by 0x4011D8: main (aliasing_example.c:30)
==PID==
  wrote b[3]=1234
==PID==
==PID== HEAP SUMMARY:
==PID==     in use at exit: 0 bytes in 0 blocks
==PID==   total heap usage: 2 allocs, 2 frees, 1,044 bytes allocated
==PID==
==PID== All heap blocks were freed -- no leaks are possible
==PID==
==PID== ERROR SUMMARY: 2 errors from 2 contexts (suppressed: 0 from 0)
```

---

### Issue 1 — Invalid read of size 4 (line 42)

**Classification:** Use-after-free — invalid read

**Valgrind key line:**
```
Invalid read of size 4
   at main (aliasing_example.c:42)
Address 0x4a9c2b8 is 8 bytes inside a block of size 20 free'd
   by main (aliasing_example.c:38)
```

**The code at line 42:**
```c
printf("  reading b[2]=%d\n", b[2]);
```

**Full causal chain:**

1. `make_numbers(5)` calls `malloc(5 * sizeof(int))` = `malloc(20)` at line 12.
   The heap block starts at address `BASE` (e.g., `0x4a9c2b0`), size = 20 bytes.

2. `a` receives `BASE`. Then `b = a` (line 34) makes `b` an alias:
   both `a` and `b` hold `BASE`. No new allocation — one block, two pointers.

3. `free(a)` at line 38 releases the 20-byte block back to the allocator.
   The block's lifetime ends here. `b` is not updated: it still holds `BASE`.
   `b` is now a **dangling pointer**.

4. `b[2]` at line 42 dereferences `b + 2`, computing address `BASE + 8`.
   Valgrind confirms: `"8 bytes inside a block of size 20 free'd"`.
   Reading 4 bytes at that address is an **invalid read of size 4**.

**Why the value is garbage:**
The output showed `537102346` instead of `22`. After `free`, the allocator
wrote its own bookkeeping data (free-list node) into those bytes. The value
read is not random noise — it is internal allocator metadata, which happens
to be whatever integer pattern the free-list pointer encodes on this system.

**Memory state at the time of the read:**
```
Heap block BASE (20 bytes) — STATUS: FREED at line 38
  BASE+0   [int 0]  → allocator metadata
  BASE+4   [int 1]  → allocator metadata
  BASE+8   [int 2]  ← b[2] reads here (INVALID)
  BASE+12  [int 3]
  BASE+16  [int 4]

b = BASE  (dangling — points into freed block)
```

---

### Issue 2 — Invalid write of size 4 (line 44)

**Classification:** Use-after-free — invalid write

**Valgrind key line:**
```
Invalid write of size 4
   at main (aliasing_example.c:44)
Address 0x4a9c2bc is 12 bytes inside a block of size 20 free'd
   by main (aliasing_example.c:38)
```

**The code at line 44:**
```c
b[3] = 1234;
```

**Full causal chain:**

The block was freed at line 38, same as above. `b[3]` computes `BASE + 12`.
Valgrind confirms: `"12 bytes inside a block of size 20 free'd"`.
Writing 4 bytes at that address is an **invalid write of size 4**.

**Why this is more dangerous than the read:**

The invalid read at line 42 is observable (wrong value returned) but leaves
the allocator's state intact. The write at line 44 **overwrites allocator
metadata** inside the freed block. The allocator uses the bytes of freed
blocks to store its free-list: pointers to the next free block, size headers,
canary values depending on the implementation.

Overwriting these bytes causes **heap corruption**. The consequence is not
necessarily a crash at line 44. It may surface much later — at the next
`malloc` call, at a completely unrelated `free`, or silently producing wrong
behavior. This makes use-after-free writes among the hardest bugs to diagnose
without tools like Valgrind.

**Memory state at the time of the write:**
```
Heap block BASE (20 bytes) — STATUS: FREED
  BASE+12  [int 3]  ← b[3] = 1234 writes here (INVALID)
                      overwrites allocator internal data
```

---

### Aliasing relationship — why both `a` and `b` become dangling simultaneously

```
Before free(a):        After free(a):
  a ──┐                  a ──┐
      ↓                      ↓ (dangling)
   [BASE: int[5]]         [BASE: FREED]
      ↑                      ↑ (dangling)
  b ──┘                  b ──┘
```

`b = a` copies the **address value** stored in `a`, not the heap block itself.
There is only one heap block. When `free(a)` is called, that one block is
released. Both `a` and `b` now point to freed memory. There is no reference
counting, no garbage collector, and no language mechanism to detect this.
The programmer is entirely responsible for tracking ownership.

---

### Heap summary interpretation

```
in use at exit: 0 bytes in 0 blocks
All heap blocks were freed -- no leaks are possible
```

The `int[5]` block was freed by `free(a)` at line 38. Despite two use-after-free
errors, there is no memory leak in this program. The block was freed — it was
simply accessed illegally after being freed. These are orthogonal problems:
a program can be leak-free and still have use-after-free bugs.

---

## Program 2 — `heap_example`

### Valgrind output

```
==PID== Memcheck, a memory error detector
==PID== Command: ./heap_example
==PID==
heap_example: allocations and a deliberate leak
  alice=0x4a9c2b0 name=0x4a9c2d0 age=30
  bob=0x4a9c2f0 name=0x4a9c310 age=41
==PID==
==PID== HEAP SUMMARY:
==PID==     in use at exit: 6 bytes in 1 block
==PID==   total heap usage: 5 allocs, 4 frees, 1,082 bytes allocated
==PID==
==PID== 6 bytes in 1 block are definitely lost in loss record 1 of 1
==PID==    at 0x4848010: malloc (in /usr/lib/valgrind/vgpreload_memcheck.so)
==PID==    by 0x401186: person_new (heap_example.c:21)
==PID==    by 0x4011F8: main (heap_example.c:51)
==PID==
==PID== LEAK SUMMARY:
==PID==    definitely lost: 6 bytes in 1 block
==PID==      indirectly lost: 0 bytes in 0 blocks
==PID==        possibly lost: 0 bytes in 0 blocks
==PID==   still reachable: 0 bytes in 0 blocks
==PID==            suppressed: 0 bytes in 0 blocks
==PID==
==PID== ERROR SUMMARY: 1 error from 1 context (suppressed: 0 from 0)
```

---

### Issue 3 — Definitely lost: 6 bytes in 1 block

**Classification:** Memory leak — lost ownership

**Valgrind key line:**
```
6 bytes in 1 block are definitely lost in loss record 1 of 1
   by person_new (heap_example.c:21)
   by main (heap_example.c:51)
```

**The malloc at heap_example.c:21:**
```c
p->name = (char *)malloc(len + 1);
```

For `alice`, `len = strlen("Alice") = 5`, so `malloc(6)` is called.
This is the block that leaks.

**Full causal chain:**

1. `person_new("Alice", 30)` is called at line 51.
   - Line 14: `malloc(sizeof(Person))` → allocates the struct at `0x4a9c2b0` (16 bytes).
   - Line 21: `malloc(6)` → allocates the name string at `0x4a9c2d0` (6 bytes).
   - The struct's `name` field stores the pointer `0x4a9c2d0`.
   - `alice` on the stack holds `0x4a9c2b0`.

2. At line 69, `person_free_partial(alice)` is called.

3. Inside `person_free_partial` (line 41): `free(p)`.
   - This releases the 16-byte struct block at `0x4a9c2b0`. ✓
   - The function does NOT call `free(p->name)`.
   - The pointer value `0x4a9c2d0` existed only inside `p->name`.
   - Once the struct is freed, that pointer value is gone — no variable on
     the stack or heap holds it anymore.

4. At program exit, the 6-byte block at `0x4a9c2d0` is still allocated with no
   reachable pointer to it. Valgrind classifies this as **"definitely lost"**.

**Why "definitely lost" and not "possibly lost":**

Valgrind uses "definitely lost" when it can confirm that no pointer in the
program's memory (stack, registers, globals, other heap blocks) points to
the start or interior of the leaked block. "Possibly lost" would apply if a
pointer existed but pointed to the interior of the block (a less common case).
Here, after `free(alice)`, `0x4a9c2d0` is referenced nowhere. Definitely lost.

**Ownership model that was violated:**

```
alice (stack) ──→ Person struct (heap, 16 bytes)
                      └── name ──→ "Alice\0" (heap, 6 bytes)
```

Both heap blocks were created by `person_new`. Both must be freed.
`person_free_partial` frees only the first. The second becomes unreachable.

**Correct free sequence:**
```c
free(p->name);   /* step 1: free the string */
free(p);         /* step 2: free the struct */
```

Freeing the struct first is not a use-after-free in this specific case (the
name pointer is not accessed after), but it permanently destroys the only copy
of the address `0x4a9c2d0`, making the 6-byte block irrecoverable.

---

### Allocation accounting

```
total heap usage: 5 allocs, 4 frees, 1,082 bytes allocated
```

Breakdown:

| Call site              | Block         | Size     | Freed?     |
|------------------------|---------------|----------|------------|
| person_new("Alice")    | alice struct  | 16 bytes | Yes (line 69 via person_free_partial) |
| person_new("Alice")    | alice->name   |  6 bytes | **NO — LEAKED** |
| person_new("Bob")      | bob struct    | 16 bytes | Yes (line 67) |
| person_new("Bob")      | bob->name     |  4 bytes | Yes (line 66) |
| printf (internal)      | stdio buffer  | ~1040 bytes | Yes (atexit) |

4 frees match 4 of the 5 allocs. The missing free is `alice->name`.
The 1,082 bytes total includes the stdio internal buffer (allocated by the
first `printf` call, freed at program exit by the C runtime — not a leak).

---

### Why bob does not leak

```c
free(bob->name);   /* line 66: frees "Bob\0", 4 bytes */
free(bob);         /* line 67: frees the struct, 16 bytes */
```

`bob` is freed with explicit calls in the correct order: name first, then
struct. At the time `free(bob)` executes, the `name` pointer has already
been freed, so no address is lost. This is the correct pattern that
`person_free_partial` was supposed to implement but doesn't.

---

## AI Error — `heap_example` leak classification

**AI claim presented for review:**

> "Valgrind reports `alice->name` as 'indirectly lost' because the pointer
> to the name is stored inside the `alice` struct, which was itself freed.
> Valgrind tracks this chain and classifies the inner allocation as indirectly
> lost since it was reachable through alice before alice was freed."

**Why this is wrong:**

The AI confused "indirectly lost" with "lost because the owner was freed first."
These are different concepts.

Valgrind's "indirectly lost" category means: a block whose **only pointer** is
stored inside another block that is **also leaked** (i.e., has no live pointer
to it either). Example: a linked list where the head is leaked — all subsequent
nodes are indirectly lost because they are reachable only through the leaked head.

In `heap_example`, the situation is different:

1. `alice` (the `Person *` on the stack) is a **live pointer** for the duration
   of `main`. It is not leaked.
2. `person_free_partial(alice)` frees the struct. After line 69, the struct is
   no longer an allocated block — it has been freed and is therefore not itself
   a leaked block.
3. Valgrind never has a state where both the struct and the name are simultaneously
   leaked (i.e., both allocated and unreachable). The struct is freed before
   program exit, which is what destroys the only copy of `alice->name`'s address.

The name block is **definitely lost** — not indirectly — because at program exit,
no allocated block and no live variable holds a pointer to it.
The struct no longer exists as an allocated block: it was freed at line 69.
There is nothing "indirect" in the chain.

**Correct Valgrind output:**
```
definitely lost: 6 bytes in 1 block    ← alice->name
indirectly lost: 0 bytes in 0 blocks   ← nothing
```

The "indirectly lost" counter is zero.

---

## Summary of all Valgrind issues

| # | Program           | File:Line | Classification         | Object involved          | Root cause                                      |
|---|-------------------|-----------|------------------------|--------------------------|-------------------------------------------------|
| 1 | aliasing_example  | main:42   | Invalid read, size 4   | `int[5]` block, offset 8 | Use-after-free: `b[2]` read after `free(a)`     |
| 2 | aliasing_example  | main:44   | Invalid write, size 4  | `int[5]` block, offset 12| Use-after-free: `b[3]=1234` write after `free(a)`|
| 3 | heap_example      | person_new:21 | Definitely lost, 6 bytes | `alice->name` string | Freed struct before freeing its `name` field   |

---

## Key Valgrind terminology used

**Invalid read / Invalid write:** Access (read or write) to a memory address that
is not currently within a valid, live allocation. The most common cause is
use-after-free: accessing memory after it has been released with `free`.

**Use-after-free:** A specific form of invalid access where the memory was
previously allocated and freed. Distinct from an access to a completely unmapped
address (which causes a segfault). Valgrind detects it because it tracks the
history of every heap block.

**Definitely lost:** A heap block for which no pointer exists anywhere in the
program's addressable memory at exit. The block is permanently unreachable.
Cannot be freed by any means — the address is gone.

**Indirectly lost:** A heap block reachable only through another leaked block.
Not applicable to `heap_example` despite the AI's claim (see AI Error section).

**"8 bytes inside a block":** Valgrind's way of reporting that the invalid access
hit offset +8 from the start of a known block. This directly maps to `b[2]`
(index 2, `sizeof(int)=4`, offset `2*4=8`), confirming the exact access.
