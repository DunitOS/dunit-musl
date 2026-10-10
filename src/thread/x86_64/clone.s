/* Dunit port: __clone is provided in C by src/dunit/clone.c (it packs the
 * thread descriptor and calls the native ThreadCreate, since Dunit has no
 * Linux-style clone with CLONE_SETTLS). This file is intentionally empty so the
 * upstream x86_64 clone.s does not also define __clone. */
