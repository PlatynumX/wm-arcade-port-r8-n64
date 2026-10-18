/*
 * A stand-in for libdragon.h, for HOST-SIDE SYNTAX CHECKING ONLY.
 *
 * tools/n64_link_check.py deliberately skips src/platform/n64 because
 * it "genuinely needs libdragon headers", which left the platform
 * layer compiled by nothing in a checkout without libdragon -- and
 * this container is one. A file nobody can compile is a file whose
 * next edit is unverified.
 *
 * This is now a FALLBACK and not the main path. tools/n64_platform_check.py
 * prefers the REAL libdragon headers -- from $N64_INST when a local
 * install exists, otherwise from a pinned shallow clone it fetches once
 * -- and checks all five platform sources against them. The argument
 * this file was written under was that a shim big enough for main.c
 * "would be a fake libdragon rather than a check". That was right about
 * the shim and wrong about the conclusion: the real headers can simply
 * be fetched, and nothing about them has to be guessed.
 *
 * What is left here is the offline case: no N64_INST, no network, no
 * cache. Then the check says so plainly and covers the one file this
 * can honestly speak for, rather than implying the platform layer was
 * checked.
 *
 * This is NOT a libdragon implementation and must never reach a ROM
 * build; n64_platform_check.py is the only thing that puts it on an
 * include path.
 */
#ifndef WM_HOST_SHIM_LIBDRAGON_H
#define WM_HOST_SHIM_LIBDRAGON_H

#include <stddef.h>
#include <stdint.h>

/* Write back a range of the data cache to RDRAM. */
void data_cache_hit_writeback(volatile const void *addr, unsigned long length);

#endif
