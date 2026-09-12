/*
 * A stand-in for libdragon.h, for HOST-SIDE SYNTAX CHECKING ONLY.
 *
 * tools/n64_link_check.py deliberately skips src/platform/n64 because
 * it "genuinely needs libdragon headers", which left the platform
 * layer compiled by nothing in a checkout without libdragon -- and
 * this container is one. A file nobody can compile is a file whose
 * next edit is unverified.
 *
 * So: declare the handful of things the platform sources actually use
 * from libdragon, well enough for a -fsyntax-only pass to type-check
 * everything around them. This is NOT a libdragon implementation and
 * must never reach a ROM build; tools/n64_platform_check.py is the
 * only thing that puts it on an include path.
 *
 * Add a declaration here when a platform file starts using something
 * new -- the check failing is the signal that it did.
 */
#ifndef WM_HOST_SHIM_LIBDRAGON_H
#define WM_HOST_SHIM_LIBDRAGON_H

#include <stddef.h>
#include <stdint.h>

/* Write back a range of the data cache to RDRAM. */
void data_cache_hit_writeback(volatile const void *addr, unsigned long length);

#endif
