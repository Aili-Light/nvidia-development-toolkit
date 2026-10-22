/*
 * alg-max9296-alix8b.c - ailiteam max9296 driver
 *
 * Copyright (c) 2016-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>

#include <media/mc_common.h>
#include <media/tegra_v4l2_camera.h>
#include <media/tegracam_core.h>

#define DRIVER_VERSION KERNEL_VERSION(0x01, 0x00, 0x00)

#define MAX9296_PHY1_CLK_ADDR 0x320
#define MAX9296_INIT_DESKEW_ADDR 0x443

static const int algx8b_30fps_framerates[] = {
	30,
};

static const struct camera_common_frmfmt alg_max9296_alix8b_frmfmt[] = {
	{
		.size.width = 3840,
		.size.height = 2160,
		.framerates = algx8b_30fps_framerates,
		.num_framerates = ARRAY_SIZE(algx8b_30fps_framerates),
		.hdr_en = 0,
		.mode = 0,
	},
};

static const struct of_device_id alg_max9296_alix8b_of_match[] = {
	{.compatible = "ailiteam,alg-max9296-alix8b"},
	{},
};
MODULE_DEVICE_TABLE(of, alg_max9296_alix8b_of_match);

static const u32 ctrl_cid_list[] = {
	TEGRA_CAMERA_CID_GAIN,
	TEGRA_CAMERA_CID_EXPOSURE,
	TEGRA_CAMERA_CID_EXPOSURE_SHORT,
	TEGRA_CAMERA_CID_FRAME_RATE,
	// TEGRA_CAMERA_CID_FUSE_ID,
	// TEGRA_CAMERA_CID_HDR_EN,
	// TEGRA_CAMERA_CID_SENSOR_MODE_ID,
};

#define REG_NULL_MAGIC_NUM 0x36578453

struct regval
{
	u16 i2c_addr;
	u32 reg;
	u32 val;
	u32 mode;
};

typedef struct deserdes_param_config
{
	u32 width;
	u32 height;
	struct v4l2_fract max_fps;
	u32 hts_def;
	u32 vts_def;
	u32 exp_def;
	u32 link_freq;
	u32 bus_fmt;
	u32 bpp;
	u32 reg_length;
	u32 current_reg_length;
	u32 reg_start_pos;
	s32 channel;
	struct regval reg_list[500];
} deserdes_param_config_t;

uint8_t driver_busy_flag = 0;
struct alg_max9296_alix8b
{
	struct i2c_client *i2c_client;
	struct v4l2_subdev *subdev;
	u32 frame_length;
	s64 last_wdr_et_val;
	struct camera_common_data *s_data;
	struct tegracam_device *tc_dev;

	unsigned char i2c_addr;					// i2c地址
	int reset_gpio;							// 复位引脚
	int camera_pwdn_gpio;					// 电源引脚

	deserdes_param_config_t cfg; 			// 配置
	int deser_config;						// deser配置
	int config_index;						// 配置索引
};

static const struct regmap_config sensor_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	.use_single_rw = true,
#else
	.use_single_read = true,
	.use_single_write = true,
#endif
};


static const struct regval max9296_alix8b_1ch_master_reg_cfg_table[] =
{
	{0x00,  0x00,0xff,0x0808},
	{0x80,0x02d6,0x80,0x1608},
	{0x80,0x02c7,0x80,0x1608},
	{0x80,0x02ca,0x80,0x1608},
	{0x80,0x02be,0x80,0x1608},
	{0x00,  0x00,0x0f,0x0808},
	{0x90,0x0330,0x04,0x1608},
	{0x90,0x0002,0xf3,0x1608},
	{0x90,0x0051,0x02,0x1608},
	{0x90,0x0052,0x01,0x1608},
	{0x90,0x044a,0xd0,0x1608},
	{0x90,0x0320,0x2c,0x1608},
	{0x90,0x02cb,0x83,0x1608},
	{0x90,0x02cc,0x00,0x1608},
	{0x90,0x02cd,0x00,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x80,0x0383,0x00,0x1608},
	{0x80,0x0318,0x5e,0x1608},
	{0x80,0x0570,0x0c,0x1608},
	{0x80,0x02c7,0x90,0x1608},
	{0x80,0x02ca,0x90,0x1608},
	{0x80,0x02be,0x90,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x80,0x02d6,0x90,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x80,0x02ca,0x80,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x48,0x9645,0x00,0x1608},
	{0x48,0x9646,0x20,0x1608},
	{0x48,0x8006,0x13,0x1608},
	{0x90, REG_NULL_MAGIC_NUM, 0x00, 0x1608},
};

static const struct regval max9296_alix8b_2ch_master_reg_cfg_table[] =
{
	{0x90,0x0010,0x83,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x90,0x0313,0x00,0x1608},
	{0x00,  0x00,0x10,0x0808},
	{0x90,0x0010,0x23,0x1608},
	{0x00,  0x00,0x10,0x0808},
	{0x90,0x0320,0x37,0x1608},
	{0x90,0x0323,0x37,0x1608},
	{0x90,0x0330,0x04,0x1608},
	{0x90,0x044a,0xc0,0x1608},
	{0x90,0x048a,0xc0,0x1608},
	{0x90,0x0050,0x00,0x1608},
	{0x90,0x0051,0x01,0x1608},
	{0x90,0x0052,0x02,0x1608},
	{0x90,0x0053,0x03,0x1608},
	{0x90,0x040b,0x07,0x1608},
	{0x90,0x044b,0x07,0x1608},
	{0x90,0x048b,0x07,0x1608},
	{0x90,0x04cb,0x07,0x1608},
	{0x90,0x042d,0x00,0x1608},
	{0x90,0x046d,0x15,0x1608},
	{0x90,0x04ad,0x15,0x1608},
	{0x90,0x04ed,0x00,0x1608},
	{0x90,0x044d,0x1e,0x1608},
	{0x90,0x044e,0x5e,0x1608},
	{0x90,0x044f,0x00,0x1608},
	{0x90,0x0450,0x40,0x1608},
	{0x90,0x0451,0x01,0x1608},
	{0x90,0x0452,0x41,0x1608},
	{0x90,0x048d,0x1e,0x1608},
	{0x90,0x048e,0x1e,0x1608},
	{0x90,0x048f,0x00,0x1608},
	{0x90,0x0490,0x00,0x1608},
	{0x90,0x0491,0x01,0x1608},
	{0x90,0x0492,0x01,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x90,0x0010,0x21,0x1608},
	{0x00,  0x00,0xf0,0x0808},
	{0x80,0x0000,0x88,0x1608},
	{0x00,  0x00,0x10,0x0808},
	{0x88,0x0042,0xe4,0x1608},
	{0x88,0x0043,0xa0,0x1608},
	{0x88,0x0044,0xe6,0x1608},
	{0x88,0x0045,0x6c,0x1608},
	{0x88,0x008b,0x01,0x1608},
	{0x88,0x02d6,0x80,0x1608},
	{0x88,0x02c7,0x80,0x1608},
	{0x88,0x02ca,0x90,0x1608},
	{0x88,0x02be,0x80,0x1608},
	{0x00,  0x00,0x0f,0x0808},
	{0x88,0x005b,0x11,0x1608},
	{0x88,0x0383,0x00,0x1608},
	{0x88,0x0318,0x5e,0x1608},
	{0x88,0x0570,0x0c,0x1608},
	{0x88,0x02c7,0x90,0x1608},
	{0x88,0x02d6,0x90,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x88,0x02ca,0x80,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x48,0x9645,0x00,0x1608},
	{0x48,0x9646,0x20,0x1608},
	{0x48,0x8006,0x13,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x90,0x0010,0x22,0x1608},
	{0x00,  0x00,0xf0,0x0808},
	{0x80,0x0000,0x84,0x1608},
	{0x00,  0x00,0x10,0x0808},
	{0x84,0x0042,0xe8,0x1608},
	{0x84,0x0043,0xa0,0x1608},
	{0x84,0x0044,0xea,0x1608},
	{0x84,0x0045,0x6c,0x1608},
	{0x84,0x008b,0x02,0x1608},
	{0x84,0x02d6,0x80,0x1608},
	{0x84,0x02c7,0x80,0x1608},
	{0x84,0x02ca,0x90,0x1608},
	{0x84,0x02be,0x80,0x1608},
	{0x00,  0x00,0x0f,0x0808},
	{0x84,0x005b,0x12,0x1608},
	{0x84,0x0383,0x00,0x1608},
	{0x84,0x0318,0x5e,0x1608},
	{0x84,0x0570,0x0c,0x1608},
	{0x84,0x02c7,0x90,0x1608},
	{0x84,0x02d6,0x90,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x84,0x02ca,0x80,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x48,0x9645,0x00,0x1608},
	{0x48,0x9646,0x20,0x1608},
	{0x48,0x8006,0x13,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x90,0x0010,0x23,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x90,0x0313,0x02,0x1608},
	{0x90, REG_NULL_MAGIC_NUM, 0x00, 0x1608},
};


static const struct regval max9296_alix8b_1ch_slave_reg_cfg_table[] =
{
	{0x00,  0x00,0xff,0x0808},
	{0x80,0x02d6,0x80,0x1608},
	{0x80,0x02c7,0x80,0x1608},
	{0x80,0x02ca,0x80,0x1608},
	{0x80,0x02be,0x80,0x1608},
	{0x00,  0x00,0x0f,0x0808},
	{0x90,0x0330,0x04,0x1608},
	{0x90,0x0002,0xf3,0x1608},
	{0x90,0x0051,0x02,0x1608},
	{0x90,0x0052,0x01,0x1608},
	{0x90,0x044a,0xd0,0x1608},
	{0x90,0x0320,0x2c,0x1608},
	{0x90,0x02cb,0x83,0x1608},
	{0x90,0x02cc,0x00,0x1608},
	{0x90,0x02cd,0x00,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x80,0x0383,0x00,0x1608},
	{0x80,0x0318,0x5e,0x1608},
	{0x80,0x0570,0x0c,0x1608},
	{0x80,0x02c7,0x90,0x1608},
	{0x80,0x02ca,0x90,0x1608},
	{0x80,0x02be,0x84,0x1608},
	{0x80,0x02bf,0x20,0x1608},
	{0x80,0x02c0,0x00,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x80,0x02d6,0x90,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x80,0x02ca,0x80,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x48,0x9645,0x00,0x1608},
	{0x48,0x9646,0x20,0x1608},
	{0x48,0x8006,0x13,0x1608},
	{0x90, REG_NULL_MAGIC_NUM, 0x00, 0x1608},
};

static const struct regval max9296_alix8b_2ch_slave_reg_cfg_table[] =
{
	{0x90,0x0010,0x83,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x90,0x0313,0x00,0x1608},
	{0x00,  0x00,0x10,0x0808},
	{0x90,0x0010,0x23,0x1608},
	{0x00,  0x00,0x10,0x0808},
	{0x90,0x0320,0x37,0x1608},
	{0x90,0x0323,0x37,0x1608},
	{0x90,0x0330,0x04,0x1608},
	{0x90,0x044a,0xc0,0x1608},
	{0x90,0x048a,0xc0,0x1608},
	{0x90,0x0050,0x00,0x1608},
	{0x90,0x0051,0x01,0x1608},
	{0x90,0x0052,0x02,0x1608},
	{0x90,0x0053,0x03,0x1608},
	{0x90,0x040b,0x07,0x1608},
	{0x90,0x044b,0x07,0x1608},
	{0x90,0x048b,0x07,0x1608},
	{0x90,0x04cb,0x07,0x1608},
	{0x90,0x042d,0x00,0x1608},
	{0x90,0x046d,0x15,0x1608},
	{0x90,0x04ad,0x15,0x1608},
	{0x90,0x04ed,0x00,0x1608},
	{0x90,0x044d,0x1e,0x1608},
	{0x90,0x044e,0x5e,0x1608},
	{0x90,0x044f,0x00,0x1608},
	{0x90,0x0450,0x40,0x1608},
	{0x90,0x0451,0x01,0x1608},
	{0x90,0x0452,0x41,0x1608},
	{0x90,0x048d,0x1e,0x1608},
	{0x90,0x048e,0x1e,0x1608},
	{0x90,0x048f,0x00,0x1608},
	{0x90,0x0490,0x00,0x1608},
	{0x90,0x0491,0x01,0x1608},
	{0x90,0x0492,0x01,0x1608},
	{0x90,0x02cb,0x83,0x1608},
	{0x90,0x02cc,0x29,0x1608},
	{0x90,0x02ce,0x83,0x1608},
	{0x90,0x02cf,0x2a,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x90,0x0010,0x21,0x1608},
	{0x00,  0x00,0xf0,0x0808},
	{0x80,0x0000,0x88,0x1608},
	{0x00,  0x00,0x10,0x0808},
	{0x88,0x0042,0xe4,0x1608},
	{0x88,0x0043,0xa0,0x1608},
	{0x88,0x0044,0xe6,0x1608},
	{0x88,0x0045,0x6c,0x1608},
	{0x88,0x008b,0x01,0x1608},
	{0x88,0x02d6,0x80,0x1608},
	{0x88,0x02c7,0x80,0x1608},
	{0x88,0x02ca,0x90,0x1608},
	{0x88,0x02be,0x80,0x1608},
	{0x00,  0x00,0x0f,0x0808},
	{0x88,0x005b,0x11,0x1608},
	{0x88,0x0383,0x00,0x1608},
	{0x88,0x0318,0x5e,0x1608},
	{0x88,0x0570,0x0c,0x1608},
	{0x88,0x02c7,0x90,0x1608},
	{0x88,0x02be,0x84,0x1608},
	{0x88,0x02bf,0x20,0x1608},
	{0x88,0x02c0,0x0a,0x1608},
	{0x88,0x02d6,0x90,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x88,0x02ca,0x80,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x48,0x9645,0x00,0x1608},
	{0x48,0x9646,0x20,0x1608},
	{0x48,0x8006,0x13,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x90,0x0010,0x22,0x1608},
	{0x00,  0x00,0xf0,0x0808},
	{0x80,0x0000,0x84,0x1608},
	{0x00,  0x00,0x10,0x0808},
	{0x84,0x0042,0xe8,0x1608},
	{0x84,0x0043,0xa0,0x1608},
	{0x84,0x0044,0xea,0x1608},
	{0x84,0x0045,0x6c,0x1608},
	{0x84,0x008b,0x02,0x1608},
	{0x84,0x02d6,0x80,0x1608},
	{0x84,0x02c7,0x80,0x1608},
	{0x84,0x02ca,0x90,0x1608},
	{0x84,0x02be,0x80,0x1608},
	{0x00,  0x00,0x0f,0x0808},
	{0x84,0x005b,0x12,0x1608},
	{0x84,0x0383,0x00,0x1608},
	{0x84,0x0318,0x5e,0x1608},
	{0x84,0x0570,0x0c,0x1608},
	{0x84,0x02c7,0x90,0x1608},
	{0x84,0x02be,0x84,0x1608},
	{0x84,0x02bf,0x20,0x1608},
	{0x84,0x02c0,0x09,0x1608},
	{0x84,0x02d6,0x90,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x84,0x02ca,0x80,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x48,0x9645,0x00,0x1608},
	{0x48,0x9646,0x20,0x1608},
	{0x48,0x8006,0x13,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x90,0x0010,0x23,0x1608},
	{0x00,  0x00,0xff,0x0808},
	{0x90,0x0313,0x02,0x1608},
	{0x90, REG_NULL_MAGIC_NUM, 0x00, 0x1608},
};


static inline int alg_max9296_alix8b_read_reg(struct camera_common_data *s_data,
											  u16 addr, u8 *val)
{
	int err = 0;
	u32 reg_val = 0;

	err = regmap_read(s_data->regmap, addr, &reg_val);
	*val = reg_val & 0xFF;

	return err;
}

static int alg_max9296_alix8b_set_group_hold(struct tegracam_device *tc_dev, bool val)
{
	dev_info(&tc_dev->client->dev, "alg_max9296_alix8b_set_group_hold \n");
	return 0;
}

static int alg_max9296_alix8b_set_gain(struct tegracam_device *tc_dev, s64 val)
{
	dev_info(&tc_dev->client->dev, "alg_max9296_alix8b_set_gain \n");
	return 0;
}

static int alg_max9296_alix8b_set_frame_rate(struct tegracam_device *tc_dev, s64 val)
{
	dev_info(&tc_dev->client->dev, "alg_max9296_alix8b_set_frame_rate \n");
	return 0;
}

static int alg_max9296_alix8b_set_exposure(struct tegracam_device *tc_dev, s64 val)
{
	dev_info(&tc_dev->client->dev, "alg_max9296_alix8b_set_exposure \n");
	return 0;
}

static struct tegracam_ctrl_ops alg_max9296_alix8b_ctrl_ops = {
	.numctrls = ARRAY_SIZE(ctrl_cid_list),
	.ctrl_cid_list = ctrl_cid_list,
	.set_gain = alg_max9296_alix8b_set_gain,
	.set_exposure = alg_max9296_alix8b_set_exposure,
	.set_exposure_short = alg_max9296_alix8b_set_exposure,
	.set_frame_rate = alg_max9296_alix8b_set_frame_rate,
	.set_group_hold = alg_max9296_alix8b_set_group_hold,
};

static int alg_max9296_alix8b_power_on(struct camera_common_data *s_data)
{
	struct device *dev = s_data->dev;
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int alg_max9296_alix8b_power_off(struct camera_common_data *s_data)
{
	struct device *dev = s_data->dev;
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int alg_max9296_alix8b_power_get(struct tegracam_device *tc_dev)
{
	struct camera_common_data *s_data = tc_dev->s_data;
	struct device *dev = s_data->dev;
	dev_info(dev, "%s\n", __func__);
	return 0;
}

static int alg_max9296_alix8b_power_put(struct tegracam_device *tc_dev)
{
	struct camera_common_data *s_data = tc_dev->s_data;
	struct camera_common_power_rail *pw = s_data->power;

	dev_info(&tc_dev->client->dev, "%s\n", __func__);
	if (unlikely(!pw))
		return -EFAULT;

	return 0;
}

static struct camera_common_pdata *alg_max9296_alix8b_parse_dt(struct tegracam_device *tc_dev)
{
	struct device *dev = tc_dev->dev;
	struct device_node *np = dev->of_node;
	struct camera_common_pdata *board_priv_pdata;
	const struct of_device_id *match;
	if (!np)
		return NULL;

	match = of_match_device(alg_max9296_alix8b_of_match, dev);
	if (!match)
	{
		dev_err(dev, "Failed to find matching dt id\n");
		return NULL;
	}

	board_priv_pdata = devm_kzalloc(dev,
									sizeof(*board_priv_pdata), GFP_KERNEL);
	if (!board_priv_pdata)
		return NULL;

	return board_priv_pdata;
}

static int alg_max9296_alix8b_set_mode(struct tegracam_device *tc_dev)
{
	struct device *dev = tc_dev->dev;
	dev_info(dev, "set mode...\n");
	return 0;
}

static int max9296_write_reg(struct i2c_client *client,
							 u16 client_addr, u32 reg, u16 reg_len, u16 val_len, u32 val)
{
	u32 buf_i = 0, val_i;
	u8 buf[8];
	u8 *val_p;
	__be32 val_be;

	dev_info(&client->dev, "addr(0x%02x) write reg(0x%04x, %d, 0x%04x)\n",
			 client_addr, reg, reg_len, val);

	if (val_len > 4)
		return -EINVAL;

	// if (reg_len == 2) {
	// 	buf[0] = reg >> 8;
	// 	buf[1] = reg & 0xff;

	// 	buf_i = 2;
	// } else {
	// 	buf[0] = reg & 0xff;

	// 	buf_i = 1;
	// }
	val_be = cpu_to_be32(reg);
	val_p = (u8 *)&val_be;
	val_i = 4 - reg_len;
	while (val_i < 4)
		buf[buf_i++] = val_p[val_i++];

	val_be = cpu_to_be32(val);
	val_p = (u8 *)&val_be;
	val_i = 4 - val_len;

	while (val_i < 4)
		buf[buf_i++] = val_p[val_i++];

	client->addr = client_addr;

	if (i2c_master_send(client, buf, (val_len + reg_len)) != (val_len + reg_len))
	{
		dev_err(&client->dev,
				"%s: writing register 0x%04x from 0x%02x failed\n",
				__func__, reg, client->addr);
		return -EIO;
	}

	return 0;
}

static int max9296_read_reg(struct i2c_client *client, unsigned short addr, unsigned short int reg_rec, unsigned char *val, unsigned int val_len)
{
	int ret = 0;
	struct i2c_msg msgs[2] = {0};
	uint8_t uint8_t_buf[2] = {0};
	uint8_t buf_len = 2;
	uint8_t_buf[0] = (reg_rec >> 8) & 0xFF;
	uint8_t_buf[1] = reg_rec & 0xFF;

	/* Write register address */
	msgs[0].addr = addr;
	msgs[0].flags = 0;
	msgs[0].len = buf_len;
	msgs[0].buf = uint8_t_buf;

	/* Read data from register */
	msgs[1].addr = addr;
	msgs[1].flags = 1;
	msgs[1].len = val_len;
	msgs[1].buf = val;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return -1;
	return 0;
}

#if 1
static int max9296_write_array(struct i2c_client *client,
							   const struct regval *regs)
{
	struct device *dev = &client->dev;
	u32 i;
	int ret = 0;
	u8 reg_length = 0;
	u8 val_length = 0;
	u16 addr = 0;
	for (i = 0; ret == 0 && regs[i].reg != REG_NULL_MAGIC_NUM; i++)
	{
		reg_length = ((regs[i].mode >> 8) & 0xff) / 8;
		val_length = (regs[i].mode & 0xff) / 8;
		// dev_info(dev, " addr 0x%4x, reg 0x%4x, data 0x%4x mode 0x%4x param input\n",
		// 			regs[i].i2c_addr, regs[i].reg, regs[i].val, regs[i].mode);
		if ((reg_length == 0) || (reg_length > 4) || (val_length == 0) || (val_length > 4))
		{
			dev_err(dev, "addr 0x%4x, reg 0x%4x, data 0x%4x mode 0x%4x param input error\n",
					regs[i].i2c_addr, regs[i].reg, regs[i].val, regs[i].mode);
			continue;
		}
		if ((regs[i].i2c_addr == 0) && (regs[i].reg == 0))
		{
			msleep(regs[i].val);
		}
		else
		{
			addr = regs[i].i2c_addr >> 1;
			ret = max9296_write_reg(client, addr,
									regs[i].reg, reg_length, val_length, regs[i].val);
		}
	}
	return ret;
}
#endif


static int alg_max9296_alix8b_start_streaming(struct tegracam_device *tc_dev)
{
	struct alg_max9296_alix8b *priv = (struct alg_max9296_alix8b *)tc_dev->s_data->priv;
	struct device *dev = tc_dev->dev;

	int ret = 0;
	unsigned char val[1] = {0x00};
	unsigned short addr = priv->i2c_addr;

	if (!priv || !priv->i2c_client || !priv->s_data || !priv->tc_dev)
	{
		dev_err(dev, "alg_max9296_alix8b_start_streaming: invalid private data\n");
		return -EINVAL;
	}

	// 读取line速率
	ret = max9296_read_reg(tc_dev->client, priv->i2c_addr, MAX9296_PHY1_CLK_ADDR, val, 1);
	if (ret < 0)
	{
		dev_err(dev, "read line rate err\n");
		return -1;
	}
	dev_info(dev, "read line rate:0x%02x \n", val[0]);
	usleep_range(5000, 10000);
	// mipi clock > 1.5G 校准
	if ((val[0] & 0x1F) > 0x0f)
	{
		dev_info(dev, "start deskew \n");
		ret = max9296_read_reg(tc_dev->client, addr, MAX9296_INIT_DESKEW_ADDR, val, 1);
		if (ret < 0)
		{
			return -1;
		}
		// ret = max9296_write_reg(tc_dev->client, addr, 0x313, 2,1,0x00);
		// if(ret <0) {
		// 	dev_info(dev, "deskew err 1\n");
		// 	return -1;
		// }
		// msleep(500);
		usleep_range(50000, 100000);
		if (((val[0] >> 5) & 0x1) == 0x1)
		{
			ret = max9296_write_reg(tc_dev->client, addr, MAX9296_INIT_DESKEW_ADDR, 2, 1, 0x17);
			if (ret < 0)
			{
				dev_info(dev, "deskew err 1\n");
				return -1;
			}
		}
		else if (((val[0] >> 5) & 0x1) == 0x0)
		{
			ret = max9296_write_reg(tc_dev->client, addr, MAX9296_INIT_DESKEW_ADDR, 2, 1, 0x37);
			if (ret < 0)
			{
				dev_info(dev, "deskew err 2\n");
				return -1;
			}
		}
	}
	dev_info(dev, "start streaming ok\n");
	return 0;
}

static int alg_max9296_alix8b_stop_streaming(struct tegracam_device *tc_dev)
{
	struct device *dev = tc_dev->dev;
	struct alg_max9296_alix8b *priv = (struct alg_max9296_alix8b *)tc_dev->s_data->priv;
	if (!priv || !priv->i2c_client || !priv->s_data || !priv->tc_dev)
	{
		dev_err(dev, "alg_max9296_alix8b_stop_streaming: invalid private data\n");
		return -EINVAL;
	}
	dev_info(dev, "stop streaming...\n");
	return 0;
}

static struct camera_common_sensor_ops alg_max9296_alix8b_common_ops = {
	.numfrmfmts = ARRAY_SIZE(alg_max9296_alix8b_frmfmt),
	.frmfmt_table = alg_max9296_alix8b_frmfmt,
	.power_on = alg_max9296_alix8b_power_on,
	.power_off = alg_max9296_alix8b_power_off,
	// .write_reg = alg_max9296_alix8b_write_reg,
	.read_reg = alg_max9296_alix8b_read_reg,
	.parse_dt = alg_max9296_alix8b_parse_dt,
	.power_get = alg_max9296_alix8b_power_get,
	.power_put = alg_max9296_alix8b_power_put,
	.set_mode = alg_max9296_alix8b_set_mode,
	.start_streaming = alg_max9296_alix8b_start_streaming,
	.stop_streaming = alg_max9296_alix8b_stop_streaming,
};

static int alg_max9296_alix8b_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "%s:\n", __func__);

	return 0;
}

static const struct v4l2_subdev_internal_ops alg_max9296_alix8b_subdev_internal_ops = {
	.open = alg_max9296_alix8b_open,
};

static int max9296_verfy_chipid(struct i2c_client *client, unsigned short addr)
{
	unsigned char buf[2] = {0x00, 0x0d};
	unsigned char val = 0;
	struct i2c_msg msgs[2] = {0};

	msgs[0].addr = addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = buf;

	msgs[1].addr = addr;
	msgs[1].flags = 1;
	msgs[1].len = 1;
	msgs[1].buf = &val;

	i2c_transfer(client->adapter, msgs, 2);

	return val == 0x94;
}


struct max9296_dt_info
{
	char addr;								// i2c地址
	int reset_gpio;							// 复位引脚
	int camera_pwdn_gpio;					// 电源引脚
	int deser_config;						// deser配置
	int config_index;						// 配置索引
};
static unsigned short get_dt_info(struct i2c_client *client, struct max9296_dt_info *info)
{
	struct device_node *node = client->dev.of_node;
	const struct of_device_id *match;
	int ret;
	// int num = 0;
	u32 value;
	// const char *num_start;

	match = of_match_device(alg_max9296_alix8b_of_match, &client->dev);
	if (!match)
	{
		dev_err(&client->dev, "Failed to find matching dt id.\n");
		return -1;
	}

	ret = of_property_read_u32(node, "addr_reg", &value);
	if (ret < 0)
	{
		dev_err(&client->dev, "No addr_reg info\n");
		return ret;
	}
	info->addr = value;
	dev_info(&client->dev, "addr:%d \n", info->addr);

	info->reset_gpio = of_get_named_gpio(node, "reset-gpios", 0);
	if (info->reset_gpio < 0)
	{
		dev_err(&client->dev, "reset-gpios not found \n");
		return -1;
	}
	dev_info(&client->dev, "reset_gpio : %d \n", info->reset_gpio);

	info->camera_pwdn_gpio = of_get_named_gpio(node, "camera-gpios", 0);
	if (info->camera_pwdn_gpio < 0)
	{
		dev_err(&client->dev, "camera_pwdn_gpio not found \n");
		return -1;
	}
	dev_info(&client->dev, "camera_pwdn_gpio : %d \n", info->camera_pwdn_gpio);

	info->deser_config = 0;
	if (device_property_read_bool(&client->dev, "deser-config"))
	{
		dev_info(&client->dev, "deser-config \n");
		info->deser_config = 1;
		if (device_property_read_u32(&client->dev, "config-index", &value))
		{
			dev_info(&client->dev, "config-index not found \n");
			return -1;
		}
		info->config_index = value;
	}



	return 0;
}

struct v4l2_subdev_core_ops alg_9296_subdev_core;
struct v4l2_subdev_ops alg_9296_subdev_subdev_ops;

static int alg_max9296_alix8b_probe(struct i2c_client *client,
									const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct tegracam_device *tc_dev;
	struct alg_max9296_alix8b *priv;

	int err;

	struct max9296_dt_info dt_info;

	dev_info(&client->dev, "start alg_max9296_alix8b_probe \n");
	dev_info(&client->dev, "[aili_max9296]driver version: %02x.%02x.%02x\n", DRIVER_VERSION >> 16,
			 (DRIVER_VERSION & 0xff00) >> 8, DRIVER_VERSION & 0x00ff);

	if (!IS_ENABLED(CONFIG_OF) || !client->dev.of_node)
		return -EINVAL;

	if (get_dt_info(client, &dt_info))
		return -EINVAL;

	// 上电并首先检查芯片id , 确定max9296连接正常
	gpio_set_value(dt_info.reset_gpio, 1);
	gpio_set_value(dt_info.camera_pwdn_gpio, 1);
	usleep_range(5000, 5100);
	if (!max9296_verfy_chipid(client, dt_info.addr))
	{
		dev_err(&client->dev, "cann't find max9296!\n");
		return -ENODEV;
	}

	// 实例化 max9296_alix8b
	priv = devm_kzalloc(dev,
						sizeof(struct alg_max9296_alix8b), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->i2c_addr = dt_info.addr;
	priv->reset_gpio = dt_info.reset_gpio;
	priv->camera_pwdn_gpio = dt_info.camera_pwdn_gpio;
	priv->deser_config = dt_info.deser_config;
	priv->config_index = dt_info.config_index;
	// 实例化相机
	tc_dev = devm_kzalloc(dev, sizeof(struct tegracam_device), GFP_KERNEL);
	if (!tc_dev)
		return -ENOMEM;

	// 记录 client
	priv->i2c_client = tc_dev->client = client;

	// 拷贝子类信息到父类
	tc_dev->dev = dev;
	strncpy(tc_dev->name, "max9296", sizeof(tc_dev->name));
	// 寄存器配置格式
	tc_dev->dev_regmap_config = &sensor_regmap_config;
	// 操作接口注册初始化
	tc_dev->sensor_ops = &alg_max9296_alix8b_common_ops;

	// 操作接口注册
	tc_dev->v4l2sd_internal_ops = &alg_max9296_alix8b_subdev_internal_ops;
	tc_dev->tcctrl_ops = &alg_max9296_alix8b_ctrl_ops;

	// 相机注册
	err = tegracam_device_register(tc_dev);
	if (err)
	{
		dev_err(dev, "tegra camera driver registration failed\n");
		return err;
	}

	// 建立关联数据结构之间关联
	priv->tc_dev = tc_dev;
	priv->s_data = tc_dev->s_data;
	priv->subdev = &tc_dev->s_data->subdev;
	tegracam_set_privdata(tc_dev, (void *)priv);

	// 注册标准V4L2相机为subdevice
	err = tegracam_v4l2subdev_register(tc_dev, true);
	if (err)
	{
		dev_err(&client->dev, "tegra camera subdev registration failed\n");
		return err;
	}

	dev_info(&client->dev, "tegracam_v4l2subdev_register ok... \n");

	if(priv->deser_config)
	{
		gpio_set_value(priv->reset_gpio, 1);
		gpio_set_value(priv->camera_pwdn_gpio, 1);
		msleep(100);

		switch (priv->config_index)
		{
		case 1:
			dev_info(&client->dev, "deser config: 1ch master \n");
			err = max9296_write_array(tc_dev->client, max9296_alix8b_1ch_master_reg_cfg_table);
			if (err)
			{
				dev_err(&tc_dev->client->dev, "Failed to write reg array\n");
			}
			break;
		case 2:
			dev_info(&client->dev, "deser config: 2ch master \n");
			err = max9296_write_array(tc_dev->client, max9296_alix8b_2ch_master_reg_cfg_table);
			if (err)
			{
				dev_err(&tc_dev->client->dev, "Failed to write reg array\n");
			}
			break;
		case 3:
			dev_info(&client->dev, "deser config: 1ch slave \n");
			err = max9296_write_array(tc_dev->client, max9296_alix8b_1ch_slave_reg_cfg_table);
			if (err)
			{
				dev_err(&tc_dev->client->dev, "Failed to write reg array\n");
			}
			break;
		case 4:
			dev_info(&client->dev, "deser config: 2ch slave \n");
			err = max9296_write_array(tc_dev->client, max9296_alix8b_2ch_slave_reg_cfg_table);
			if (err)
			{
				dev_err(&tc_dev->client->dev, "Failed to write reg array\n");
			}
			break;
		default:
			dev_info(&client->dev, "deser config: default \n");
			break;
		}
	}
	dev_info(&client->dev, "success config deser\n");

	return 0;
}

static int alg_max9296_alix8b_remove(struct i2c_client *client)
{
	struct camera_common_data *s_data = to_camera_common_data(&client->dev);
	struct alg_max9296_alix8b *priv = (struct alg_max9296_alix8b *)s_data->priv;

	// 下电，使得复位
	gpio_set_value(priv->reset_gpio, 0);
	gpio_set_value(priv->camera_pwdn_gpio, 0);
	tegracam_v4l2subdev_unregister(priv->tc_dev);
	tegracam_device_unregister(priv->tc_dev);

	return 0;
}

static const struct i2c_device_id alg_max9296_alix8b_id[] = {
	{"alg-max9296-alix8b", 0},
	{}};

MODULE_DEVICE_TABLE(i2c, alg_max9296_alix8b_id);

// 作为1个i2c clent 设备注册
static struct i2c_driver alg_max9296_alix8b_i2c_driver = {
	.driver = {
		.name = "alg-max9296-alix8b",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(alg_max9296_alix8b_of_match),
	},
	.probe = alg_max9296_alix8b_probe,
	.remove = alg_max9296_alix8b_remove,
	.id_table = alg_max9296_alix8b_id,
};

module_i2c_driver(alg_max9296_alix8b_i2c_driver);

MODULE_DESCRIPTION("Media Controller driver for ailiteam max9296 alix8b");
MODULE_AUTHOR("NVIDIA Corporation");
MODULE_LICENSE("GPL v2");
