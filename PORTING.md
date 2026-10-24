# Porting musl to Dunit OS (`x86_64-dunit`, static-first)

This fork ports musl to the **Green Tea Kernel's native ABI** — not a new CPU, a
new **OS**. The kernel takes no Linux syscall numbers and exports no Linux kernel
structs; musl's public headers stay as the portable C facade, and a thin Dunit
adaptation layer translates POSIX flags/`errno`/paths/`stat`/dirents/clocks/
process ops into Dunit contracts. See `DUNIT_OS_TECHNICAL_ROADMAP.md` §M6 in the
`dunit-os` repo.

## The ABI contract (source of truth)

The target ABI is **Dunit Userspace ABI v0**, generated and documented in the
`dunit-os` repo under `abi/`:

- `abi/include/dunit/syscall.h` — `SYS_<NAME>` numbers (NOT Linux numbers)
- `abi/include/dunit/errno.h` — `DUNIT_E<NAME>` (== POSIX errno magnitudes)
- `abi/include/dunit/rights.h` — handle capability bits
- `abi/README.md` — syscall calling convention, errno, process-entry stack
  (argc/argv/envp/**auxv**: AT_PAGESZ/AT_SECURE/AT_RANDOM), ELF contract
  (ELF64 LE `ET_EXEC`, static-first), capability query (`sys_abi_query`)

When this fork is used as `dunit-os/toolchains/dunit-musl`, those headers live at
`../../abi/include/dunit/`. The low-level syscall mechanism already matches:
musl's x86_64 `syscall` instruction convention (`rax`=number, `rdi/rsi/rdx/r10/
r8/r9`=args, `rax`=result) is exactly Dunit's. The port's work is the NUMBERS and
the SEMANTICS, not the instruction.

A reference crt0 that already parses the Dunit initial stack + auxv lives at
`dunit-os/userspace/ctests/crt0.s` — the seed for this port's `crt1`.

## Workflow — thematic commits

Carry the Dunit delta as small, reviewable, themed commits (roadmap M6.2):

| area | scope |
|---|---|
| `build`   | `x86_64-dunit` target, `configure`/`Makefile` wiring, sysroot layout |
| `crt`     | `crt1`/`Scrt1`/`crti`/`crtn` for the Dunit initial stack + auxv |
| `syscall` | Dunit syscall numbers + the `__syscall` thin layer to Dunit's native set |
| `fs`      | `open`/`stat`/dirents/paths → Dunit VFS contracts |
| `vm`      | `mmap`/`mprotect`/`brk` → Dunit VM syscalls |
| `thread`  | TLS (Variant II, already in the ELF/kernel), `clone`/futex → Dunit |
| `signal`  | signal surface (stub to documented `ENOSYS` until the kernel grows it) |
| `ldso`    | deferred — static-first; no dynamic linker yet |

## Rules (roadmap M6.1)

- No fake success. An unimplemented function returns a **documented** `ENOSYS`/
  `ENOTSUP` or is excluded from the declared profile — never a silent fake OK.
- Don't delete temporarily-unsupported subsystems from the tree; exclude them from
  the build profile so they can be restored without re-fetching upstream.
- `x86_64-dunit` is the logical target name — never pass a Dunit binary off as
  `x86_64-linux-musl`.

## Build & test (as the port lands)

Build produces a static `libc.a` for `x86_64-dunit`, installed into the Dunit
sysroot. Conformance is driven from the `dunit-os` repo via `tools/qemu_test.py`:
a static-musl `hello`/`file`/`malloc`/`thread` suite runs on the kernel and is
checked by serial markers (the M6 acceptance target). The existing freestanding
`c_hello` (`dunit-os/userspace/ctests/`) is the no-libc baseline this builds on.

## Current static profile

The port provides native startup/argv/env/static TLS, errno, scalar/buffered
I/O, allocation, files/directories/seek/stat/fsync, clocks/sleep, a pthread
subset, pipes/dup, and direct spawn/wait. Unsupported calls remain in the full
upstream tree and return documented errors. This is not a complete POSIX OS.

- Open translates access/create/truncate/append/exclusive/nonblocking/CLOEXEC
  flags; unsupported semantics fail explicitly. Fcntl=86 translates descriptor
  flags separately from shared open-description status and duplicates.
- Poll=87 translates bounded native readiness records and absolute monotonic
  deadlines; it waits in kernel queues. select delegates to poll with explicit
  overflow/unsupported-event errors; signal-mask/ppoll support is not claimed.
- Ioctl=88 uses typed, length-checked Dunit operations for terminal geometry,
  pending input and probing. Internal musl ioctl calls use the same adapter;
  Linux requests/termios records do not enter the kernel. Canonical input,
  echo, process groups, baud rates and signal generation are unsupported.
- system(NULL) reports no available command processor; system(command)/popen
  explicitly return ENOSYS until shell policy and signal semantics are defined.
  Architecture backends exclude, but retain, the complete upstream sources.
- Posix_spawn serializes argv/env and ordered open/close/dup2/chdir actions.
  Bit 63 of bounded block length opts into the extension; old calls ignore
  register six. The kernel prepares a private child fd table and ELF before
  publishing it; inherited native capabilities are not granted. Unsupported
  attributes/fchdir fail before creating a child. PATH search is userspace,
  excludes empty/relative components, and has no compiled implicit cwd.
- Dunit I/O chunks allow POSIX short transfers. Blocking EAGAIN waits for
  readiness; O_NONBLOCK returns EAGAIN. FILE refill preserves scanf/ungetc
  buffer invariants. tmpfile uses the path adapter and unlink retains an open
  file object until close. Snapshot directories support seek/rewind cookies;
  overflow is explicit, dirfd remains unsupported.
- Barrier waits route through the native futex dispatcher. pthread_cancel and
  asynchronous cancellation return ENOTSUP without side effects; signals,
  interruption/cancellation points and dynamic TLS remain separate contracts.
- CLOCK_MONOTONIC is elapsed time. CLOCK_REALTIME uses a valid bootloader UTC
  timestamp plus that timebase; a missing timestamp returns an error.
- Static private mmap includes eager file snapshots with explicit EOF bounds;
  shared/COW/demand mappings are not claimed. W+X remains forbidden.

The Dunit OS repository supplies the QEMU marker contracts and pinned upstream
libc-test subset. Its acceptance record is the source of execution evidence;
code presence and building libc are not runtime proof. Signal/interruption,
secure entropy, dynamic linking, fd-backed directory iteration, POSIX ownership
and network objects are outside this declared static subset. F_OK is a real
Stat-based existence check; R/W/X permission queries explicitly fail until the
native permission model can answer them.

## Sysroot isolation and reproducibility

`tools/dunit/build-libc.sh` installs crt/libc/headers and relocatable compiler
wrappers. Headers use -nostdinc; explicit host include roots are refused;
ambient include/library environment is cleared. Linking uses --sysroot and the
selected target lib directory before user -l options (including musl's empty
libm archive), so a host libm cannot be linked accidentally. Compiler support
libgcc is an explicit input. The OS build fingerprints actual source/compiler
inputs, regenerates SDK CMake/Meson/Autoconf cross files and provenance hashes,
and rejects ELF files outside its static profile before packing the initrd.

DELTA.md is regenerated from committed history with tools/dunit/gen-delta.sh.
