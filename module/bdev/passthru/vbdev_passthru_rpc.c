/*   SPDX-License-Identifier: BSD-3-Clause
 *   Copyright (C) 2018 Intel Corporation.
 *   Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 *   All rights reserved.
 */

#include "vbdev_passthru.h"
#include "spdk/rpc.h"
#include "spdk/util.h"
#include "spdk/string.h"
#include "spdk/log.h"

/* Structure to hold the parameters for this RPC method. */
struct rpc_bdev_passthru_create {
	char *base_bdev_name;
	char *name;
	struct spdk_uuid uuid;
};

/* Free the allocated memory resource after the RPC handling. */
static void
free_rpc_bdev_passthru_create(struct rpc_bdev_passthru_create *r)
{
	free(r->base_bdev_name);
	free(r->name);
}

/* Structure to decode the input parameters for this RPC method. */
static const struct spdk_json_object_decoder rpc_bdev_passthru_create_decoders[] = {
	{"base_bdev_name", offsetof(struct rpc_bdev_passthru_create, base_bdev_name), spdk_json_decode_string},
	{"name", offsetof(struct rpc_bdev_passthru_create, name), spdk_json_decode_string},
	{"uuid", offsetof(struct rpc_bdev_passthru_create, uuid), spdk_json_decode_uuid, true},
};

/* Decode the parameters for this RPC method and properly construct the passthru
 * device. Error status returned in the failed cases.
 */
static void
rpc_bdev_passthru_create(struct spdk_jsonrpc_request *request,
			 const struct spdk_json_val *params)
{
	struct rpc_bdev_passthru_create req = {NULL};
	struct spdk_json_write_ctx *w;
	int rc;

	if (spdk_json_decode_object(params, rpc_bdev_passthru_create_decoders,
				    SPDK_COUNTOF(rpc_bdev_passthru_create_decoders),
				    &req)) {
		SPDK_DEBUGLOG(vbdev_passthru, "spdk_json_decode_object failed\n");
		spdk_jsonrpc_send_error_response(request, SPDK_JSONRPC_ERROR_INTERNAL_ERROR,
						 "spdk_json_decode_object failed");
		goto cleanup;
	}

	rc = bdev_passthru_create_disk(req.base_bdev_name, req.name, &req.uuid);
	if (rc != 0) {
		spdk_jsonrpc_send_error_response(request, rc, spdk_strerror(-rc));
		goto cleanup;
	}

	w = spdk_jsonrpc_begin_result(request);
	spdk_json_write_string(w, req.name);
	spdk_jsonrpc_end_result(request, w);

cleanup:
	free_rpc_bdev_passthru_create(&req);
}
SPDK_RPC_REGISTER("bdev_passthru_create", rpc_bdev_passthru_create, SPDK_RPC_RUNTIME)

struct rpc_bdev_passthru_delete {
	char *name;
};

static void
free_rpc_bdev_passthru_delete(struct rpc_bdev_passthru_delete *req)
{
	free(req->name);
}

static const struct spdk_json_object_decoder rpc_bdev_passthru_delete_decoders[] = {
	{"name", offsetof(struct rpc_bdev_passthru_delete, name), spdk_json_decode_string},
};

static void
rpc_bdev_passthru_delete_cb(void *cb_arg, int bdeverrno)
{
	struct spdk_jsonrpc_request *request = cb_arg;

	if (bdeverrno == 0) {
		spdk_jsonrpc_send_bool_response(request, true);
	} else {
		spdk_jsonrpc_send_error_response(request, bdeverrno, spdk_strerror(-bdeverrno));
	}
}

static void
rpc_bdev_passthru_delete(struct spdk_jsonrpc_request *request,
			 const struct spdk_json_val *params)
{
	struct rpc_bdev_passthru_delete req = {NULL};

	if (spdk_json_decode_object(params, rpc_bdev_passthru_delete_decoders,
				    SPDK_COUNTOF(rpc_bdev_passthru_delete_decoders),
				    &req)) {
		spdk_jsonrpc_send_error_response(request, SPDK_JSONRPC_ERROR_INTERNAL_ERROR,
						 "spdk_json_decode_object failed");
		goto cleanup;
	}

	bdev_passthru_delete_disk(req.name, rpc_bdev_passthru_delete_cb, request);

cleanup:
	free_rpc_bdev_passthru_delete(&req);
}
SPDK_RPC_REGISTER("bdev_passthru_delete", rpc_bdev_passthru_delete, SPDK_RPC_RUNTIME)



/* Structure to hold the statistics */
struct rpc_passthru_stats {
    char *name;
};

/* Free the memory allocated for the statistics */
static void
free_rpc_passthru_stats(struct rpc_passthru_stats *r)
{
    free(r->name);
}

/* Decode the RPC request parameters */
static const struct spdk_json_object_decoder rpc_passthru_stats_decoder[] = {
    {"name", offsetof(struct rpc_passthru_stats, name), spdk_json_decode_string},
};

/* Construct the RPC response */
static void
rpc_bdev_passthru_get_stats(struct spdk_jsonrpc_request *request,
                            const struct spdk_json_val *params)
{
    struct rpc_passthru_stats req = {};
    struct passthru_bdev_io *pt_bdev;
    struct spdk_json_write_ctx *w;

    if (spdk_json_decode_object(params, rpc_passthru_stats_decoder,
                                SPDK_COUNTOF(rpc_passthru_stats_decoder), &req)) {
        SPDK_ERRLOG("spdk_json_decode_object failed\n");
        spdk_jsonrpc_send_error_response(request, SPDK_JSONRPC_ERROR_INVALID_PARAMS,
                                         "Invalid parameters");
        goto cleanup;
    }

    pt_bdev = spdk_bdev_get_by_name(req.name)->ctxt;
    if (pt_bdev == NULL) {
        SPDK_ERRLOG("passthru bdev not found: %s\n", req.name);
        spdk_jsonrpc_send_error_response(request, SPDK_JSONRPC_ERROR_INVALID_PARAMS,
                                         "Passthru bdev not found");
        goto cleanup;
    }

    w = spdk_jsonrpc_begin_result(request);
    if (w == NULL) {
        goto cleanup;
    }

    spdk_json_write_object_begin(w);
    spdk_json_write_named_uint64(w, "read_io_count",  pt_bdev->read_io_count);
    spdk_json_write_named_uint64(w, "write_io_count", pt_bdev->write_io_count);
    spdk_json_write_named_uint64(w, "blocked_io_count", pt_bdev->blocked_io_count);
    spdk_json_write_named_uint64(w, "bytes_read", pt_bdev->bytes_read);
    spdk_json_write_named_uint64(w, "bytes_written", pt_bdev->bytes_written);
    spdk_json_write_object_end(w);

    spdk_jsonrpc_end_result(request, w);

cleanup:
    free_rpc_passthru_stats(&req);
}

/* Register the RPC method */
SPDK_RPC_REGISTER("bdev_passthru_get_stats", rpc_bdev_passthru_get_stats, SPDK_RPC_RUNTIME)
