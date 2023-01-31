/*
 * Copyright (c) 2023 Gaël PORTAY
 * Copyright (c) 2023 Rtone
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <stdlib.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/fcb.h>

#if DT_HAS_CHOSEN(zephyr_settings_partition)
#define SETTINGS_PARTITION DT_FIXED_PARTITION_ID(DT_CHOSEN(zephyr_settings_partition))
#else
#define SETTINGS_PARTITION FIXED_PARTITION_ID(storage_partition)
#endif

static int init_helper(const struct shell *shell, int id, struct fcb *fcb,
		       struct flash_sector *fs, uint32_t fs_cnt)
{
	const struct flash_parameters *fp;
	const struct flash_area *fa;
	const struct device *dev;
	int ret = 0;

	ret = flash_area_open(id, &fa);
	if (ret) {
		shell_error(shell, "Failed to open flash area, ret: %d", ret);
		return 1;
	}

	dev = fa->fa_dev;

	flash_area_close(fa);

	fp = flash_get_parameters(dev);
	if (fp == NULL) {
		shell_error(shell, "Failed to get flash device parameters");
		return 1;
	}

	ret = flash_area_get_sectors(id, &fs_cnt, fs);
	if (fp == NULL) {
		shell_error(shell, "Failed to get flash sectors, ret: %d",
			    ret);
		return 1;
	}

	memset(fcb, 0, sizeof(*fcb));
	fcb->f_magic = 0xfcb1fcb1;
	fcb->f_version = 1;
	fcb->f_sector_cnt = fs_cnt;
	fcb->f_scratch_cnt = 1;
	fcb->f_sectors = fs;
	fcb->f_erase_value = fp->erase_value;
	ret = fcb_init(id, fcb);
	if (ret) {
		shell_error(shell, "Failed to init fcb, ret: %d", ret);
		return 1;
	}

	return 0;
}

static int cmd_info(const struct shell *shell, size_t argc, char *argv[])
{
	struct fcb fcb;
	struct flash_sector fs[10];
	uint32_t fs_cnt = ARRAY_SIZE(fs);
	int ret;

	ret = init_helper(shell, SETTINGS_PARTITION, &fcb, fs, fs_cnt);
	if (ret) {
		return ret;
	}


	shell_print(shell, "Magic:             %08x", fcb.f_magic);
	shell_print(shell, "Version:           %d", fcb.f_version);
	shell_print(shell, "Sector count:      %d", fcb.f_sector_cnt);
	shell_print(shell, "Scratch count:     %d", fcb.f_scratch_cnt);
	shell_print(shell, "Erase value:       %x", fcb.f_erase_value);

	ret = fcb_free_sector_cnt(&fcb);
	if (ret < 0) {
		shell_error(shell, "Failed to get free sector count from fcb, ret: %d",
			    ret);
		return ret;
	}

	shell_print(shell, "Free sector count: %d", ret);

	ret = fcb_is_empty(&fcb);
	if (ret < 0) {
		shell_error(shell, "Failed to get emptyness from fcb, ret: %d",
			    ret);
		return ret;
	}

	shell_print(shell, "Empty:             %d", ret);

	return 0;
}

static int cmd_is_empty(const struct shell *shell, size_t argc, char *argv[])
{
	struct fcb fcb;
	struct flash_sector fs[10];
	uint32_t fs_cnt = ARRAY_SIZE(fs);
	struct fcb_entry loc;
	int ret;

	ret = init_helper(shell, SETTINGS_PARTITION, &fcb, fs, fs_cnt);
	if (ret) {
		return ret;
	}

	ret = fcb_is_empty(&fcb);
	if (ret < 0) {
		shell_error(shell, "Failed to get emptyness from fcb, ret: %d",
			    ret);
		return ret;
	} else if (ret == 0) {
		shell_warn(shell, "FCB is not empty");
		return 1;
	}

	shell_print(shell, "FCB is empty");

	return 0;
}

static int cmd_next(const struct shell *shell, size_t argc, char *argv[])
{
	struct fcb fcb;
	struct flash_sector fs[10];
	uint32_t fs_cnt = ARRAY_SIZE(fs);
	uint8_t buf[FCB_MAX_LEN];
	uint16_t len;
	struct fcb_entry loc;
	int ret;

	ret = init_helper(shell, SETTINGS_PARTITION, &fcb, fs, fs_cnt);
	if (ret) {
		return ret;
	}

	memset(&loc, 0, sizeof(loc));
	ret = fcb_getnext(&fcb, &loc);
	if (ret) {
		shell_error(shell, "Failed to get next from fcb, ret: %d",
			    ret);
		return ret;
	}

	len = loc.fe_data_len;
	shell_print(shell, "Reading %u bytes to offset 0x%lx", len,
		    FCB_ENTRY_FA_DATA_OFF(loc));
	ret = flash_area_read(fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc), buf, len);
	if (ret) {
		shell_error(shell, "Failed to read flash area, ret: %d", ret);
		return ret;
	}

	shell_hexdump(shell, buf, len);

	return 0;
}

static int cmd_last(const struct shell *shell, size_t argc, char *argv[])
{
	struct fcb fcb;
	struct flash_sector fs[10];
	uint32_t fs_cnt = ARRAY_SIZE(fs);
	uint8_t n = 0, buf[FCB_MAX_LEN];
	uint16_t len;
	struct fcb_entry loc;
	int ret;

	ret = init_helper(shell, SETTINGS_PARTITION, &fcb, fs, fs_cnt);
	if (ret) {
		return ret;
	}

	if (argc > 1)
		n = strtoul(argv[1], NULL, 16);

	memset(&loc, 0, sizeof(loc));
	ret = fcb_offset_last_n(&fcb, n, &loc);
	if (ret) {
		shell_error(shell, "Failed to get offset last n from fcb, ret: %d",
			    ret);
		return ret;
	}

	len = loc.fe_data_len;
	shell_print(shell, "Reading %u bytes to offset 0x%lx", len,
		    FCB_ENTRY_FA_DATA_OFF(loc));
	ret = flash_area_read(fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc), buf, len);
	if (ret) {
		shell_error(shell, "Failed to read flash area, ret: %d", ret);
		return ret;
	}

	shell_hexdump(shell, buf, len);

	return 0;
}

static int cmd_append(const struct shell *shell, size_t argc, char *argv[])
{
	struct fcb fcb;
	struct flash_sector fs[10];
	uint32_t fs_cnt = ARRAY_SIZE(fs);
	uint8_t *buf;
	uint16_t len;
	struct fcb_entry loc;
	int i, ret;

	ret = init_helper(shell, SETTINGS_PARTITION, &fcb, fs, fs_cnt);
	if (ret) {
		return ret;
	}

	for (i = 1; i < argc; i ++) {
		buf = argv[i];
		len = strlen(argv[i]);
		ret = fcb_append(&fcb, len, &loc);
		if (ret) {
			shell_error(shell, "Failed to append to fcb, ret: %d", ret);
			return ret;
		}
	
		shell_print(shell, "Writing %u byte(s) to offset 0x%lx", len,
			    FCB_ENTRY_FA_DATA_OFF(loc));
		ret = flash_area_write(fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc), buf, len);
		if (ret) {
			shell_error(shell, "Failed to write flash area, ret: %d",
				    ret);
			return ret;
		}

		ret = fcb_append_finish(&fcb, &loc);
		if (ret) {
			shell_error(shell, "Failed to finish append to fcb, ret: %d",
				    ret);
			return ret;
		}
	}

	return 0;
}

static int cmd_scratch(const struct shell *shell, size_t argc, char *argv[])
{
	struct fcb fcb;
	struct flash_sector fs[10];
	uint32_t fs_cnt = ARRAY_SIZE(fs);
	int ret;

	ret = init_helper(shell, SETTINGS_PARTITION, &fcb, fs, fs_cnt);
	if (ret) {
		return ret;
	}

	ret = fcb_append_to_scratch(&fcb);
	if (ret) {
		shell_error(shell, "Failed to finish append to fcb, ret: %d",
			    ret);
		return ret;
	}

	return 0;
}

static int walk_cb(struct fcb_entry_ctx *entry_ctx, void *arg)
{
	const struct shell *shell = (const struct shell *)arg;
	uint8_t buf[FCB_MAX_LEN];
	uint16_t len = sizeof(buf);
	int ret;

	len = entry_ctx->loc.fe_data_len;
	shell_print(shell, "Reading %u byte(s) to offset 0x%lx", len,
		    FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc));
	ret = flash_area_read(entry_ctx->fap, FCB_ENTRY_FA_DATA_OFF(entry_ctx->loc), buf, len);
	if (ret) {
		shell_error(shell, "Failed to read flash area, ret: %d", ret);
		return ret;
	}

	shell_hexdump(shell, buf, len);

	return 0;
}

static int cmd_walk(const struct shell *shell, size_t argc, char *argv[])
{
	struct fcb fcb;
	struct flash_sector fs[10];
	uint32_t fs_cnt = ARRAY_SIZE(fs);
	int ret;

	ret = init_helper(shell, SETTINGS_PARTITION, &fcb, fs, fs_cnt);
	if (ret) {
		return ret;
	}

	ret = fcb_walk(&fcb, NULL, walk_cb, (void *)shell);
	if (ret) {
		shell_error(shell, "Failed to walk from fcb, ret: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_rotate(const struct shell *shell, size_t argc, char *argv[])
{
	struct fcb fcb;
	struct flash_sector fs[10];
	uint32_t fs_cnt = ARRAY_SIZE(fs);
	int ret;

	ret = init_helper(shell, SETTINGS_PARTITION, &fcb, fs, fs_cnt);
	if (ret) {
		return ret;
	}

	ret = fcb_rotate(&fcb);
	if (ret) {
		shell_error(shell, "Failed to rotate fcb, ret: %d", ret);
		return ret;
	}

	return 0;
}

static int cmd_clear(const struct shell *shell, size_t argc, char *argv[])
{
	struct fcb fcb;
	struct flash_sector fs[10];
	uint32_t fs_cnt = ARRAY_SIZE(fs);
	int ret;

	ret = init_helper(shell, SETTINGS_PARTITION, &fcb, fs, fs_cnt);
	if (ret) {
		return ret;
	}

	ret = fcb_clear(&fcb);
	if (ret) {
		shell_error(shell, "Failed to clear fcb, ret: %d", ret);
		return ret;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(fcb_cmds,
	SHELL_CMD_ARG(info,     NULL, NULL,         cmd_info,     1, 0),
	SHELL_CMD_ARG(is-empty, NULL, NULL,         cmd_is_empty, 1, 0),
	SHELL_CMD_ARG(next,     NULL, NULL,         cmd_next,     1, 0),
	SHELL_CMD_ARG(last,     NULL, "[n]",        cmd_last,     1, 1),
	SHELL_CMD_ARG(append,   NULL, "data [...]", cmd_append,   2, 255),
	SHELL_CMD_ARG(scratch,  NULL, NULL,         cmd_scratch,  1, 0),
	SHELL_CMD_ARG(walk,     NULL, NULL,         cmd_walk,     1, 0),
	SHELL_CMD_ARG(rotate,   NULL, NULL,         cmd_rotate,   1, 0),
	SHELL_CMD_ARG(clear,    NULL, NULL,         cmd_clear,    1, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_fcb(const struct shell *shell, size_t argc, char **argv)
{
	shell_error(shell, "%s:unknown parameter: %s", argv[0], argv[1]);
	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(fcb, &fcb_cmds, "FCB shell commands", cmd_fcb, 2, 0);
