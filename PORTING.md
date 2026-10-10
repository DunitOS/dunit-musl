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

## Status

**The port has started and runs on the kernel.** Done so far:

- `build:` — `x86_64-dunit` target recognised by `configure`; reproducible
  static-libc build via `tools/dunit/build-libc.sh` (host x86_64 compiler, since
  the syscall instruction/registers already match). Full `libc.a` + crt objects
  build cleanly.
- `syscall:` — `arch/x86_64/bits/syscall.h.in` regenerated from a documented
  Linux-name → Dunit-number map (`tools/dunit/gen_syscall_h.py`); unmapped calls
  become `-ENOSYS` sentinels (M6.1). `__set_thread_area.s` installs the thread
  pointer via the native `SetThreadPointer` (39).
- First milestone verified end-to-end: a static `musl_hello` (musl `crt1` +
  `__libc_start_main` + `__init_tls` + `write()`) runs on the Green Tea Kernel
  via `dunit-os/tools/qemu_test.py` and exits 0.
- `fs:` (write path) — `writev`/`__stdio_write` loop over the native `Write`, so
  buffered stdio (`printf`/`fflush`) works. Verified with a static printf test.
- `vm:` — `mmap`/`munmap`/`mprotect` map 1:1 onto the native Dunit VM syscalls
  (prot and `MAP_PRIVATE|MAP_ANONYMOUS` flag values already match). Verified with
  a static malloc/calloc/1 MiB-mmap test.
- `fs:` (files) — `open` goes through `src/dunit/open.c`: the `__sys_open*` macros
  pass the path as pointer+length and translate POSIX flags to Dunit `OpenFlags`
  (the kernel's `Open` is not Linux-shaped). Verified: `open`/`read`/`write`/
  `close` + a write+read roundtrip in `/persist`.
- `thread:` — pthreads work. `__clone` (src/dunit/clone.c) starts a thread via the
  native `ThreadCreate` with a trampoline that installs the thread pointer
  (`SetThreadPointer`) and registers clear-child-tid (`SetTidAddress`) before
  running musl's entry. The futex dispatcher (src/dunit/futex.c, wired into
  `__wake`/`__futexwait`/`__wait`/`__timedwait`) maps Linux `SYS_futex` onto the
  native `FutexWait`/`FutexWake`; `exit`→`ThreadExit` vs `exit_group`→`Exit`;
  `__unmapself` uses native numbers. The kernel's new `SetTidAddress` zeroes the
  clear-child-tid word + futex-wakes it on thread exit, releasing the thread-list
  lock musl holds across `SYS_exit` (the Linux `CLONE_CHILD_CLEARTID` contract).
  Verified: `pthread_create` ×2 + a contended `pthread_mutex` + `pthread_join`
  (counter=10000, both return values correct).

- `fs:` (read path) — `__stdio_read`/`readv` fill the caller's buffer with the
  native `Read` (no readv in the kernel). Verified: `fopen`/`fputs` then
  `fopen`/`fgets`×2/`fgetc` EOF roundtrip.
- `fs:` (stat) — `__fstatat` calls the native `Stat` and maps Dunit's
  `UserFileStat` (type + size) into `struct stat` (`st_mode` type bits +
  `st_size`; rest zero). Path-based only; fstat-by-fd / dirfd-relative report a
  documented `ENOSYS`. Verified: `stat` of a file + a directory.
- `fs:` (getdents) — `opendir` snapshots a directory via the path-based native
  `Readdir`; `readdir`/`closedir` serialize/free that snapshot (no fd+offset
  getdents in the kernel, so `dir->fd` is -1 and `dirfd()` is not a real fd).
  Verified: `opendir`/`readdir`/`closedir` list `/app` (64 entries) and find a
  known name.

The static C acceptance suite (`hello`/`stdio`/`malloc`/`file`/`thread`) all run
on the kernel, and the `fs:` surface (open/read/write/close, stat, directory
iteration, buffered stdio) is complete. `build-libc.sh` also installs a
`<sysroot>/bin/dunit-cc` wrapper that compiles+links a static `x86_64-dunit`
program in one command (the M6.4 cross-"toolchain"); the dunit-os Makefile builds
its C tests through it. Remaining:
- `signal:`/`network:`/`ldso:` — stubbed to documented `ENOSYS`; `AT_PHDR` auxv
  only needed once a program uses `__thread` TLS variables.
- Packaging: the upstream diff/version table (M6.2).

Each lands with its own themed commit and a `qemu_test.py` suite entry.
