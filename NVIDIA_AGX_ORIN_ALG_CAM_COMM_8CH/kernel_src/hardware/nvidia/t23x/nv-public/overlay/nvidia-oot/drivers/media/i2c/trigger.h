/**
 * @file trigger.h
 * @brief
 * @author mark (mark@ailiteam.com)
 * @version 1.0
 * @date 2024-03-09
 *
 * @copyright Copyright (c) 2024  ailiteam
 *
 * @par 修改日志:
 */

#ifndef _AILI_TRIGGER_H_
#define _AILI_TRIGGER_H_
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/hrtimer.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include "trigger_export.h"

#define DRIVER_VERSION			KERNEL_VERSION(0x00, 0x00, 0x00)

#define MAX_CONTORL_NODE_NUMBER (18U)
#define MAX_PARAM_NODE_NUMBER   (18U)
#define DEVICE_NAME             "aili_trigger"
#define DEVICE_CLASS_NAME       "aili_trigger_class"
#define MAX_CONTROL_TIME_DIFF_US   (20U)

#define AILI_TIRGGER_ADD_NODE               (0x00000001)
#define AILI_TRIGGER_RESET_NODE             (0x00000002)
#define AILI_TRIGGER_RESET_AND_ADD_NODE     AILI_TIRGGER_ADD_NODE | AILI_TRIGGER_RESET_NODE
#define AILI_TRIGGER_DISABLE_IRQ            (1U)
#define AILI_TRIGGER_ENABLE_IRQ             (2U)
typedef void (*node_dispos_handler)(void *dispose_param);
typedef enum
{
    AILI_MASTER_TRIGGER_DISABLE_MODE = 0,
	AILI_MASTER_TRIGGER_EXT_TRG_MODE = 1,
	AILI_MASTER_TRIGGER_INTER_TRG_MODE = 2,
    AILI_MASTER_TRIGGER_MAX_MODE,
} aili_master_trigger_mode_e;


enum
{
    AILI_SLAVE_TRIGGER_POSITIVE = 0,
    AILI_SLAVE_TRIGGER_NAGTIVE,
    AILI_SLAVE_TRIGGER_MAX,
};
typedef struct aili_slave_trigger_control_param
{
    uint32_t trigger_delay_time_us;
    uint32_t trigger_valid_time_us;
    uint8_t  trigger_polarity;
}aili_slave_trigger_control_param_t;

typedef struct aili_slave_trigger_master_mode_timer_param
{
    uint32_t output_hz;
    uint32_t output_period_us;
    uint32_t remain_time_run_time;
}aili_slave_trigger_master_mode_timer_param_t;

typedef struct aili_slave_trigger_control_class
{
    uint8_t valid;
    uint32_t channel;
    struct gpio_desc* output_io;
    aili_slave_trigger_control_param_t control_param;
}aili_slave_trigger_control_class_t;

typedef struct aili_trigger_param
{
    aili_master_trigger_mode_e master_trigger_mode;
    aili_slave_trigger_master_mode_timer_param_t master_mode_timer_param;
    aili_slave_trigger_control_class_t slave_trigger_class[MAX_OUT_TRIGGER_NUMBER];
}aili_trigger_param_t;

typedef struct
{
    struct gpio_desc* io_param;
    uint8_t value;
}aili_slave_trigger_io_node_param;

typedef struct aili_slave_control_node_param
{
    uint8_t valid;
    uint32_t absolute_time;
    uint32_t relative_time;
    struct list_head param_node;
    node_dispos_handler dispose_handler;
    void *dispose_param;
}aili_slave_control_node_param_t;

typedef struct aili_slave_control_node
{
    uint8_t valid;
    uint32_t time_control_time;
    uint32_t time_control_abs_time;
    struct list_head run_node;
    struct list_head param_head;
}aili_slave_control_node_t;

typedef struct aili_trigger
{
    struct platform_device *pdev;
    struct class *trigger_class;
    int device_class_major;
    struct gpio_desc *extern_trigger_gpio;
    struct gpio_desc *pps_gpio;
    int extern_trigger_irq_num;
    int pps_irq_num;
    int trigger_gpio_num;
    int irq_status;
    int pps_irq_status;
    struct gpio_descs *trigger_out_gpio;
    struct hrtimer timer;
    aili_trigger_param_t trigger_param;
    int32_t control_node_param_num;
    struct list_head run_node_head;
    struct list_head release_node_head;
    aili_slave_control_node_t *current_run_node;
    struct fasync_struct *pps_async_queue;
}aili_trigger_t;


#endif