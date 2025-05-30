/**
 * @file trigger.c
 * @brief
 * @author mark (mark@ailiteam.com)
 * @version 1.0
 * @date 2024-03-09
 *
 * @copyright Copyright (c) 2024  ailiteam
 *
 * @par 修改日志:
 */

#include "trigger.h"
#include "asm-generic/errno-base.h"
#include "asm/string.h"
#include "linux/dynamic_debug.h"
#include "linux/err.h"
#include "linux/fs.h"
#include "linux/dev_printk.h"
#include "linux/gpio/consumer.h"
#include "linux/interrupt.h"
#include "linux/kern_levels.h"
#include "linux/kernel.h"
#include "linux/limits.h"
#include "linux/list.h"
#include "linux/minmax.h"
#include "linux/timer.h"
#include "linux/types.h"
#include "linux/of.h"
#include "trigger_export.h"

static aili_trigger_t *aili_tirgger_g = NULL;
static aili_slave_control_node_t control_node[MAX_CONTORL_NODE_NUMBER];
static aili_slave_control_node_param_t param_node[MAX_PARAM_NODE_NUMBER];
static aili_slave_trigger_io_node_param io_node_param[MAX_PARAM_NODE_NUMBER];
aili_trigger_param_t default_trigger_pram =
{
    .master_trigger_mode = AILI_MASTER_TRIGGER_INTER_TRG_MODE,
    .master_mode_timer_param =
    {
        .output_hz = 30,
        .output_period_us = 33333,
    },
    .slave_trigger_class[0] =
    {
        .channel = 0,
        .valid = 1,
        .control_param =
        {
            .trigger_delay_time_us = 1000,
            .trigger_valid_time_us = 1000,
            .trigger_polarity = 0,
        },
    },
    .slave_trigger_class[1] =
    {
        .channel = 1,
        .valid = 1,
        .control_param =
        {
            .trigger_delay_time_us = 2000,
            .trigger_valid_time_us = 1000,
            .trigger_polarity = 0,
        },
    },
    .slave_trigger_class[2] =
    {
        .channel = 2,
        .valid = 1,
        .control_param =
        {
            .trigger_delay_time_us = 3000,
            .trigger_valid_time_us = 1000,
            .trigger_polarity = 0,
        },
    },
    .slave_trigger_class[3] =
    {
        .channel = 3,
        .valid = 1,
        .control_param =
        {
            .trigger_delay_time_us = 4000,
            .trigger_valid_time_us = 1000,
            .trigger_polarity = 0,
        },
    },
    .slave_trigger_class[4] =
    {
        .channel = 4,
        .valid = 1,
        .control_param =
        {
            .trigger_delay_time_us = 5000,
            .trigger_valid_time_us = 1000,
            .trigger_polarity = 0,
        },
    },
    .slave_trigger_class[5] =
    {
        .channel = 5,
        .valid = 1,
        .control_param =
        {
            .trigger_delay_time_us = 6000,
            .trigger_valid_time_us = 1000,
            .trigger_polarity = 0,
        },
    },
    .slave_trigger_class[6] =
    {
        .channel = 6,
        .valid = 1,
        .control_param =
        {
            .trigger_delay_time_us = 7000,
            .trigger_valid_time_us = 1000,
            .trigger_polarity = 0,
        },
    },
    .slave_trigger_class[7] =
    {
        .channel = 7,
        .valid = 1,
        .control_param =
        {
            .trigger_delay_time_us = 8000,
            .trigger_valid_time_us = 1000,
            .trigger_polarity = 0,
        },
    },
};

static int aili_set_trigger_param(aili_trigger_t *cb, aili_trigger_param_t trigger_param, int param_count)
{
    int status = 0;
    int i = 0;
    uint32_t channel = 0;
    struct device *dev = &cb->pdev->dev;
    if(IS_ERR_OR_NULL(cb))
    {
        dev_err(dev, "[aili_trigger]input cb is null\n");
        return -EPERM;
    }
    // memset(&cb->trigger_param, 0, sizeof(aili_trigger_param_t));
    cb->trigger_param.master_trigger_mode = trigger_param.master_trigger_mode;
    cb->trigger_param.master_mode_timer_param = trigger_param.master_mode_timer_param;
    if(param_count > MAX_OUT_TRIGGER_NUMBER)
    {
        dev_err(dev, "[aili_trigger]input cb is null\n");
        return -EPERM;
    }
    dev_info(dev, "trigger mode %d, HZ %d period %d\n", cb->trigger_param.master_trigger_mode, cb->trigger_param.master_mode_timer_param.output_hz, cb->trigger_param.master_mode_timer_param.output_period_us);
    for(i = 0; i < param_count; i++)
    {
        if((trigger_param.slave_trigger_class[i].valid == 1)
           && (trigger_param.slave_trigger_class[i].channel < cb->trigger_gpio_num))
        {
            channel = trigger_param.slave_trigger_class[i].channel;
            cb->trigger_param.slave_trigger_class[channel].channel = channel;
            cb->trigger_param.slave_trigger_class[channel].valid = trigger_param.slave_trigger_class[i].valid;
            cb->trigger_param.slave_trigger_class[channel].control_param = trigger_param.slave_trigger_class[i].control_param;
            dev_info(dev, "param channel %d, valid %d, delay %d width %d polarity %d \n",
                    cb->trigger_param.slave_trigger_class[channel].channel,
                    cb->trigger_param.slave_trigger_class[channel].valid,
                    cb->trigger_param.slave_trigger_class[channel].control_param.trigger_delay_time_us,
                    cb->trigger_param.slave_trigger_class[channel].control_param.trigger_valid_time_us,
                    cb->trigger_param.slave_trigger_class[channel].control_param.trigger_polarity);
        }
    }

    return status;
}

static int aili_find_valid_param_node(struct device *dev, aili_slave_control_node_param_t **param)
{
    int i = 0;
    if(IS_ERR_OR_NULL(param))
    {
        dev_err(dev, "[aili_trigger]input node param error\n");
        return -1;
    }
    for(i = 0; i < MAX_PARAM_NODE_NUMBER; i++)
    {
        if(param_node[i].valid == 0)
        {
            break;
        }
    }
    if(i == MAX_PARAM_NODE_NUMBER)
    {
        dev_err(dev, "[aili_trigger]not find valid param node\n");
        return -1;
    }
    *param = &param_node[i];
    return i;
}

static int aili_find_valid_contorl_node(struct device *dev, aili_slave_control_node_t **use_control_node)
{
    int status = 0;
    int i = 0;
    if(IS_ERR_OR_NULL(use_control_node))
    {
        dev_err(dev, "[aili_trigger]input node param error\n");
        return -ENOMEM;
    }
    for(i = 0; i < MAX_CONTORL_NODE_NUMBER; i++)
    {
        if(control_node[i].valid == 0)
        {
            break;
        }
    }
    if(i == MAX_CONTORL_NODE_NUMBER)
    {
        dev_err(dev, "[aili_trigger]not find valid param node\n");
        return -EPERM;
    }
    *use_control_node = &control_node[i];
    return status;
}

static int aili_slave_trigger_init_param_node(struct device *dev, node_dispos_handler dispose_handler,aili_slave_trigger_io_node_param **dispose_param,aili_slave_control_node_param_t **param)
{
    int status = 0;
    int idx = -1;
    aili_slave_control_node_param_t *control_param = NULL;
    if(IS_ERR_OR_NULL(dev))
    {
        dev_err(dev, "[aili_trigger]input dev failed\n");
        return -EPERM;
    }

    if(IS_ERR_OR_NULL(param) || IS_ERR_OR_NULL(dispose_param))
    {
        dev_err(dev, "[aili_trigger]input node param error\n");
        return -EPERM;
    }
    idx = aili_find_valid_param_node(dev, &control_param);
    if(IS_ERR_OR_NULL(control_param) || (idx < 0) || (idx >=MAX_CONTORL_NODE_NUMBER))
    {
        dev_err(dev, "[aili_trigger]param node idx %d failed\n", idx);
        return -EPERM;
    }

    memset(control_param, 0, sizeof(aili_slave_control_node_param_t));
    control_param->valid = 1;
    control_param->dispose_handler = dispose_handler;
    control_param->dispose_param = (void *)&io_node_param[idx];
    INIT_LIST_HEAD(&control_param->param_node);
    *param = control_param;
    *dispose_param = &io_node_param[idx];
    return status;
}

static int aili_slave_trigger_init_control_node(struct device *dev, uint32_t abs_control_time, uint32_t rel_control_time,aili_slave_control_node_t **control_node)
{
    int status = 0;
    aili_slave_control_node_t *node = NULL;
    if(IS_ERR_OR_NULL(dev))
    {
        dev_err(dev, "[aili_trigger]input dev failed\n");
        return -ENOMEM;
    }

    if(IS_ERR_OR_NULL(control_node))
    {
        dev_err(dev, "[aili_trigger]input control node error\n");
        return -ENOMEM;
    }
    status = aili_find_valid_contorl_node(dev, control_node);
    if(IS_ERR_OR_NULL(*control_node) || (status < 0))
    {
        dev_err(dev, "[aili_trigger]contorl node malloc failed\n");
        return -ENOMEM;
    }
    node  = *control_node;
    memset(node, 0, sizeof(aili_slave_control_node_t));
    node->time_control_time = rel_control_time;
    node->time_control_abs_time = abs_control_time;
    node->valid = 1;
    INIT_LIST_HEAD(&node->param_head);
    INIT_LIST_HEAD(&node->run_node);
    return status;
}

static int aili_slave_trigger_register_control_node(aili_trigger_t *cb, uint32_t abs_control_time, uint32_t rel_time, aili_slave_control_node_param_t *node_param)
{
    int status = 0;
    struct device *dev = &cb->pdev->dev;
    aili_slave_control_node_t *control_node = NULL;
    aili_slave_control_node_t *init_control_node = NULL;
    aili_slave_control_node_t *exist_node = NULL;
    uint32_t node_abs_control_time = 0;
    uint32_t max_value = 0;
    uint32_t min_value = 0;
    int64_t value = 0;
    if(IS_ERR_OR_NULL(node_param))
    {
        dev_err(dev, "[aili_trigger]input node param is null\n");
        return -ENOMEM;
    }

    list_for_each_entry(control_node, &cb->run_node_head, run_node) {
        node_abs_control_time = control_node->time_control_abs_time;
        value = (int64_t)node_abs_control_time - MAX_CONTROL_TIME_DIFF_US;
        min_value = value < 0 ? 0 : value;
        value = (int64_t)node_abs_control_time + MAX_CONTROL_TIME_DIFF_US;
        max_value = value > U32_MAX ? U32_MAX : value;
        if((abs_control_time >= min_value) && (abs_control_time <= max_value))
        {
            exist_node = control_node;
            break;
        }
    }

    if(!exist_node)
    {
        status = aili_slave_trigger_init_control_node(dev, abs_control_time,rel_time, &init_control_node);
        if(IS_ERR_OR_NULL(init_control_node) || status)
        {
            dev_err(dev, "[aili_trigger]creat param node error error \n");
            return status;
        }
        if(list_empty(&cb->run_node_head))
        {
            list_add(&init_control_node->run_node, &cb->run_node_head);
        }
        else
        {
            list_for_each_entry(control_node, &cb->run_node_head, run_node) {
                node_abs_control_time = control_node->time_control_abs_time;
                if(abs_control_time > node_abs_control_time)
                    break;
            }
            if(IS_ERR_OR_NULL(control_node))
            {
                dev_err(dev, "[aili_trigger]control node is null \n");
            }
            list_add_tail(&init_control_node->run_node, &control_node->run_node);
        }
        list_add(&node_param->param_node, &init_control_node->param_head);
    }
    else
    {
        list_add(&node_param->param_node, &exist_node->param_head);
    }

    return status;
}


static int aili_trigger_reset_node(aili_trigger_t *cb)
{
    int status = 0;
    struct device *dev = &cb->pdev->dev;
    if(IS_ERR_OR_NULL(cb))
    {
        dev_err(dev, "[aili_trigger]input cb is null\n");
        return -EPERM;
    }
    memset(control_node, 0, sizeof(control_node));
    memset(param_node, 0, sizeof(param_node));
    memset(io_node_param, 0, sizeof(io_node_param));
    INIT_LIST_HEAD(&(aili_tirgger_g->run_node_head));
    INIT_LIST_HEAD(&(aili_tirgger_g->release_node_head));
    cb->control_node_param_num = 0;
    dev_info(dev,"reset all node param successful!!\n");
    return status;
}

static void aili_slave_trigger_control_dispose_handler(void *dispose_param)
{
    aili_slave_trigger_io_node_param *node_param = (aili_slave_trigger_io_node_param *)dispose_param;
    if(node_param == NULL)
    {
        return;
    }
    gpiod_set_value_cansleep(node_param->io_param,node_param->value);
}

static int aili_trigger_set_and_register_param_node(aili_trigger_t *cb, aili_slave_trigger_control_class_t* control_class)
{
    int status = 0;
    uint32_t on_time = 0;
    uint32_t off_time = 0;
    struct device *dev;
    aili_slave_trigger_io_node_param* dispose_param[2] = {NULL, NULL};
    aili_slave_control_node_param_t* node_param[2] = {NULL, NULL};
    int i = 0;
    int  first_value = 0;

    dev = &cb->pdev->dev;

    if(IS_ERR_OR_NULL(control_class))
    {
        dev_err(dev, "[aili_trigger]control class is NULL\n");
        return -EPERM;
    }
    if((control_class->control_param.trigger_polarity >= AILI_SLAVE_TRIGGER_MAX) || !(control_class->valid))
    {
        dev_err(dev, "[aili_trigger]not support trigger mode %d valid %d\n", control_class->control_param.trigger_polarity, control_class->valid);
        return -EPERM;
    }
    if(IS_ERR_OR_NULL(control_class->output_io))
    {
        dev_err(dev, "[aili_trigger]channel %d output io is null\n",control_class->channel);
        return -EPERM;
    }
    on_time = control_class->control_param.trigger_delay_time_us;
    off_time = control_class->control_param.trigger_valid_time_us;
    off_time += on_time;
    if(off_time < on_time)
    {
        off_time = U32_MAX;
    }
    for(i = 0; i < sizeof(dispose_param)/sizeof(aili_slave_trigger_io_node_param*); i++)
    {
        status = aili_slave_trigger_init_param_node(dev,aili_slave_trigger_control_dispose_handler, &dispose_param[i], &node_param[i]);
        if(status || IS_ERR_OR_NULL(dispose_param) || IS_ERR_OR_NULL(node_param))
        {
            dev_err(dev, "[aili_trigger]get param error \n");
            return -EPERM;
        }
        cb->control_node_param_num++;
        dev_info(dev, "currnt cuont %d, total count %d", i, cb->control_node_param_num);
    }
    // if(node_param[0]->dispose_param == NULL)
    // {
    //     dev_info(dev,"dispose param is null");
    // }
    first_value = (control_class->control_param.trigger_polarity == AILI_SLAVE_TRIGGER_POSITIVE) ? 1 : 0;
    gpiod_set_value_cansleep(control_class->output_io,first_value);
    dispose_param[0]->io_param = control_class->output_io;
    dispose_param[0]->value = (~first_value) & 0x00000001;
    node_param[0]->absolute_time = on_time;
    dispose_param[1]->io_param = control_class->output_io;
    dispose_param[1]->value = first_value;
    node_param[1]->absolute_time = off_time;
    dev_info(dev, "0 io %d value %d, abs_time %d", desc_to_gpio(dispose_param[0]->io_param), dispose_param[0]->value, node_param[0]->absolute_time);
    dev_info(dev, "1 io %d value %d, abs_time %d", desc_to_gpio(dispose_param[1]->io_param), dispose_param[1]->value, node_param[1]->absolute_time);
    // for(i = 0; i < MAX_CONTORL_NODE_NUMBER; i++)
    // {
    //     if(param_node[i].dispose_param == NULL)
    //     {
    //         dev_info(dev, "dispose param is NULL\n");
    //     }
    // }
    return status;
}

static int aili_trigger_sort_param_abs_time(struct device *dev, uint32_t param_count, uint32_t *min_time, uint32_t *max_time)
{
    int status = 0;
    int i = 0;
    int j = 0;
    aili_slave_control_node_param_t temp_node;
    aili_slave_trigger_io_node_param *node_param = NULL;
    if((param_count <= 0) || (param_count > MAX_PARAM_NODE_NUMBER))
    {
        dev_err(dev, "[aili_trigger] param_count %d error,please check\n", param_count);
        return -EPERM;
    }
    for(i = 0; i < (param_count - 1); i++)
    {
        for(j = 0;j < (param_count - 1 - i); j++)
        {
            if((param_node[j].absolute_time) > (param_node[j + 1].absolute_time))
            {
                temp_node = param_node[j];
                param_node[j] = param_node[j + 1];
                param_node[j + 1] = temp_node;
            }
        }
    }
    *min_time = param_node[0].absolute_time;
    *max_time = param_node[param_count - 1].absolute_time;
    dev_info(dev, "min time %d ,max time %d\n", *min_time, *max_time);
    for(i = 0; i < param_count; i++)
    {
        node_param = (aili_slave_trigger_io_node_param*)(param_node[i].dispose_param);
        if(node_param == NULL)
        {
            dev_info(dev, "node %d param is null\r\n",i);
            return -EPERM;
        }
        dev_info(dev,"node %d, valid %d time %d, io %d, value %d,\r\n",
                i, param_node[i].valid, param_node[i].absolute_time,
                desc_to_gpio(node_param->io_param),
                node_param->value);
    }
    return status;
}

static int aili_trigger_cal_rel_time(aili_trigger_t *cb, int param_count)
{
    int status = 0;
    struct device *dev = &cb->pdev->dev;
    uint32_t value;
    int i = 0;
    if((param_count <= 0) || (param_count > MAX_PARAM_NODE_NUMBER))
    {
        dev_err(dev, "[aili_trigger] param_count %d error,please check\n", param_count);
        return -EPERM;
    }
    param_node[0].relative_time = param_node[0].absolute_time;
    for(i = 1; i < param_count; i++)
    {
        if(param_node[i - 1].absolute_time > param_node[i].absolute_time)
        {
            dev_err(dev, "[aili_trigger] node %d time %d > node %d time %d\n", i-1, param_node[i-1].absolute_time, i, param_node[i].absolute_time);
            return -EPERM;
        }
        value = param_node[i].absolute_time - param_node[i - 1].absolute_time;
        if(!value)
        {
            param_node[i].relative_time = param_node[i - 1].relative_time;
        }
        else
        {
            param_node[i].relative_time = value;
        }
    }
    for(i = 0; i < param_count; i++)
    {
        dev_info(dev, "channel %d, abs_time %d, rel_time %d\n", i, param_node[i].absolute_time,  param_node[i].relative_time);
    }
    return status;
}

static int aili_reg_all_node_control_param(aili_trigger_t *cb)
{
    int status = 0;
    struct device *dev = &cb->pdev->dev;
    int i = 0;
    aili_slave_control_node_t* control_node = NULL;
    aili_slave_control_node_param_t *node_param = NULL;
    if(IS_ERR_OR_NULL(cb))
    {
        dev_err(dev, "[aili_trigger]input cb is null\n");
        return -EPERM;
    }
    if((cb->control_node_param_num <= 0) || (cb->control_node_param_num > MAX_PARAM_NODE_NUMBER))
    {
        dev_err(dev, "[aili_trigger]param_count %d error,please check\n", cb->control_node_param_num);
        return -EPERM;
    }

    for(i = 0; i < cb->control_node_param_num; i++)
    {
        status = aili_slave_trigger_register_control_node(cb, param_node[i].absolute_time, param_node[i].relative_time, &param_node[i]);
        if(status)
        {
            dev_err(dev, "[aili_trigger]param node %d register control node error,please check\n", i);
            return -EPERM;
        }
    }
    list_for_each_entry_reverse(control_node, &cb->run_node_head, run_node)
    {
        dev_info(dev, "valid %d, abs_time %d ,rel time %d\n",control_node->valid, control_node->time_control_abs_time, control_node->time_control_time);
        list_for_each_entry(node_param, &control_node->param_head, param_node)
        {
            dev_info(dev, "absolute_time %d, relative_time %d, io %d, value %d \n",
                     node_param->absolute_time,
                     node_param->relative_time,
                     desc_to_gpio(((aili_slave_trigger_io_node_param *)(node_param->dispose_param))->io_param),
                     ((aili_slave_trigger_io_node_param *)(node_param->dispose_param))->value);
        }
    }
    return status;
}

static int aili_trigger_set_inter_left_time(struct device *dev, uint32_t max_abs_time, aili_slave_trigger_master_mode_timer_param_t *master_mode_timer_param)
{
    int status = 0;
    if(IS_ERR_OR_NULL(master_mode_timer_param))
    {
        dev_err(dev, "[aili_trigger]input cb is null\n");
        return -EPERM;
    }
    if((max_abs_time == 0) || (master_mode_timer_param->output_hz == 0) || (master_mode_timer_param->output_period_us < max_abs_time))
    {
        dev_err(dev, "input param error, max abs time %d, output_period_us %d ,max_abs_time %d\n", max_abs_time, master_mode_timer_param->output_hz, max_abs_time);
        return -EPERM;
    }
    master_mode_timer_param->remain_time_run_time = master_mode_timer_param->output_period_us - max_abs_time;
    dev_info(dev, "inter left time %d \n", master_mode_timer_param->remain_time_run_time);
    return status;
}

static int aili_trigger_all_node_init(aili_trigger_t *cb, aili_trigger_param_t trigger_param, int param_count)
{
    int status = 0;
    int gpio_num_count = 0;
    struct device *dev = &cb->pdev->dev;
    int i = 0;
    uint32_t min_abs_time = 0;
    uint32_t max_abs_time = 0;
    if(IS_ERR_OR_NULL(cb))
    {
        dev_err(dev, "[aili_trigger]input cb is null\n");
        return -EPERM;
    }

    status = aili_trigger_reset_node(cb);
    if(status)
    {
        dev_err(dev, "[aili_trigger]rest node error\n");
        return -EPERM;
    }
    gpio_num_count = cb->trigger_gpio_num;
    if((param_count > gpio_num_count) || (!param_count) || (!gpio_num_count))
    {
        dev_err(dev, "[aili_trigger]param_count %d or gpio count %d error\n", param_count, gpio_num_count);
        return -EPERM;
    }
    dev_info(dev,"valid gpio num count %d, param_count %d\n", gpio_num_count, param_count);
    status = aili_set_trigger_param(cb, trigger_param, param_count);
    if(status)
    {
        dev_err(dev, "[aili_trigger]set param error\n");
        return -EPERM;
    }

    for(i = 0; i < MAX_OUT_TRIGGER_NUMBER; i++)
    {
        if(!(cb->trigger_param.slave_trigger_class[i].valid))
        {
            continue;
        }
        status = aili_trigger_set_and_register_param_node(cb, &(cb->trigger_param.slave_trigger_class[i]));
        if(status)
        {
            dev_err(dev, "[aili_trigger]set and register param node error\n");
            return -EPERM;
        }
    }

    status = aili_trigger_sort_param_abs_time(dev, cb->control_node_param_num, &min_abs_time, &max_abs_time);
    if(status)
    {
        dev_err(dev, "[aili_trigger]sort abs time error\n");
        return -EPERM;
    }

    status = aili_trigger_set_inter_left_time(dev, max_abs_time, &cb->trigger_param.master_mode_timer_param);
    if(status)
    {
        dev_err(dev, "set inter_left_time error\n");
        return -EPERM;
    }

    status = aili_trigger_cal_rel_time(cb, cb->control_node_param_num);
    if(status)
    {
        dev_err(dev, "[aili_trigger]cal_rel_time_error\n");
        return -EPERM;
    }

    status = aili_reg_all_node_control_param(cb);
    if(status)
    {
        dev_err(dev, "[aili_trigger]reg all node control param error\n");
        return -EPERM;
    }
    return status;
}
    // list_for_each_entry(control_node_param, &control_node->param_head, param_node)
    // {
    //     if(control_node_param->dispose_handler)
    //     {
    //         control_node_param->dispose_handler(control_node_param->dispose_param);
    //     }
    // }
    // if(list_empty(&cb->run_node_head))
    // {
    //     list_replace_init(&cb->release_node_head,&cb->run_node_head);
    // }
static irqreturn_t extern_trigger_irq(int irq, void *_dev)
{
    aili_slave_control_node_param_t *control_node_param = NULL;
    aili_trigger_t *cb = (aili_trigger_t *)_dev;
    aili_slave_control_node_t *control_node = cb->current_run_node;
    // printk(KERN_INFO "into extern irq handle\n");
    if(!cb)
        return IRQ_HANDLED;
    if(cb->trigger_param.master_trigger_mode != AILI_MASTER_TRIGGER_EXT_TRG_MODE)
        return IRQ_HANDLED;
    if(!cb->current_run_node)
        return IRQ_HANDLED;
    if(!control_node->time_control_time)
    {
        list_for_each_entry(control_node_param, &control_node->param_head, param_node)
        {
            if(control_node_param->dispose_handler)
            {
                control_node_param->dispose_handler(control_node_param->dispose_param);
            }
        }
    }
    control_node = list_last_entry(&cb->run_node_head, aili_slave_control_node_t, run_node);
    if(control_node == NULL)
    {
        return IRQ_HANDLED;
    }
    cb->current_run_node = control_node;
    list_del_init(&control_node->run_node);
    list_add(&control_node->run_node, &cb->release_node_head);
    control_node = cb->current_run_node;
    if(hrtimer_is_queued(&cb->timer))
    {
        return IRQ_HANDLED;
    }
    hrtimer_start(&cb->timer, (control_node->time_control_time) * 1000,HRTIMER_MODE_REL);
	return IRQ_HANDLED;
}

static irqreturn_t pps_trigger_irq(int irq, void *_dev)
{
    aili_trigger_t *cb = (aili_trigger_t *)_dev;
    if(!cb)
        return IRQ_HANDLED;
    kill_fasync(&cb->pps_async_queue, SIGIO, POLL_IN);
	return IRQ_HANDLED;
}

static enum hrtimer_restart aili_trigger_hrtimer_handler(struct hrtimer *user_timer)
{
    aili_slave_control_node_param_t *control_node_param = NULL;
    aili_trigger_t *cb = container_of(user_timer, struct aili_trigger, timer);
    aili_slave_control_node_t *control_node = cb->current_run_node;
    aili_slave_control_node_t *delt_node = NULL;
    uint32_t next_run_timer = 0;
    if(!cb)
        return HRTIMER_NORESTART;
    // if(cb->trigger_param.master_trigger_mode != AILI_MASTER_TRIGGER_INTER_TRG_MODE)
    //     return HRTIMER_NORESTART;
    if(!cb->current_run_node)
        return HRTIMER_NORESTART;

    list_for_each_entry(control_node_param, &control_node->param_head, param_node)
    {
        if(control_node_param->dispose_handler)
        {
            control_node_param->dispose_handler(control_node_param->dispose_param);
        }
    }
    if(list_empty(&cb->run_node_head))
    {
        list_replace_init(&cb->release_node_head,&cb->run_node_head);
        control_node = list_last_entry(&cb->run_node_head, aili_slave_control_node_t, run_node);
        if(control_node == NULL)
        {
            return HRTIMER_NORESTART;
        }
        cb->current_run_node = control_node;
        if(cb->trigger_param.master_trigger_mode == AILI_MASTER_TRIGGER_EXT_TRG_MODE)
        {
            return HRTIMER_NORESTART;
        }
        next_run_timer = cb->trigger_param.master_mode_timer_param.remain_time_run_time + cb->current_run_node->time_control_time;
        if(next_run_timer <= MAX_CONTROL_TIME_DIFF_US)
        {
            list_for_each_entry_reverse(control_node, &cb->run_node_head, run_node)
            {
                if(!control_node->time_control_time)
                {
                    list_for_each_entry(control_node_param, &control_node->param_head, param_node)
                    {
                        if(control_node_param->dispose_handler)
                        {
                            control_node_param->dispose_handler(control_node_param->dispose_param);
                        }
                    }
                    delt_node = control_node;
                    control_node = list_next_entry(control_node,run_node);
                    list_del_init(&delt_node->run_node);
                    list_add(&delt_node->run_node, &cb->release_node_head);
                }
            }
            control_node = list_last_entry(&cb->run_node_head, aili_slave_control_node_t, run_node);
            if(control_node == NULL)
            {
                return HRTIMER_NORESTART;
            }
            cb->current_run_node = control_node;
            next_run_timer = cb->current_run_node->time_control_time;
        }
        list_del_init(&control_node->run_node);
        list_add(&control_node->run_node, &cb->release_node_head);
        // printk(KERN_INFO "time last%lld\n", (uint64_t)(next_run_timer *1000));
        hrtimer_forward_now(user_timer, next_run_timer * 1000);
        return HRTIMER_RESTART;
    }
    control_node = list_last_entry(&cb->run_node_head, aili_slave_control_node_t, run_node);
    if(control_node == NULL)
    {
        return HRTIMER_NORESTART;
    }
    cb->current_run_node = control_node;
    list_del_init(&control_node->run_node);
    list_add(&control_node->run_node, &cb->release_node_head);
    // printk(KERN_INFO "time %lld\n", (uint64_t)((cb->current_run_node->time_control_time) *1000));
    hrtimer_forward_now(user_timer, (cb->current_run_node->time_control_time) * 1000);
    // if(list_empty(&cb->run_node_head))
    // {
    //     next_run_timer = cb->trigger_param.master_mode_timer_param.remain_time_run_time + cb->current_run_node->time_control_time;
    //     // printk(KERN_INFO "time all %lld\n", (uint64_t)(next_run_timer *1000));
    //     hrtimer_forward_now(user_timer, next_run_timer * 1000);
    // }
    // else
    // {
    //     // printk(KERN_INFO "time %lld\n", (uint64_t)((cb->current_run_node->time_control_time) *1000));
    //     hrtimer_forward_now(user_timer, (cb->current_run_node->time_control_time) * 1000);
    // }
    return HRTIMER_RESTART;
}
static int aili_tirgger_set_run(aili_trigger_t *cb)
{
    int status = 0;
    aili_slave_control_node_t* control_node = NULL;
    aili_slave_control_node_t* delt_node = NULL;
    struct device *dev = &cb->pdev->dev;
    aili_slave_control_node_param_t *control_node_param = NULL;
    if(cb == NULL)
    {
        printk(KERN_ERR "[aili_trigger]cb is null \r\n");
        return -EINVAL;
    }
    if(list_empty(&cb->run_node_head))
    {
        dev_err(dev, "[aili_trigger]control node is empty not run\n");
        return -EINVAL;
    }
    if(cb->trigger_param.master_trigger_mode == AILI_MASTER_TRIGGER_INTER_TRG_MODE)
    {
        dev_info(dev,"into inter trg mode\n");
        list_for_each_entry_reverse(control_node, &cb->run_node_head, run_node)
        {
            if(!control_node->time_control_time)
            {
                list_for_each_entry(control_node_param, &control_node->param_head, param_node)
                {
                    if(control_node_param->dispose_handler)
                    {
                        control_node_param->dispose_handler(control_node_param->dispose_param);
                    }
                }
                delt_node = control_node;
                dev_info(dev ," node addr 0x%px\n",control_node);
                control_node = list_next_entry(control_node,run_node);
                list_del_init(&delt_node->run_node);
                list_add(&delt_node->run_node, &cb->release_node_head);
            }
        }
        dev_info(dev, "complete 0 time node run!\n");
        if(list_empty(&cb->run_node_head))
        {
            dev_warn(dev, "[aili_trigger]control node run complete time offset is zero no other node\n");
            return -EINVAL;
        }
        control_node = list_last_entry(&cb->run_node_head, aili_slave_control_node_t, run_node);
        if((control_node) != NULL && (control_node->time_control_time != 0))
        {
            cb->current_run_node = control_node;
            list_del_init(&control_node->run_node);
            list_add(&control_node->run_node, &cb->release_node_head);
            dev_info(dev, "current control time %lld, ns", (uint64_t)(control_node->time_control_time) * 1000);
            hrtimer_start(&cb->timer, (control_node->time_control_time) * 1000,HRTIMER_MODE_REL);
        }
        else
        {
            dev_err(dev, "[aili_trigger]control node not get\n");
            return -EINVAL;
        }
    }
    else if(cb->trigger_param.master_trigger_mode == AILI_MASTER_TRIGGER_EXT_TRG_MODE)
    {
        dev_info(dev,"into extern trg mode\n");
        control_node = list_last_entry(&cb->run_node_head, aili_slave_control_node_t, run_node);
        if(control_node == NULL)
        {
            dev_err(dev, "[aili_trigger]control node not get\n");
            return -EINVAL;
        }
        cb->current_run_node = control_node;
        list_del_init(&control_node->run_node);
        list_add(&control_node->run_node, &cb->release_node_head);
        if((cb->extern_trigger_irq_num > 0) && (cb->irq_status == AILI_TRIGGER_DISABLE_IRQ))
        {
            enable_irq(cb->extern_trigger_irq_num);
            cb->irq_status = AILI_TRIGGER_ENABLE_IRQ;
        }
        else
        {
            dev_err(dev, "no trigger irq num !!\n");
        };
    }
    return status;
}

static int aili_trigger_drv_open(struct inode *node, struct file *file)
{
    int status = 0;
    aili_trigger_t *cb = aili_tirgger_g;
    // struct device *dev = &cb->pdev->dev;
    if(cb == NULL)
    {
        printk(KERN_INFO "cb is null \r\n");
    }
    file->private_data = (void *)cb;

    return status;
}

static ssize_t aili_trigger_drv_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	return 0;
}

static ssize_t aili_trigger_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    return 0;
}

static int aili_trigger_drv_close (struct inode *node, struct file *file)
{
	return 0;
}

static int aili_trigger_stop(aili_trigger_t *cb)
{
    int status = 0;
    if(cb == NULL)
    {
        printk(KERN_ERR "trigger control blk is null\n");
        return -EIO;
    }
    hrtimer_cancel(&cb->timer);
    if((cb->extern_trigger_irq_num > 0) && (cb->irq_status == AILI_TRIGGER_ENABLE_IRQ))
    {
        disable_irq(cb->extern_trigger_irq_num);
        cb->irq_status = AILI_TRIGGER_DISABLE_IRQ;
    }
    return status;
}

static int aili_pps_start(aili_trigger_t *cb)
{
    int status = 0;
    if(cb == NULL)
    {
        printk(KERN_ERR "trigger control blk is null\n");
        return -EIO;
    }
    if((cb->pps_irq_num > 0) && (cb->pps_irq_status == AILI_TRIGGER_DISABLE_IRQ))
    {
        enable_irq(cb->pps_irq_num);
        cb->pps_irq_status = AILI_TRIGGER_ENABLE_IRQ;
    }
    return status;
}

static int aili_pps_stop(aili_trigger_t *cb)
{
    int status = 0;
    if(cb == NULL)
    {
        printk(KERN_ERR "trigger control blk is null\n");
        return -EIO;
    }
    if((cb->pps_irq_num > 0) && (cb->pps_irq_status == AILI_TRIGGER_ENABLE_IRQ))
    {
        disable_irq(cb->pps_irq_num);
        cb->pps_irq_status = AILI_TRIGGER_DISABLE_IRQ;
    }
    return status;
}


static int aili_trigger_set_user_param(struct device *dev, aili_trigger_user_param_t user_param)
{
    int status = 0;
    int i = 0;
    uint32_t channel = 0;
    memset(&default_trigger_pram, 0, sizeof(default_trigger_pram));
    if(user_param.trigger_mode >= AILI_MASTER_TRIGGER_MAX_MODE)
    {
        dev_err(dev, "input trigger_mode %d is error\n",user_param.trigger_mode);
        return -EINVAL;
    }
    default_trigger_pram.master_trigger_mode = user_param.trigger_mode;

    if((user_param.period_us < MAX_CONTROL_TIME_DIFF_US) || (user_param.period_us >(U32_MAX - 1000)))
    {
        dev_err(dev, "input period %d is error\n",user_param.period_us);
        return -EINVAL;
    }
    default_trigger_pram.master_mode_timer_param.output_period_us = user_param.period_us;
    default_trigger_pram.master_mode_timer_param.output_hz = 1000000U / user_param.period_us;

    if(user_param.param_count > MAX_OUT_TRIGGER_NUMBER)
    {
        dev_err(dev, "input param_count %d is error\n",user_param.param_count);
        return -EINVAL;
    }
    for(i = 0; i < user_param.param_count; i++)
    {
        channel = user_param.user_chanel_param[i].channel;
        if(channel >= MAX_OUT_TRIGGER_NUMBER)
        {
            dev_err(dev, "input param idx %d channel %d is error\n",i, channel);
            return -EINVAL;
        }
        default_trigger_pram.slave_trigger_class[channel].valid = 1;
        default_trigger_pram.slave_trigger_class[channel].channel = channel;
        default_trigger_pram.slave_trigger_class[channel].control_param.trigger_delay_time_us = user_param.user_chanel_param[i].trigger_delay_time_us;
        default_trigger_pram.slave_trigger_class[channel].control_param.trigger_valid_time_us = user_param.user_chanel_param[i].trigger_valid_time_us;
        default_trigger_pram.slave_trigger_class[channel].control_param.trigger_polarity = user_param.user_chanel_param[i].trigger_polarity;
    }

    dev_info(dev, "mode %d, period %d, count %d \n",user_param.trigger_mode, user_param.period_us, user_param.param_count);
    for(i = 0; i < user_param.param_count; i++)
    {
        dev_info(dev, "channel %d, delay %d, valid %d, polarity %d\n",
                 user_param.user_chanel_param[i].channel,
                 user_param.user_chanel_param[i].trigger_delay_time_us,
                 user_param.user_chanel_param[i].trigger_valid_time_us,
                 user_param.user_chanel_param[i].trigger_polarity
                );
    }
    return status;
}

static int aili_trigger_return_tirgger_param(aili_trigger_t *cb, aili_trigger_user_param_t *user_param)
{
    int status = 0;
    struct device *dev;
    int i = 0;
    if(!cb)
    {
        printk(KERN_ERR "input trigger cb is error\n");
        return -EINVAL;
    }
    dev = &cb->pdev->dev;
    if(user_param == NULL)
    {
        dev_err(dev, "input user param is NULL\n");
        return -EINVAL;
    }

    user_param->trigger_mode = cb->trigger_param.master_trigger_mode;
    user_param->period_us = cb->trigger_param.master_mode_timer_param.output_period_us;
    user_param->param_count = cb->trigger_gpio_num;

    for(i = 0; i < MAX_OUT_TRIGGER_NUMBER; i++)
    {
        user_param->user_chanel_param[i].channel = cb->trigger_param.slave_trigger_class[i].channel;
        user_param->user_chanel_param[i].trigger_delay_time_us = cb->trigger_param.slave_trigger_class[i].control_param.trigger_delay_time_us;
        user_param->user_chanel_param[i].trigger_valid_time_us = cb->trigger_param.slave_trigger_class[i].control_param.trigger_valid_time_us;
        user_param->user_chanel_param[i].trigger_polarity = cb->trigger_param.slave_trigger_class[i].control_param.trigger_polarity;
    }
    return status;
}

static int aili_trigger_return_tirgger_io_param(aili_trigger_t *cb, aili_trigger_read_io_param_t *io_param)
{
    int status = 0;
    struct device *dev;
    int i = 0;
    if(!cb)
    {
        printk(KERN_ERR "input trigger cb is error\n");
        return -EINVAL;
    }
    dev = &cb->pdev->dev;
    if(io_param == NULL)
    {
        dev_err(dev, "input trigger io param is NULL\n");
        return -EINVAL;
    }

    io_param->exit_extern_io_flag = cb->irq_status ? 1 : 0;
    io_param->extern_io = desc_to_gpio(cb->extern_trigger_gpio);
    io_param->trigger_io_count = cb->trigger_gpio_num;
    for(i = 0; i < (io_param->trigger_io_count); i++)
    {
        io_param->trigger_io[i] = desc_to_gpio(cb->trigger_out_gpio->desc[i]);
    }
    return status;
}

static long aili_trigger_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int status = 0;
    aili_trigger_t *cb = file->private_data;
    struct device *dev;
    void __user *argp = (void __user *)arg;
    aili_trigger_user_param_t user_param;
    aili_trigger_read_io_param_t io_param;
    if(cb == NULL)
    {
        printk(KERN_ERR "trigger control blk is null\n");
        return -EIO;
    }
    dev = &cb->pdev->dev;

    if(_IOC_TYPE(cmd) != AILI_TRIGGER_IOCTL_TYPE)
    {
        dev_err(dev, "input not aili trigger type 0x%x, should 0x%x\n", _IOC_TYPE(cmd), AILI_TRIGGER_IOCTL_TYPE);
        return -EINVAL;
    }

    switch(cmd)
    {
    case AILI_TRIGGER_ON_CMD:
        status = aili_trigger_stop(cb);
        if(status)
        {
            dev_err(dev, "stop trigger failed\n");
            return -EINVAL;
        }
        status = aili_trigger_all_node_init(cb, default_trigger_pram, MAX_OUT_TRIGGER_NUMBER);
        if(status)
        {
            dev_err(dev, "[aili_trigger]reg all node init error error\n");
            return -EINVAL;
        }

        status = aili_tirgger_set_run(cb);
        if(status)
        {
            dev_err(dev, "trigger set run error\n");
            return -EINVAL;
        }
        break;
    case AILI_TRIGGER_OFF_CMD:
        status = aili_trigger_stop(cb);
        break;
    case AILI_TRIGGER_SET_PARAM_CMD:
        if (copy_from_user(&user_param, argp, sizeof(aili_trigger_user_param_t)))
			return -EFAULT;
        status = aili_trigger_set_user_param(dev, user_param);
        if(status)
        {
            dev_err(dev, "trigger set param error\n");
            return -EINVAL;
        }
        break;
    case AILI_TRIGGER_GET_PARAM_CMD:
        status = aili_trigger_return_tirgger_param(cb, &user_param);
        if(status)
        {
            dev_err(dev, "get trigger param error\n");
            return -EINVAL;
        }
        return copy_to_user(argp, &user_param, sizeof(aili_trigger_user_param_t)) ? -EFAULT : 0;
        break;
    case AILI_TRIGGER_GET_IO_PARAM_CMD:
        status = aili_trigger_return_tirgger_io_param(cb, &io_param);
        if(status)
        {
            dev_err(dev, "get io param error\n");
        }
        return copy_to_user(argp, &io_param, sizeof(aili_trigger_read_io_param_t)) ? -EFAULT : 0;
        break;
    case AILI_PPS_IRQ_ON_CMD:
        status = aili_pps_start(cb);
        break;
    case AILI_PPS_IRQ_OFF_CMD:
        status = aili_pps_stop(cb);
        break;
    default:
		return -EINVAL;
    }
    return status;
}
static int aili_pps_fasync (int fd, struct file *filp, int mode)
{
    aili_trigger_t *cb = filp->private_data;
    struct device *dev;
    if(cb == NULL)
    {
        printk(KERN_ERR "trigger control blk is null\n");
        return -EIO;
    }
    dev = &cb->pdev->dev;
    return fasync_helper(fd, filp, mode, &cb->pps_async_queue);
}

static struct file_operations aili_trigger_char_drv = {
	.owner	 = THIS_MODULE,
	.open    = aili_trigger_drv_open,
	.read    = aili_trigger_drv_read,
	.write   = aili_trigger_drv_write,
	.release = aili_trigger_drv_close,
    .unlocked_ioctl = aili_trigger_ioctl,
    .fasync  = aili_pps_fasync,
};
static int aili_trigger_probe(struct platform_device *pdev)
{
    int status = 0;
    struct device *dev = &pdev->dev;
    struct device_node *node = dev->of_node;
    int irq_num = 0;
    int i = 0;
    // struct device_node *node = dev->of_node;
    dev_info(dev, "[aili_trigger]driver version: %02x.%02x.%02x\n", DRIVER_VERSION >> 16,
		 (DRIVER_VERSION & 0xff00) >> 8, DRIVER_VERSION & 0x00ff);

    if(aili_tirgger_g)
        return -EPERM;
    if(!of_device_is_available(node))
        return -ENOMEM;

    aili_tirgger_g = devm_kzalloc(dev, sizeof(*aili_tirgger_g), GFP_KERNEL);
    if(!aili_tirgger_g)
        return -ENOMEM;

    aili_tirgger_g->pdev = pdev;
    aili_tirgger_g->device_class_major = register_chrdev(0, DEVICE_NAME, &aili_trigger_char_drv);
    aili_tirgger_g->trigger_class = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
    aili_tirgger_g->irq_status = 0;
    if(IS_ERR(aili_tirgger_g->trigger_class))
    {
        dev_err(dev, "[aili_trigger]creat char dev error\n");
        status = PTR_ERR(aili_tirgger_g->trigger_class);
        goto chrdev_error;
        // unregister_chrdev(aili_tirgger_g->device_class_major, DEVICE_NAME);
        // devm_kfree(dev, aili_tirgger_g);
        // return PTR_ERR(aili_tirgger_g->trigger_class);
    }

    device_create(aili_tirgger_g->trigger_class, NULL, MKDEV(aili_tirgger_g->device_class_major, 0), (void*)aili_tirgger_g, "aili_trigger%d", 0);

    aili_tirgger_g->trigger_gpio_num = MAX_OUT_TRIGGER_NUMBER;
    INIT_LIST_HEAD(&(aili_tirgger_g->run_node_head));
    INIT_LIST_HEAD(&(aili_tirgger_g->release_node_head));
    memset(control_node, 0, sizeof(control_node));
    memset(param_node, 0, sizeof(param_node));
    memset(io_node_param, 0, sizeof(io_node_param));

    aili_tirgger_g->extern_trigger_gpio = devm_gpiod_get(dev, "extern", GPIOD_IN);
	if (IS_ERR(aili_tirgger_g->extern_trigger_gpio))
    {
        status = PTR_ERR(aili_tirgger_g->extern_trigger_gpio);
        dev_err(dev, "[aili_trigger]not have trigger out io :%d \n",status);
        goto chrdev_error;
    }
    else
    {
        dev_info(dev, "[aili_trigger]current support extern gpio trigger\n");
        irq_num =  gpiod_to_irq(aili_tirgger_g->extern_trigger_gpio);
        if(irq_num < 0)
        {
            dev_err(dev, "[aili_trigger]Failed to get extern IRQ number\n");
            status = irq_num;
            goto classdev_error;
        }
        status = request_irq(irq_num, extern_trigger_irq, IRQF_TRIGGER_RISING,"aili_extern_irq",(void *)aili_tirgger_g);
        if(status)
        {
            dev_err(dev, "[aili_trigger]Failed to request extern trigger IRQ\n");
            goto classdev_error;
        }
        aili_tirgger_g->extern_trigger_irq_num = irq_num;
        dev_info(dev, "extern gpio num %d, irq_number %d\n", desc_to_gpio(aili_tirgger_g->extern_trigger_gpio), aili_tirgger_g->extern_trigger_irq_num);
        // devm_request_threaded_irq(dev, irq_num,extern_trigger_irq,
        //                                    NULL, IRQF_TRIGGER_RISING | IRQF_ONESHOT,
        //                                    "aili_extern_irq",(void *)aili_tirgger_g);
        disable_irq(aili_tirgger_g->extern_trigger_irq_num);
        aili_tirgger_g->irq_status = AILI_TRIGGER_DISABLE_IRQ;
    }

    aili_tirgger_g->pps_gpio = devm_gpiod_get(dev, "pps", GPIOD_IN);
	if (!IS_ERR(aili_tirgger_g->pps_gpio))
    {
        dev_info(dev, "[aili_trigger]current support pps gpio trigger\n");
        irq_num =  gpiod_to_irq(aili_tirgger_g->pps_gpio);
        if(irq_num < 0)
        {
            dev_err(dev, "[aili_trigger]Failed to get pps IRQ number\n");
            status = irq_num;
            goto classdev_error;
        }
        status = request_irq(irq_num, pps_trigger_irq, IRQF_TRIGGER_RISING,"aili_pps_irq",(void *)aili_tirgger_g);
        if(status)
        {
            dev_err(dev, "[aili_trigger]Failed to request pps trigger IRQ\n");
            goto classdev_error;
        }
        aili_tirgger_g->pps_irq_num = irq_num;
        dev_info(dev, "pps gpio num %d, irq_number %d\n", desc_to_gpio(aili_tirgger_g->pps_gpio), aili_tirgger_g->pps_irq_num);
        // devm_request_threaded_irq(dev, irq_num,extern_trigger_irq,
        //                                    NULL, IRQF_TRIGGER_RISING | IRQF_ONESHOT,
        //                                    "aili_extern_irq",(void *)aili_tirgger_g);
        disable_irq(aili_tirgger_g->pps_irq_num);
        aili_tirgger_g->pps_irq_status = AILI_TRIGGER_DISABLE_IRQ;
    }


    aili_tirgger_g->trigger_out_gpio = devm_gpiod_get_array(dev, "triggerout", GPIOD_OUT_LOW);
    if(IS_ERR(aili_tirgger_g->trigger_out_gpio))
    {
        status = PTR_ERR(aili_tirgger_g->trigger_out_gpio);
        dev_err(dev, "[aili_trigger]not have trigger out io :%d \n",status);
        goto classdev_error;
    }

    if(aili_tirgger_g->trigger_out_gpio &&
      (aili_tirgger_g->trigger_out_gpio->ndescs > aili_tirgger_g->trigger_gpio_num))
    {
        dev_warn(dev, "[aili_trigger] dt get io number %d > support number %d only using support io num \n ",
                 aili_tirgger_g->trigger_out_gpio->ndescs,
                 aili_tirgger_g->trigger_gpio_num);

    }
    else if(aili_tirgger_g->trigger_out_gpio &&
           (aili_tirgger_g->trigger_out_gpio->ndescs <= aili_tirgger_g->trigger_gpio_num))
    {
        aili_tirgger_g->trigger_gpio_num = aili_tirgger_g->trigger_out_gpio->ndescs;
    }

    for(i = 0; i < aili_tirgger_g->trigger_gpio_num; i++)
    {
        aili_tirgger_g->trigger_param.slave_trigger_class[i].output_io = aili_tirgger_g->trigger_out_gpio->desc[i];
        dev_info(dev, "gpio number %d \n", desc_to_gpio(aili_tirgger_g->trigger_out_gpio->desc[i]));
    }
    memset(&aili_tirgger_g->timer, 0, sizeof(struct hrtimer));
    hrtimer_init(&aili_tirgger_g->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_HARD);
    aili_tirgger_g->timer.function = aili_trigger_hrtimer_handler;
    return status;
classdev_error:
    hrtimer_cancel(&aili_tirgger_g->timer);
    if(aili_tirgger_g->extern_trigger_irq_num > 0)
    {
        free_irq(aili_tirgger_g->extern_trigger_irq_num, aili_tirgger_g);
        aili_tirgger_g->extern_trigger_irq_num = -1;
    }
    if(aili_tirgger_g->pps_irq_num > 0)
    {
        free_irq(aili_tirgger_g->pps_irq_num, aili_tirgger_g);
        aili_tirgger_g->pps_irq_num = -1;
    }
    device_destroy(aili_tirgger_g->trigger_class, MKDEV(aili_tirgger_g->device_class_major, 0));
    class_destroy(aili_tirgger_g->trigger_class);
chrdev_error:
    unregister_chrdev(aili_tirgger_g->device_class_major, DEVICE_NAME);
    devm_kfree(dev, aili_tirgger_g);
    return status;
}

static int aili_trigger_remove(struct platform_device *pdev)
{
    int status = 0;
    struct device *dev = &pdev->dev;
    dev_info(dev, "[aili_trigger] remove aili trigger\n");
    hrtimer_cancel(&aili_tirgger_g->timer);
    if(aili_tirgger_g->extern_trigger_irq_num > 0)
    {
        free_irq(aili_tirgger_g->extern_trigger_irq_num, aili_tirgger_g);
        aili_tirgger_g->extern_trigger_irq_num = -1;
    }
    device_destroy(aili_tirgger_g->trigger_class, MKDEV(aili_tirgger_g->device_class_major, 0));
    class_destroy(aili_tirgger_g->trigger_class);
    unregister_chrdev(aili_tirgger_g->device_class_major, DEVICE_NAME);
    devm_kfree(dev, aili_tirgger_g);
    return status;
}

static const struct of_device_id aili_trigger_match_table[] = {
    { .compatible = "aili-light,trigger" },
    {},
};

static struct platform_driver aili_trigger_driver = {
    .probe      = aili_trigger_probe,
    .remove     = aili_trigger_remove,
    .driver     = {
        .name   = "aili_trigger",
        .of_match_table = aili_trigger_match_table,
    },
};

static int __init trigger_init(void)
{
    int status = 0;
    status = platform_driver_register(&aili_trigger_driver);
    return status;
};

static void __exit trigger_exit(void)
{
    platform_driver_unregister(&aili_trigger_driver);
}

module_init(trigger_init);
module_exit(trigger_exit);

MODULE_LICENSE("GPL");