/*
 * alg-max9296.c - ailiteam max9296 driver
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

#define MAX9296_PHY1_CLK_ADDR 0x320
#define MAX9296_INIT_DESKEW_ADDR 0x443

static const struct of_device_id alg_max9296_of_match[] = {
	{ .compatible = "ailiteam,alg-max9296",},
	{ },
};
MODULE_DEVICE_TABLE(of, alg_max9296_of_match);


static const u32 ctrl_cid_list[] = {
	TEGRA_CAMERA_CID_GAIN,
	TEGRA_CAMERA_CID_EXPOSURE,
	TEGRA_CAMERA_CID_EXPOSURE_SHORT,
	TEGRA_CAMERA_CID_FRAME_RATE,
	//TEGRA_CAMERA_CID_FUSE_ID,
	// TEGRA_CAMERA_CID_HDR_EN,
	//TEGRA_CAMERA_CID_SENSOR_MODE_ID,
};

#define REG_NULL_MAGIC_NUM			0x36578453

struct regval {
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
}deserdes_param_config_t;

struct alg_max9296 {
	struct i2c_client	*i2c_client;
	struct v4l2_subdev	*subdev;
	u32				frame_length;
	s64 last_wdr_et_val;
	struct camera_common_data	*s_data;
	struct tegracam_device		*tc_dev;

	struct camera_common_sensor_ops common_ops;
	struct camera_common_frmfmt frmfmt_table[1];
	int	framerates[1];

	unsigned char i2c_addr;				// i2c地址
	int reset_gpio;						// 复位引脚
    int camera_pwdn_gpio;				// 电源引脚
	const char *subdev_name;			// 设备编号
	struct device_node *companion_dev_node;		// 伴生设备的节点
	int dev_num;						// 设备编号

	int deskew_flag;

	struct task_struct *my_task;		// 用来更新相机模式的内核线程
	deserdes_param_config_t cfg;		// 配置
	struct wait_queue_head waitq;		// 等待消息队列
	bool thread_running;  				// 线程运行标志
	bool cam_mode_update;				// 相机模式更新标志
	spinlock_t lock;					// 保护条件变量struct max9296_mode mode的锁
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

static const struct regval max9296_default_cfg_table[] = 
{
	{0x90,REG_NULL_MAGIC_NUM, 0x00, 0x1608},
};


static int test_mode;
module_param(test_mode, int, 0644);

static inline int alg_max9296_read_reg(struct camera_common_data *s_data,
				u16 addr, u8 *val)
{
	int err = 0;
	u32 reg_val = 0;

	err = regmap_read(s_data->regmap, addr, &reg_val);
	*val = reg_val & 0xFF;

	return err;
}

static int alg_max9296_set_group_hold(struct tegracam_device *tc_dev, bool val)
{
	dev_info(&tc_dev->client->dev,"alg_max9296_set_group_hold \n");
	return 0;
}

static int alg_max9296_set_gain(struct tegracam_device *tc_dev, s64 val)
{
	dev_info(&tc_dev->client->dev,"alg_max9296_set_gain \n");
	return 0;
}

static int alg_max9296_set_frame_rate(struct tegracam_device *tc_dev, s64 val)
{
	dev_info(&tc_dev->client->dev,"alg_max9296_set_frame_rate \n");
	return 0;
}

static int alg_max9296_set_exposure(struct tegracam_device *tc_dev, s64 val)
{
	dev_info(&tc_dev->client->dev,"alg_max9296_set_exposure \n");
	return 0;
}

static struct tegracam_ctrl_ops alg_max9296_ctrl_ops = {
	.numctrls = ARRAY_SIZE(ctrl_cid_list),
	.ctrl_cid_list = ctrl_cid_list,
	.set_gain = alg_max9296_set_gain,
	.set_exposure = alg_max9296_set_exposure,
	.set_exposure_short = alg_max9296_set_exposure,
	.set_frame_rate = alg_max9296_set_frame_rate,
	.set_group_hold = alg_max9296_set_group_hold,
};

static int alg_max9296_power_on(struct camera_common_data *s_data)
{
	// int err = 0;
	// struct camera_common_power_rail *pw = s_data->power;
	// struct camera_common_pdata *pdata = s_data->pdata;
	struct device *dev = s_data->dev;

	dev_info(dev, "%s\n", __func__);
	// if (pdata && pdata->power_on) {
	// 	err = pdata->power_on(pw);
	// 	if (err)
	// 		dev_err(dev, "%s failed.\n", __func__);
	// 	else
	// 		pw->state = SWITCH_ON;
	// 	return err;
	// }

	// pw->state = SWITCH_ON;

	return 0;
}

static int alg_max9296_power_off(struct camera_common_data *s_data)
{
	// int err = 0;
	// struct camera_common_power_rail *pw = s_data->power;
	// struct camera_common_pdata *pdata = s_data->pdata;
	struct device *dev = s_data->dev;

	dev_info(dev, "%s\n", __func__);
// 	if (pdata && pdata->power_off) {
// 		err = pdata->power_off(pw);
// 		if (!err)
// 			goto power_off_done;
// 		else
// 			dev_err(dev, "%s failed.\n", __func__);
// 		return err;
// 	}
// 	/* enter reset mode: XCLR */
// 	usleep_range(1, 2);

// power_off_done:
// 	pw->state = SWITCH_OFF;

	return 0;
}

static int alg_max9296_power_get(struct tegracam_device *tc_dev)
{
	struct camera_common_data *s_data = tc_dev->s_data;
	// struct camera_common_power_rail *pw = s_data->power;
	struct device *dev = s_data->dev;
	dev_info(dev, "%s\n", __func__);
	// pw->state = SWITCH_OFF;

    return 0;
}

static int alg_max9296_power_put(struct tegracam_device *tc_dev)
{
	struct camera_common_data *s_data = tc_dev->s_data;
	struct camera_common_power_rail *pw = s_data->power;

	dev_info(&tc_dev->client->dev, "%s\n", __func__);
	if (unlikely(!pw))
		return -EFAULT;

	return 0;
}

static struct camera_common_pdata *alg_max9296_parse_dt(struct tegracam_device *tc_dev)
{
	struct device *dev = tc_dev->dev;
	struct device_node *np = dev->of_node;
	struct camera_common_pdata *board_priv_pdata;
	const struct of_device_id *match;
	pr_err("alg_max9296_parse_dt \n");
	if (!np)
		return NULL;

	match = of_match_device(alg_max9296_of_match, dev);
	if (!match) {
		dev_err(dev, "Failed to find matching dt id\n");
		return NULL;
	}

	board_priv_pdata = devm_kzalloc(dev,
					sizeof(*board_priv_pdata), GFP_KERNEL);
	if (!board_priv_pdata)
		return NULL;

	return board_priv_pdata;
}

static int alg_max9296_set_mode(struct tegracam_device *tc_dev)
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

	dev_info(&client->dev, "addr(0x%02x) write reg(0x%04x, %d, 0x%04x)\n", \
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

	if (i2c_master_send(client, buf, (val_len + reg_len)) != (val_len + reg_len)) {
		dev_err(&client->dev,
			"%s: writing register 0x%04x from 0x%02x failed\n",
			__func__, reg, client->addr);
		return -EIO;
	}

	return 0;
}

static int max9296_read_reg(struct i2c_client *client, unsigned short addr, unsigned short int reg_rec,unsigned char* val ,unsigned int val_len )
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
		if((reg_length == 0) || (reg_length > 4) || (val_length == 0) || (val_length > 4))
		{
			dev_err(dev, "addr 0x%4x, reg 0x%4x, data 0x%4x mode 0x%4x param input error\n",
			            regs[i].i2c_addr, regs[i].reg, regs[i].val, regs[i].mode);
			continue;
		}
		if((regs[i].i2c_addr == 0) && (regs[i].reg == 0))
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




static int alg_max9296_start_streaming(struct tegracam_device *tc_dev)
{
	struct alg_max9296 *priv = (struct alg_max9296 *)tc_dev->s_data->priv;
	struct device *dev = tc_dev->dev;

	int ret = 0;
	unsigned char val[1]= {0x00};
	unsigned short addr = priv->i2c_addr;
#if 1
		priv->deskew_flag = 0;
		// 读取line速率
		ret = max9296_read_reg(tc_dev->client, priv->i2c_addr, MAX9296_PHY1_CLK_ADDR,  val, 1);
		if ( ret < 0) {
			dev_err(dev, "read line rate err\n");
			return -1;
		}
		dev_info(dev, "read line rate:0x%02x \n",val[0]);
		usleep_range(5000, 10000);
		//mipi clock > 1.5G 校准
		if((val[0]&0x1F) > 0x0f) {
			dev_info(dev, "start deskew \n");
			ret = max9296_read_reg(tc_dev->client, addr, MAX9296_INIT_DESKEW_ADDR,  val, 1);
			if ( ret < 0) {
				return -1;
			}
			// ret = max9296_write_reg(tc_dev->client, addr, 0x313, 2,1,0x00);
			// if(ret <0) {
			// 	dev_info(dev, "deskew err 1\n");
			// 	return -1;
			// }
			// msleep(500);
			usleep_range(50000, 100000);
			if(((val[0]>> 5)&0x1) == 0x1) {
				ret = max9296_write_reg(tc_dev->client, addr, MAX9296_INIT_DESKEW_ADDR, 2,1,0x17);
				if(ret <0) {
					dev_info(dev, "deskew err 1\n");
					return -1;
				}
			} else if (((val[0] >> 5)&0x1) == 0x0) {
				ret = max9296_write_reg(tc_dev->client, addr, MAX9296_INIT_DESKEW_ADDR, 2,1,0x37);
				if(ret <0) {
					dev_info(dev, "deskew err 2\n");
					return -1;
				}
			}
		}
#endif
	dev_info(dev, "start streaming ok\n");
	return 0;
}

static int alg_max9296_stop_streaming(struct tegracam_device *tc_dev)
{
	struct device *dev = tc_dev->dev;
    dev_info(dev, "stop streaming...\n");
	return 0;
}


static int alg_max9296_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "%s:\n", __func__);

	return 0;
}

static const struct v4l2_subdev_internal_ops alg_max9296_subdev_internal_ops = {
	.open = alg_max9296_open,
};

static int max9296_verfy_chipid(struct i2c_client *client, unsigned short addr) {
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


#define DESERDES_MODULE_SET_MODULE_CONFIG    _IOW('V', BASE_VIDIOC_PRIVATE + 0, deserdes_param_config_t)
#define MAX9296_MAX_USE_MODE        10



static long max9296_print_set_param(struct v4l2_subdev *sd, deserdes_param_config_t *config)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	// struct camera_common_data *s_data = to_camera_common_data(&client->dev);
	// struct alg_max9296 *priv = (struct alg_max9296 *)s_data->priv;
	int status = 0;
	// int i = 0;

	if(config == NULL)
	{
		dev_err(&client->dev, "input config is null\n");
		return -1;
	}
	dev_info(&client->dev, "channel %d, width %d, height %d fps_num %d, fps_den %d\n",
			config->channel,
	        config->width,
			config->height,
			config->max_fps.numerator,
			config->max_fps.numerator);
	dev_info(&client->dev, "channel %d, hts %d, vts %d exp %d, link freq %d, bus_fmt %d, bpp %d\n",
			config->channel,
	        config->hts_def,
			config->vts_def,
			config->exp_def,
			config->link_freq,
			config->bus_fmt,
			config->bpp);
	dev_info(&client->dev, "channel %d reg_length %d ,current_reg_length %d ,reg_start_pos %d\n",
			config->channel,
	        config->reg_length,
			config->current_reg_length,
			config->reg_start_pos);

	// for(i = 0; i < (config->current_reg_length); i++)
	// {
	// 	dev_info(&client->dev, "addr 0x%x, reg 0x%x ,val 0x%x, mode 0x%x\n",
	// 			config->reg_list[i].i2c_addr,
	//         	config->reg_list[i].reg,
	// 			config->reg_list[i].val,
	// 			config->reg_list[i].mode);
	// }

	return status;
}

static long max9296_subdev_core_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct camera_common_data *s_data = to_camera_common_data(&client->dev);
	struct alg_max9296 *priv = (struct alg_max9296 *)s_data->priv;
	deserdes_param_config_t *config = (deserdes_param_config_t *)arg;

	dev_info(&client->dev, "max9296_subdev_core_ioctl: cmd:%x \n",cmd);
	switch (cmd) {
	case DESERDES_MODULE_SET_MODULE_CONFIG:
		dev_info(&client->dev, "DESERDES_MODULE_SET_MODULE_CONFIG : set module config width %d, hight %d\n",config->width,config->height);
		ret = max9296_print_set_param(sd, config);
		if(ret != 0)
		{
			dev_err(&client->dev, "channel %d print param error\n",config->channel);
			return -EINVAL;
		}

		spin_lock(&priv->lock);
		memcpy(&priv->cfg,config,sizeof(deserdes_param_config_t));
		priv->cfg.reg_list[priv->cfg.reg_length].reg = REG_NULL_MAGIC_NUM;
		priv->cam_mode_update = true;      	// 设置条件为 true
		spin_unlock(&priv->lock);
		wake_up(&priv->waitq);     			// 唤醒等待队列中的线程

		break;
	default:
		ret = -ENOIOCTLCMD;
		break;
	}
	return ret;

}


struct max9296_dt_info {
	char addr;					// i2c地址
	int reset_gpio;				// 复位引脚
    int camera_pwdn_gpio;		// 电源引脚
	const char *subdev_name;	// 设备编号
	int dev_num;				// 设备编号
	struct device_node *companion_dev_node;		// 伴生设备的节点
};

static unsigned short get_dt_info(struct i2c_client *client,struct max9296_dt_info *info)
{
    struct device_node *node = client->dev.of_node;
    const struct of_device_id *match;
    int ret;
	// int num = 0;
	u32 value;
 	// const char *num_start;

    match = of_match_device(alg_max9296_of_match, &client->dev);
     if (!match) {
	    dev_err(&client->dev, "Failed to find matching dt id.\n");
	          return -1;
    }

    ret = of_property_read_u32(node, "addr_reg", &value);
    if (ret < 0) {
        dev_err(&client->dev, "No addr_reg info\n");
		return ret;
    }
	info->addr = value;
	dev_info(&client->dev, "addr:%d \n",info->addr);

	info->reset_gpio = of_get_named_gpio(node, "reset-gpios", 0);
	if (info->reset_gpio < 0) {
		dev_err(&client->dev, "reset-gpios not found \n");
		return -1;
	}
	dev_info(&client->dev, "reset_gpio : %d \n",info->reset_gpio);

    info->camera_pwdn_gpio = of_get_named_gpio(node, "camera-gpios", 0);
	if (info->camera_pwdn_gpio < 0) {
		dev_err(&client->dev, "camera_pwdn_gpio not found \n");
		return -1;
	}
	dev_info(&client->dev, "camera_pwdn_gpio : %d \n",info->camera_pwdn_gpio);

	ret = of_property_read_string(node, "subdev_name",&info->subdev_name);
	if (ret < 0) {
		dev_err(&client->dev, "devnode property not found\n");
		return ret;
	}
	dev_info(&client->dev, "subdev_name : %s \n",info->subdev_name);
    // 检查字符串长度和数字字符
    if (info->subdev_name == NULL || !isdigit((unsigned char)info->subdev_name[1]) || !isdigit((unsigned char)info->subdev_name[2])) {
        return -EINVAL;
    }
    info->dev_num =  (info->subdev_name[1] - '0') * 10 + (info->subdev_name[2] - '0');
	if(info->dev_num > 8)
	{
		dev_err(&client->dev, "dev_num %d error \n",info->dev_num);
		return -EINVAL;
	}


	info->companion_dev_node = of_parse_phandle(node, "companion_device", 0);
    if (!info->companion_dev_node) {
        dev_err(&client->dev, "Failed to get 'companion_device' phandle\n");
        return -ENODEV;
    }
    return 0;
}


struct v4l2_subdev_core_ops	alg_9296_subdev_core;
struct v4l2_subdev_ops alg_9296_subdev_subdev_ops;



static int reset_tcdev(struct alg_max9296 *priv,deserdes_param_config_t *cfg)
{
	struct i2c_client *client = priv->i2c_client;
	// struct camera_common_data *s_data = priv->s_data;
	struct sensor_signal_properties* signal;
	struct tegracam_device *tc_dev;
	struct device *dev = &priv->i2c_client->dev;
	int i;
	const char * drive_name_backup;
	int ret = 0;
	uint64_t phy_clk = 0;
	dev_info(&client->dev, "reset tcdev\n");
	// 先注销相机
	tegracam_v4l2subdev_unregister(priv->tc_dev);
	tegracam_device_unregister(priv->tc_dev);
	dev_info(&client->dev,"tegracam_v4l2subdev_unregister\n");

	// 实例化相机
	tc_dev = devm_kzalloc(dev,sizeof(struct tegracam_device), GFP_KERNEL);
	if (!tc_dev)
		return -ENOMEM;
	// 记录 client
	tc_dev->client = priv->i2c_client;
	// 拷贝子类信息到父类
	tc_dev->dev = dev;
	strncpy(tc_dev->name, "max9296", sizeof(tc_dev->name));
	tc_dev->dev_regmap_config = &sensor_regmap_config;
	tc_dev->sensor_ops = &priv->common_ops;
	tc_dev->v4l2sd_internal_ops = &alg_max9296_subdev_internal_ops;
	tc_dev->tcctrl_ops = &alg_max9296_ctrl_ops;

	// 相机注册
	ret = tegracam_device_register(tc_dev);
	if (ret) {
		dev_err(&client->dev, "tegra camera driver registration failed\n");
		return ret;
	}
	dev_info(&client->dev,"tegracam_device_register:%d\n",ret);
	// 建立关联数据结构之间关联
	priv->tc_dev = tc_dev;
	priv->s_data = tc_dev->s_data;
	priv->subdev = &tc_dev->s_data->subdev;
	tegracam_set_privdata(tc_dev, (void *)priv);
	spin_lock(&priv->lock);
	priv->s_data->sensor_props.sensor_modes[0].image_properties.width = cfg->width;
	priv->s_data->sensor_props.sensor_modes[0].image_properties.height = cfg->height;
	priv->frmfmt_table[0].size.width = cfg->width;
	priv->frmfmt_table[0].size.height = cfg->height;

#if 1
	for(i=0;i< cfg->current_reg_length;i++)
	{
		if(MAX9296_PHY1_CLK_ADDR == cfg->reg_list[i].reg)
		{
			phy_clk = cfg->reg_list[i].val&0x1f;
			dev_err(&client->dev,"get phy clock:%llu x 100Mbps\n",phy_clk);
		}
	}
	if(phy_clk)
	{
		// if(phy_clk < 15)
		// 	phy_clk = 15;
		// else
		// 	phy_clk = 25;
		signal = &priv->s_data->sensor_props.sensor_modes[0].signal_properties;
		// signal->serdes_pixel_clock.val = 750000000 ;
		// signal->mipi_clock.val = 1125000000 ;
		signal->serdes_pixel_clock.val = phy_clk * 100000000 * 4 /16 ;
		signal->mipi_clock.val = phy_clk*100000000 /2;
		dev_err(&client->dev,"#1 serdes_pixel_clock:%llu mipi_clock:%llu \n",signal->serdes_pixel_clock.val,signal->mipi_clock.val);
	}
	else
	{
		dev_err(&client->dev,"#2 cfg ch:%d w:%d h:%d bpp:%d\n",cfg->channel,cfg->width,cfg->height,cfg->bpp);
	}
	dev_err(&client->dev,"#3 cfg ch:%d w:%d h:%d bpp:%d\n",cfg->channel,cfg->width,cfg->height,cfg->bpp);
#endif
	spin_unlock(&priv->lock);
	dev_info(&client->dev,"tegracam_set_privdata:%d\n",ret);


	// ！！！ 艾利光的sdk会在用户层根据subdevice的名字去判断和要控制的9296节点
	// sub device注册时会使用驱动的的名字，因此这里是临时把驱动的名字修改为自定义的名字，
	// 在subdevice注册之后，再还原回去！！！
	drive_name_backup = client->dev.driver->name;
	client->dev.driver->name = priv->subdev_name;
	ret = tegracam_v4l2subdev_register(tc_dev, true);
	if (ret) {
		pr_err( "tegracam_v4l2subdev_register failed\n");
		tegracam_device_unregister(tc_dev);
		return ret;
	}
	client->dev.driver->name = drive_name_backup;
	// ！！！ 这里是把tegra subdevice 的ops做复制和替换，在ops->core->ioctl加入自定义的函数：max9296_subdev_core_ioctl
	memcpy(&alg_9296_subdev_core,tc_dev->s_data->subdev.ops->core,sizeof(struct v4l2_subdev_core_ops)) ;
	memcpy(&alg_9296_subdev_subdev_ops,tc_dev->s_data->subdev.ops,sizeof(struct v4l2_subdev_ops)) ;
	alg_9296_subdev_core.ioctl = max9296_subdev_core_ioctl;
	alg_9296_subdev_subdev_ops.core = &alg_9296_subdev_core;
	tc_dev->s_data->subdev.ops = &alg_9296_subdev_subdev_ops;

	priv->deskew_flag = 1;

	dev_info(&client->dev,"tegracam_v4l2subdev_register ok... \n");
	return ret;
}


static struct alg_max9296 * get_max9296_companion_dev(struct alg_max9296 * priv)
{
	struct i2c_client * client = NULL;
	struct camera_common_data *s_data = NULL;
	struct alg_max9296 *priv2 = NULL;

	if(priv->companion_dev_node == NULL || priv == NULL)
	{
		pr_err("get_max9296_companion_dev input para is null\n");
		return NULL;
	}
	client = of_find_i2c_device_by_node(priv->companion_dev_node);
	if (!client) {
		dev_err(&priv->i2c_client->dev, "Failed to find companion device\n");
		return NULL;
	}

	s_data = to_camera_common_data(&client->dev);
	if(!s_data) {
		dev_err(&client->dev, "Failed to get camera_common_data\n");
		return NULL;
	}
	priv2 = (struct alg_max9296 *)s_data->priv;
	if(!priv2) {
		dev_err(&client->dev, "Failed to get alg_max9296\n");
		return NULL;
	}
	dev_info(&client->dev,"get_max9296_companion_dev: %s, %d\n",priv2->subdev_name,priv2->i2c_addr);

	return priv2;
}


static int max9296_kthread_func(void *data) 
{
	int err;
	struct alg_max9296 *priv = data;
	struct tegracam_device *tc_dev = priv->tc_dev;
	struct alg_max9296 *priv2 = NULL ;
	const char * drive_name_backup;
	struct i2c_client *client = tc_dev->client;

	dev_info(&tc_dev->client->dev, "max9296_kthread_func: Running...\n");

	msleep(100*priv->dev_num);

	// ！！！ 艾利光的sdk会在用户层根据subdevice的名字去判断和要控制的9296节点
	// sub device注册时会使用驱动的的名字，因此这里是临时把驱动的名字修改为自定义的名字，
	// 在subdevice注册之后，再还原回去！！！
	drive_name_backup = client->dev.driver->name;
	client->dev.driver->name = priv->subdev_name;
	// 注册标准V4L2相机为subdevice
	err = tegracam_v4l2subdev_register(tc_dev, true);
	if (err) {
		dev_err(&client->dev, "tegra camera subdev registration failed\n");
		return err;
	}
	client->dev.driver->name = drive_name_backup;

	// ！！！ 这里是把tegra subdevice 的ops做复制和替换，在ops->core->ioctl加入自定义的函数：max9296_subdev_core_ioctl
	memcpy(&alg_9296_subdev_core,tc_dev->s_data->subdev.ops->core,sizeof(struct v4l2_subdev_core_ops)) ;
	memcpy(&alg_9296_subdev_subdev_ops,tc_dev->s_data->subdev.ops,sizeof(struct v4l2_subdev_ops)) ;
	alg_9296_subdev_core.ioctl = max9296_subdev_core_ioctl;
	alg_9296_subdev_subdev_ops.core = &alg_9296_subdev_core;
	tc_dev->s_data->subdev.ops = &alg_9296_subdev_subdev_ops;
	dev_info(&client->dev, "tegracam_v4l2subdev_register ok... \n");
	priv->deskew_flag = 1;

	priv->thread_running = true;
	while (!kthread_should_stop() || priv->thread_running) 
	{
		// 等待消息或停止信号
        err = wait_event_interruptible(priv->waitq,priv->cam_mode_update || !priv->thread_running);
        if (err == -ERESTARTSYS) {
            dev_err(&tc_dev->client->dev,"Thread interrupted\n");
            break;
        }


        if (priv->cam_mode_update) 
		{
			dev_info(&tc_dev->client->dev,"cam_mode_update config width %d, hight %d\n",priv->cfg.width,priv->cfg.height);
			msleep(500);
			// 重新注册相机
			err = reset_tcdev(priv,&priv->cfg);
			if (err) {
				dev_err(&tc_dev->client->dev,"Failed to reset tcdev\n");
				goto err_reset;
			}
			// 重新注册伴生设备
			priv2 = get_max9296_companion_dev(priv);
			if( priv2 )
			{
				err = reset_tcdev(priv2,&priv->cfg);
				if (err) {
					dev_err(&tc_dev->client->dev,"Failed to reset companion tcdev\n");
					goto err_reset;
				}
			}

			// 复位相机
			gpio_set_value(priv->reset_gpio, 0);
			gpio_set_value(priv->camera_pwdn_gpio, 0);
			msleep(500);
			gpio_set_value(priv->reset_gpio, 1);
			gpio_set_value(priv->camera_pwdn_gpio, 1);
			msleep(100);
			err = max9296_write_array(tc_dev->client,priv->cfg.reg_list);
			if(err ) {
				dev_err(&tc_dev->client->dev, "Failed to write reg array\n");
			}
err_reset:
        	spin_lock(&priv->lock);
            priv->cam_mode_update = false;
        	spin_unlock(&priv->lock);
        }
    }
    printk(KERN_INFO "my_kthread: Exiting\n");
    return 0;
}


static int alg_max9296_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct tegracam_device *tc_dev;
	struct alg_max9296 *priv;
	int err;

	struct max9296_dt_info dt_info;

	dev_info(&client->dev,"start alg_max9296_probe \n");

	if (!IS_ENABLED(CONFIG_OF) || !client->dev.of_node)
		return -EINVAL;

	if(get_dt_info(client,&dt_info))
	    return -EINVAL;

	// 上电并首先检查芯片id , 确定max9296连接正常
	gpio_set_value(dt_info.reset_gpio, 1);
    gpio_set_value(dt_info.camera_pwdn_gpio, 1);
	usleep_range(5000, 5100);
    if(!max9296_verfy_chipid(client,dt_info.addr)){
	    dev_err(&client->dev, "cann't find max9296!\n");
	    return -ENODEV;
    }

	// 实例化 max9296
	priv = devm_kzalloc(dev,
			sizeof(struct alg_max9296), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;
	priv->i2c_addr = dt_info.addr;
	priv->reset_gpio = dt_info.reset_gpio;
	priv->camera_pwdn_gpio = dt_info.camera_pwdn_gpio;
	priv->subdev_name = dt_info.subdev_name;
	priv->companion_dev_node = dt_info.companion_dev_node;
	priv->dev_num = dt_info.dev_num;
	priv->subdev_name = dt_info.subdev_name;

	// 实例化相机
	tc_dev = devm_kzalloc(dev,sizeof(struct tegracam_device), GFP_KERNEL);
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
	tc_dev->sensor_ops = &priv->common_ops;
 	priv->common_ops.power_on = alg_max9296_power_on,
 	priv->common_ops.power_off = alg_max9296_power_off,
 	//priv->common_ops.write_reg = alg_max9296_write_reg,
 	priv->common_ops.read_reg = alg_max9296_read_reg,
 	priv->common_ops.parse_dt = alg_max9296_parse_dt,
 	priv->common_ops.power_get = alg_max9296_power_get,
 	priv->common_ops.power_put = alg_max9296_power_put,
 	priv->common_ops.set_mode = alg_max9296_set_mode,
 	priv->common_ops.start_streaming = alg_max9296_start_streaming,
 	priv->common_ops.stop_streaming = alg_max9296_stop_streaming,
	priv->common_ops.frmfmt_table = priv->frmfmt_table;
	priv->common_ops.numfrmfmts = ARRAY_SIZE(priv->frmfmt_table);
	priv->frmfmt_table->framerates = priv->framerates;
	priv->frmfmt_table[0].num_framerates = ARRAY_SIZE(priv->framerates);
	priv->frmfmt_table[0].size.width = 1920;
	priv->frmfmt_table[0].size.height = 1080;
	priv->frmfmt_table[0].hdr_en = 0;
	priv->frmfmt_table[0].mode = 0;
	priv->framerates[0] = 30;


	// 操作接口注册
	tc_dev->v4l2sd_internal_ops = &alg_max9296_subdev_internal_ops;
	tc_dev->tcctrl_ops = &alg_max9296_ctrl_ops;

	// 相机注册
	err = tegracam_device_register(tc_dev);
	if (err) {
		dev_err(dev, "tegra camera driver registration failed\n");
		return err;
	}

	// 建立关联数据结构之间关联
	priv->tc_dev = tc_dev;
	priv->s_data = tc_dev->s_data;
	priv->subdev = &tc_dev->s_data->subdev;
	tegracam_set_privdata(tc_dev, (void *)priv);
#if 0
	// ！！！ 艾利光的sdk会在用户层根据subdevice的名字去判断和要控制的9296节点
	// sub device注册时会使用驱动的的名字，因此这里是临时把驱动的名字修改为自定义的名字，
	// 在subdevice注册之后，再还原回去！！！
	drive_name_backup = client->dev.driver->name;
	client->dev.driver->name = dt_info.subdev_name;
	// 注册标准V4L2相机为subdevice
	err = tegracam_v4l2subdev_register(tc_dev, true);
	if (err) {
		dev_err(dev, "tegra camera subdev registration failed\n");
		return err;
	}
	client->dev.driver->name = drive_name_backup;

	// ！！！ 这里是把tegra subdevice 的ops做复制和替换，在ops->core->ioctl加入自定义的函数：max9296_subdev_core_ioctl
	memcpy(&alg_9296_subdev_core,tc_dev->s_data->subdev.ops->core,sizeof(struct v4l2_subdev_core_ops)) ;
	memcpy(&alg_9296_subdev_subdev_ops,tc_dev->s_data->subdev.ops,sizeof(struct v4l2_subdev_ops)) ;
	alg_9296_subdev_core.ioctl = max9296_subdev_core_ioctl;
	alg_9296_subdev_subdev_ops.core = &alg_9296_subdev_core;
	tc_dev->s_data->subdev.ops = &alg_9296_subdev_subdev_ops;
	dev_info(dev, "tegracam_v4l2subdev_register ok... \n");
#endif
	// 初始化内核线程
	init_waitqueue_head(&priv->waitq);
	priv->cam_mode_update = false;
	spin_lock_init(&priv->lock);
    priv->my_task = kthread_run(max9296_kthread_func, priv, "max9296_kthread");
    if (IS_ERR(priv->my_task)) {
        printk(KERN_ERR "Failed to start thread\n");
        return PTR_ERR(priv->my_task);
    }

	memcpy(priv->cfg.reg_list , &max9296_default_cfg_table, sizeof(max9296_default_cfg_table));
	priv->cfg.reg_length =  ARRAY_SIZE(max9296_default_cfg_table);
	dev_info(dev, "sucess to start thread\n");

	return 0;
}

static int
alg_max9296_remove(struct i2c_client *client)
{
	struct camera_common_data *s_data = to_camera_common_data(&client->dev);
	struct alg_max9296 *priv = (struct alg_max9296 *)s_data->priv;

    if (priv->my_task) {
		priv->thread_running = false;
		wake_up(&priv->waitq);
        kthread_stop(priv->my_task);
        priv->my_task = NULL;
    }
	// 下电，使得复位
	gpio_set_value(priv->reset_gpio, 0);
    gpio_set_value(priv->camera_pwdn_gpio, 0);
	tegracam_v4l2subdev_unregister(priv->tc_dev);
	tegracam_device_unregister(priv->tc_dev);

	return 0;
}

static const struct i2c_device_id alg_max9296_id[] = {
	{ "alg-max9296", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, alg_max9296_id);

// 作为1个i2c clent 设备注册
static struct i2c_driver alg_max9296_i2c_driver = {
	.driver = {
		.name = "alg-max9296",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(alg_max9296_of_match),
	},
	.probe = alg_max9296_probe,
	.remove = alg_max9296_remove,
	.id_table = alg_max9296_id,
};

module_i2c_driver(alg_max9296_i2c_driver);

MODULE_DESCRIPTION("Media Controller driver for ailiteam max9296");
MODULE_AUTHOR("NVIDIA Corporation");
MODULE_LICENSE("GPL v2");
