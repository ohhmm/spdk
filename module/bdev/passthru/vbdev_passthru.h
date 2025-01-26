/*   SPDX-License-Identifier: BSD-3-Clause
 *   Copyright (C) 2018 Intel Corporation.
 *   All rights reserved.
 */

#ifndef SPDK_VBDEV_PASSTHRU_H
#define SPDK_VBDEV_PASSTHRU_H

#include "spdk/stdinc.h"

#include "spdk/bdev.h"
#include "spdk/bdev_module.h"

/**
 * Just for stats, this pt_bdev module doesn't need it but this is essentially a per IO
 * context that we get handed by the bdev layer otherwise.
 */
struct passthru_bdev_io {
	uint8_t test;

	/* bdev related */
	struct spdk_io_channel *ch;

	/* for bdev_io_wait */
	struct spdk_bdev_io_wait_entry bdev_io_wait;

	/* stats */
    uint64_t read_io_count;
    uint64_t write_io_count;
    uint64_t blocked_io_count;
    uint64_t bytes_read;
    uint64_t bytes_written;
};

/**
 * Create new pass through bdev.
 *
 * \param bdev_name Bdev on which pass through vbdev will be created.
 * \param vbdev_name Name of the pass through bdev.
 * \param uuid Optional UUID to assign to the pass through bdev.
 * \return 0 on success, other on failure.
 */
int bdev_passthru_create_disk(const char *bdev_name, const char *vbdev_name,
			      const struct spdk_uuid *uuid);

/**
 * Delete passthru bdev.
 *
 * \param bdev_name Name of the pass through bdev.
 * \param cb_fn Function to call after deletion.
 * \param cb_arg Argument to pass to cb_fn.
 */
void bdev_passthru_delete_disk(const char *bdev_name, spdk_bdev_unregister_cb cb_fn,
			       void *cb_arg);

#endif /* SPDK_VBDEV_PASSTHRU_H */
