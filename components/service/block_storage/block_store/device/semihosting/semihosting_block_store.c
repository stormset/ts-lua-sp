/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <lib/semihosting.h>
#include "semihosting_block_store.h"

#define ERASED_DATA_VAL		(0xff)

static psa_status_t seek(long handle, ssize_t pos)
{
	/* Appears to return bogus error so ignore error code */
	semihosting_file_seek(handle, pos);
	return PSA_SUCCESS;
}

static psa_status_t prepare_for_read(
	const struct semihosting_block_store *this_instance,
	uint64_t lba, size_t offset,
	size_t requested_read_len,
	size_t *adjusted_read_len)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	const struct storage_partition *storage_partition =
		&this_instance->base_block_device.storage_partition;

	ssize_t read_pos = lba * storage_partition->block_size + offset;
	ssize_t file_len = semihosting_file_length(this_instance->file_handle);

	if (file_len >= 0) {

		/* File exists so attempt to seek the read position to the requested LBA + offset */
		if (read_pos <= file_len) {

			size_t bytes_until_end_of_file = (size_t)(file_len - read_pos);
			size_t bytes_until_end_of_block = storage_partition->block_size - offset;

			size_t read_limit = (bytes_until_end_of_file < bytes_until_end_of_block) ?
				bytes_until_end_of_file :
				bytes_until_end_of_block;

			*adjusted_read_len = (requested_read_len < read_limit) ?
				requested_read_len :
				read_limit;

			status = seek(this_instance->file_handle, read_pos);
		}
		else {

			/* Requested block is beyond the end of the file */
			status = PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	return status;
}

static psa_status_t write_erased(
	const struct semihosting_block_store *this_instance,
	size_t pos,
	size_t len)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;
	size_t remaining_len = len;

	status = seek(this_instance->file_handle, pos);

	while ((status == PSA_SUCCESS) && (remaining_len > 0)) {

		size_t erase_len = (remaining_len < sizeof(this_instance->erase_buf)) ?
			remaining_len :
			sizeof(this_instance->erase_buf);

		size_t write_len = erase_len;

		if (semihosting_file_write(this_instance->file_handle,
			&write_len,
			(const uintptr_t)this_instance->erase_buf)) {

			status = PSA_ERROR_BAD_STATE;
		}

		remaining_len -= erase_len;
	}

	return status;
}

static psa_status_t prepare_for_write(
	const struct semihosting_block_store *this_instance,
	uint64_t lba, size_t offset,
	size_t requested_write_len,
	size_t *adjusted_write_len)
{
	psa_status_t status = PSA_ERROR_BAD_STATE;

	const struct storage_partition *storage_partition =
		&this_instance->base_block_device.storage_partition;

	size_t bytes_until_end_of_block = storage_partition->block_size - offset;
	*adjusted_write_len = (requested_write_len < bytes_until_end_of_block) ?
		requested_write_len :
		bytes_until_end_of_block;

	ssize_t write_pos = lba * storage_partition->block_size + offset;
	ssize_t file_len = semihosting_file_length(this_instance->file_handle);

	if (file_len >= 0) {

		if (write_pos > file_len) {

			/* Writing beyond the current end-of-file so extend the file */
			status = write_erased(this_instance, file_len, write_pos - file_len);
		}
		else {

			/* Writing over existing data */
			status = seek(this_instance->file_handle, write_pos);
		}
	}

	return status;
}

static psa_status_t semihosting_block_store_get_partition_info(void *context,
	const struct uuid_octets *partition_guid,
	struct storage_partition_info *info)
{
	struct semihosting_block_store *this_instance = (struct semihosting_block_store*)context;

	return block_device_get_partition_info(
		&this_instance->base_block_device, partition_guid, info);
}

static psa_status_t semihosting_block_store_open(void *context,
	uint32_t client_id,
	const struct uuid_octets *partition_guid,
	storage_partition_handle_t *handle)
{
	struct semihosting_block_store *this_instance = (struct semihosting_block_store*)context;
	psa_status_t status = PSA_ERROR_BAD_STATE;

	if (this_instance->file_handle > 0) {

		status = block_device_open(
			&this_instance->base_block_device,
			client_id, partition_guid, handle);
	}

	return status;
}

static psa_status_t semihosting_block_store_close(void *context,
	uint32_t client_id,
	storage_partition_handle_t handle)
{
	struct semihosting_block_store *this_instance = (struct semihosting_block_store*)context;

	return block_device_close(
		&this_instance->base_block_device, client_id, handle);
}

static psa_status_t semihosting_block_store_read(void *context,
	uint32_t client_id,
	storage_partition_handle_t handle,
	uint64_t lba,
	size_t offset,
	size_t buffer_size,
	uint8_t *buffer,
	size_t *data_len)
{
	const struct semihosting_block_store *this_instance =
		(struct semihosting_block_store*)context;

	psa_status_t status = block_device_check_access_permitted(
		&this_instance->base_block_device, client_id, handle);

	*data_len = 0;

	if (status == PSA_SUCCESS) {

		const struct storage_partition *storage_partition =
			&this_instance->base_block_device.storage_partition;

		if (storage_partition_is_lba_legal(storage_partition, lba) &&
			(offset < storage_partition->block_size)) {

			size_t read_len = 0;

			status = prepare_for_read(
				this_instance,
				lba, offset,
				buffer_size,
				&read_len);

			if (status == PSA_SUCCESS) {

				if (!semihosting_file_read(this_instance->file_handle,
					&read_len,
					(uintptr_t)buffer)) {

					*data_len = read_len;
				}
				else {

					status = PSA_ERROR_BAD_STATE;
				}
			}
		}
		else {

			/* Block or offset outside of configured limits */
			status = PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	return status;
}

static psa_status_t semihosting_block_store_write(void *context,
	uint32_t client_id,
	storage_partition_handle_t handle,
	uint64_t lba,
	size_t offset,
	const uint8_t *data,
	size_t data_len,
	size_t *num_written)
{
	struct semihosting_block_store *this_instance = (struct semihosting_block_store*)context;
	psa_status_t status = block_device_check_access_permitted(
		&this_instance->base_block_device, client_id, handle);

	*num_written = 0;

	if (status == PSA_SUCCESS) {

		const struct storage_partition *storage_partition =
			&this_instance->base_block_device.storage_partition;

		if (storage_partition_is_lba_legal(storage_partition, lba) &&
			(offset < storage_partition->block_size)) {

			size_t adjusted_len = 0;

			status = prepare_for_write(
				this_instance,
				lba, offset,
				data_len,
				&adjusted_len);

			if (status == PSA_SUCCESS) {

				size_t write_len = adjusted_len;

				if (!semihosting_file_write(this_instance->file_handle,
					&write_len,
					(uintptr_t)data)) {

					*num_written = adjusted_len;
				}
				else {

					status = PSA_ERROR_BAD_STATE;
				}
			}
		}
		else {

			/* Block or offset outside of configured limits */
			status = PSA_ERROR_INVALID_ARGUMENT;
		}
	}

	return status;
}

static psa_status_t semihosting_block_store_erase(void *context,
	uint32_t client_id,
	storage_partition_handle_t handle,
	uint64_t begin_lba,
	size_t num_blocks)
{
	struct semihosting_block_store *this_instance = (struct semihosting_block_store*)context;
	const struct storage_partition *storage_partition =
		&this_instance->base_block_device.storage_partition;

	psa_status_t status = block_device_check_access_permitted(
		&this_instance->base_block_device, client_id, handle);

	/* Sanitize the range of LBAs to erase */
	if ((status == PSA_SUCCESS) &&
		!storage_partition_is_lba_legal(storage_partition, begin_lba)) {

		status = PSA_ERROR_INVALID_ARGUMENT;
	}

	if (status == PSA_SUCCESS) {

		size_t blocks_remaining = storage_partition->num_blocks - begin_lba;
		size_t blocks_to_erase = (num_blocks < blocks_remaining) ? num_blocks : blocks_remaining;

		ssize_t file_len = semihosting_file_length(this_instance->file_handle);

		if (file_len >= 0) {

			/* File exists. If erased block falls within the limits of the file,
			 * explicitly set blocks to the erased state. If erased block is
			 * beyond EOF, there's nothing to do. */
			ssize_t block_pos = begin_lba * storage_partition->block_size;

			if (block_pos < file_len) {

				status = write_erased(this_instance,
					block_pos,
					blocks_to_erase * storage_partition->block_size);
			}
		}
		else {

			status = PSA_ERROR_BAD_STATE;
		}
	}

	return status;
}

struct block_store *semihosting_block_store_init(
	struct semihosting_block_store *this_instance,
	const char *filename,
	size_t block_size)
{
	struct block_store *block_store = NULL;
	size_t num_blocks = 0;

	/* Define concrete block store interface */
	static const struct block_store_interface interface =
	{
		semihosting_block_store_get_partition_info,
		semihosting_block_store_open,
		semihosting_block_store_close,
		semihosting_block_store_read,
		semihosting_block_store_write,
		semihosting_block_store_erase
	};

	/* Initialize base block_store */
	this_instance->base_block_device.base_block_store.context = this_instance;
	this_instance->base_block_device.base_block_store.interface = &interface;

	/* Initialize buffer used for erase operations */
	memset(this_instance->erase_buf, ERASED_DATA_VAL, sizeof(this_instance->erase_buf));

	long int file_len = semihosting_get_flen(filename);

	if (file_len < 0) {

		/* File doesn't exist so create an empty one. A call to
		 * semihosting_block_store_configure() will be needed to
		 * set the number of blocks and block size to make a viable
		 * block store. */
		long handle = semihosting_file_open(filename, FOPEN_MODE_WPLUSB);
		semihosting_file_close(handle);
	}
	else if (file_len > 0) {

		/* There's an existing file with contents so this should be
		 * formatted to the intended size of the storage. */
		num_blocks = (size_t)file_len / block_size;
	}

	/* Open host file for normal operation */
	this_instance->file_handle = semihosting_file_open(
		filename,
		FOPEN_MODE_RPLUSB);

	if (this_instance->file_handle > 0) {

		block_store = block_device_init(
			&this_instance->base_block_device, NULL, num_blocks, block_size);
	}

	return block_store;
}

void semihosting_block_store_deinit(
	struct semihosting_block_store *this_instance)
{
	if (this_instance->file_handle > 0) {

		semihosting_file_close(this_instance->file_handle);
		this_instance->file_handle = 0;
	}

	block_device_deinit(&this_instance->base_block_device);
}

psa_status_t semihosting_block_store_configure(
	struct semihosting_block_store *this_instance,
	const struct uuid_octets *disk_guid,
	size_t num_blocks,
	size_t block_size)
{
	block_device_configure(&this_instance->base_block_device,
		disk_guid, num_blocks, block_size);

	return PSA_SUCCESS;
}

void semihosting_block_store_wipe(
	const char *filename)
{
	/* If file exists and has contents, open with mode w+b to clear contents */
	if (semihosting_get_flen(filename) > 0) {

		long handle = semihosting_file_open(filename, FOPEN_MODE_WPLUSB);
		semihosting_file_close(handle);
	}
}