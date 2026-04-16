# NASA / JPL Safety Rules — Full Reference

Source: Holzmann, G.J., "The Power of 10: Rules for Developing Safety-Critical Code,"
IEEE Computer, vol. 39, no. 6, pp. 95-99, 2006.
Augmented with: JPL Institutional Coding Standard, D-60411, March 3, 2009.
Further extended for the Nirapod embedded crypto context.

---

## The Power of 10 — Original Rules With Nirapod Extensions

### Rule 1 — Restrict to Simple Control Flow

**Original:** Do not use `goto`, `setjmp`, `longjmp`, or direct/indirect recursion.

**Nirapod extension:** This is about proving the control graph. If the compiler
cannot construct a finite, acyclic control-flow graph (excepting the main task
loop), the code is too complex to be safe. Every call stack must be provably bounded.

In practice this means: every function call you write, imagine drawing an arrow
from caller to callee. That graph must have no cycles — ever. The moment you see
recursion (even mutual recursion across two files), stop. Rewrite with an explicit
stack allocated at initialization time.

For state machines, use a `switch` statement on a typed `enum class` — never a
function pointer table as the primary dispatch mechanism (function pointer tables
break static analyzers). It is fine to have a function pointer table as a
secondary dispatch inside a `switch` case, but the outer structure must be a
plain control-flow construct.

### Rule 2 — All Loops Have a Fixed Upper Bound

**Original:** It must be trivially possible for a checking tool to prove statically
that a loop cannot exceed a preset upper bound. If a tool cannot prove the loop
bound, the rule is violated.

**How to comply:** For every loop, write the maximum iteration count as a named
constant immediately above the loop, then add an assert inside the loop:

```c
/* Maximum iterations for BLE packet reassembly: one full MTU split into
 * 20-byte fragments = 27 fragments maximum (BLE 5.0 max MTU 512 / 20). */
static const uint32_t MAX_FRAGS = 27U;
uint32_t frag_count = 0U;
while (!reassembly_complete(&ctx)) {
    NIRAPOD_ASSERT(frag_count < MAX_FRAGS);   /* loop bound enforced */
    process_fragment(&ctx, next_fragment());
    frag_count++;
}
```

The assert is not just a debug tool — it is a formal statement to the analyzer
that the loop variable is bounded. Without it, tools like PC-lint, Polyspace, and
Coverity cannot prove termination.

**Nirapod extension:** Loops that iterate over hardware registers (polling a
peripheral status bit) MUST have a timeout count, and that count must be based on
a worst-case hardware spec from the datasheet. Polling forever is a Rule 2
violation. After the timeout, the function returns an error — it does not spin.

```c
/* CC310 datasheet: CRYPTOCELL initialization completes within 10 ms at 64 MHz.
 * At the maximum polling rate (one read per ~4 cycles ≈ 16 MHz effective),
 * 10 ms yields at most 160,000 iterations. We use 200,000 as a conservative margin. */
static const uint32_t CC310_INIT_POLL_MAX = 200000U;
uint32_t poll_count = 0U;
while (!NRF_CRYPTOCELL->ENABLE) {
    NIRAPOD_ASSERT(poll_count < CC310_INIT_POLL_MAX);
    poll_count++;
}
```

### Rule 3 — No Dynamic Memory Allocation After Initialization

**Original:** Do not use heap allocation after task initialization.

**Why it matters for crypto:** Heap fragmentation on a constrained MCU can cause
an allocation to fail at a point where returning an error is catastrophic (e.g.,
mid-transaction on a hardware wallet). Static allocation eliminates this failure
mode entirely. The memory map is fixed and verifiable before the first packet is
processed.

**Nirapod implementation:** The `new` and `delete` operators and the `malloc`/`free`
family are replaced globally in `platform/memory_policy.cpp` with stubs that call
`NIRAPOD_ASSERT(false)`. This ensures any accidental heap usage fails loudly at
test time, not silently on the device.

Allowed memory regions:
- `.data` and `.bss` — statically allocated globals and static locals.
- Stack — function locals and RTOS task stacks (stack sizes are fixed in Kconfig).
- Static memory pools — initialized once during the `INIT` phase, never grown.

Prohibited patterns:
- `std::vector`, `std::string`, `std::deque`, `std::map` with default allocators.
- Any Zephyr `k_malloc` or `k_heap_alloc` outside the initialization context.
- Any stack allocation larger than 512 bytes (use a static buffer or pool instead).

### Rule 4 — Functions Fit on One Printed Page (≤ 60 Lines of Code)

**Original:** No function should be longer than what can be printed on a single
sheet of paper in a standard format with one line per statement.

**Nirapod interpretation:** 60 lines of code, not counting blank lines and comment
lines. This number is not arbitrary. Beyond ~60 lines, the probability of an
undetected off-by-one error roughly doubles per additional 10 lines (Hatton, 1995).
More importantly, a function longer than 60 lines almost always has more than one
clear responsibility, which means it should be two functions.

The discipline this enforces is: before you add line 61, you stop and ask "which
part of this function can I extract into a helper?" The answer is always the same —
there is always a natural decomposition. The 60-line rule is the forcing function
that makes you find it.

### Rule 5 — Assert a Minimum of Two Assertions Per Function

**Original:** The average density of assertions should be a minimum of two per
function, and no assertion should have side effects.

**Nirapod `NIRAPOD_ASSERT` semantics:** On device targets, `NIRAPOD_ASSERT(expr)`
evaluates `expr`, and if false, logs the failing expression, function name, and
line number to the fault log in retained RAM, then calls `k_oops()` (Zephyr) or
equivalent platform halt. On host test builds, it calls `abort()`.

Assertions are ALWAYS compiled in. The `NDEBUG` macro that silences `stdlib assert`
is never set on Nirapod builds. The overhead of an assertion check is one compare
and a predicted-not-taken branch — negligible in the context of a hardware crypto
operation that takes 2–20 ms.

**What to assert:**
- Pre-conditions: pointer non-NULL, length in valid range, state machine in expected state.
- Post-conditions: output buffer populated, length matches expected, error code set.
- Loop invariants (as described in Rule 2 above).
- Enum value ranges after a cast: `NIRAPOD_ASSERT(backend >= CryptoBackend::CC310_HW && backend <= CryptoBackend::MBEDTLS_SW)`.

**What NOT to assert:**
- Conditions that can legitimately be false in production (use `if` and return an error).
- Assertions with side effects: `NIRAPOD_ASSERT(init_hardware())` is wrong because
  `init_hardware()` might be optimized out if assertions were ever disabled.

### Rule 6 — Declare Variables at the Innermost Scope

**Original:** Data objects must be declared at the smallest possible level of scope.

**Why:** A variable visible for 200 lines when it is only used for 3 is a variable
that could be incorrectly modified by any of the 197 lines in between. Narrow scope
is a mechanical way to limit the blast radius of a bug.

In C++, this is natural: declare the variable where you initialize it. In C
(required for some Nordic SDK contexts), declare at the top of the innermost
block where it is used, not at the top of the function.

`const` is the right default. If a value doesn't change after initialization,
make it `const`. If it is a compile-time constant, make it `constexpr`. If a
`const` variable is being modified, that's a warning sign that the design is wrong.

### Rule 7 — Check Every Return Value — No Exceptions

**Original:** The return value of non-void functions must be used by each calling
function. If the return value is not relevant, the cast `(void)f()` is used to
make it explicit.

**Nirapod extension:** In security-critical code, ignoring a return value is
almost always a bug. An initialization function that returns an error but whose
return value was discarded means the system silently operates in an uninitialized
state. A crypto function whose error return was ignored means plaintext data
might be flowing out unencrypted.

The `[[nodiscard]]` attribute is applied to every function in the Nirapod public
API. The GCC flag `-Wunused-result` promotes this to a compile error. Every
intentional discard is documented:

```cpp
/* Release the mutex — errors here are non-actionable because the transaction
 * has already completed. The mutex will be released on the next timeout cycle. */
(void)os_mutex_unlock(&g_aes_hw_mutex);
```

### Rule 8 — Use the Preprocessor Minimally

**Original:** The preprocessor must be used only for including header files and
simple macros. Do not use conditional compilation to hide code from the compiler.

**Nirapod extension:** The two legitimate uses of macros are:
1. Platform-detection `#ifdef NRF52840_XXAA` / `#ifdef CONFIG_NRF_CC310` guards
   that select which backend code compiles — these are unavoidable in embedded cross-target builds.
2. The `NIRAPOD_ASSERT`, `NIRAPOD_STATIC_ASSERT`, `LOG_DBG`, `LOG_ERR` macros
   that must capture `__FILE__` and `__LINE__` at the call site.

Everything else uses `constexpr` for constants and `inline` / template functions
for function-like macros. There are no token-paste macros. There are no macros
that hide type information.

### Rule 9 — Compile Clean — Zero Warnings At Maximum Warning Level

**Original:** All code should be compiled with all compiler warnings enabled at
the most pedantic setting. Code should compile with these enabled.

**Nirapod build flags (GCC / Clang):**
```cmake
target_compile_options(nirapod_crypto PRIVATE
    -Wall -Wextra -Werror -Wpedantic
    -Wshadow                  # local variable shadows outer scope
    -Wundef                   # undefined macro used in #if
    -Wformat=2                # format string vulnerabilities
    -Wformat-security         # format string with %n
    -Wcast-align              # unaligned pointer casts (dangerous on Cortex-M4)
    -Wcast-qual               # cast discards const/volatile
    -Wconversion              # implicit numeric conversions that may lose data
    -Wsign-conversion         # implicit signed/unsigned conversion
    -Wnull-dereference        # potential NULL pointer dereference
    -Wdouble-promotion        # float promoted to double (expensive on Cortex-M4F)
    -Wno-padded               # suppress padding warnings for packed structs only
)
```

Static analysis runs in CI via Cppcheck (`--enable=all --error-exitcode=1`) and
on every PR targeting main via the Zephyr west `twister` test harness with
the `qemu_cortex_m3` QEMU target.

### Rule 10 — Single Owner Per Shared Data Object

**Original:** Data objects should have a single owning task. Only the owner
should be able to modify the object. Other tasks read it through IPC queues or
mutexes.

**Nirapod RTOS model (Zephyr):** Each module owns its data. The AES Driver owns
`g_aes_ctx`. The Key Manager owns the key slot table. The BLE module owns the
packet queue. Cross-module data flow uses Zephyr message queues (`k_msgq`) or
read-only accessor functions protected by a mutex. No task reaches into another
task's static data directly. Callbacks between tasks are replaced by queue messages.

---

## JPL Extension — Concurrency Rules (Multi-Task RTOS)

These rules are from the JPL D-60411 document, section 4, adapted for Zephyr
on Nordic and ESP32 targets.

**No task directly calls code in another task's context.** Use `k_msgq_put` to
post a message and let the owning task process it on its own schedule.

**All IPC operations that can block have a timeout.** `k_msgq_put(&queue, &msg, K_MSEC(50))` —
never `K_FOREVER` in a device driver. A permanently blocked driver is
indistinguishable from a crashed driver. Use K_FOREVER only in the top-level
task loop that is the main event pump.

**Mutex lock order is fixed and documented.** If two mutexes must be acquired
together, the order is always the same everywhere in the codebase (documented
in `platform/locking_order.md`). Inverting the order is a deadlock.

**ISR code is minimal.** Interrupt service routines set a flag, post to a queue,
or signal a semaphore. They do not call nrf_crypto, do not call mbedTLS, and do
not call any function that might block. The processing happens in a task.
