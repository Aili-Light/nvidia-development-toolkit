/**
 * @file trigger_export.h
 * @brief
 * @author mark (mark@ailiteam.com)
 * @version 1.0
 * @date 2024-03-15
 *
 * @copyright Copyright (c) 2024  ailiteam
 *
 * @par 修改日志:
 */
#ifndef _AILI_TRIGGER_EXPORT_H_
#define _AILI_TRIGGER_EXPORT_H_
#include <linux/const.h>
#include <linux/ioctl.h>
#include <linux/types.h>

#define MAX_OUT_TRIGGER_NUMBER  (8U)
#define AILI_TRIGGER_IOCTL_TYPE 'T'

typedef struct aili_trigger_user_channel_param
{
    uint32_t channel;
    uint32_t trigger_delay_time_us;
    uint32_t trigger_valid_time_us;
    uint8_t  trigger_polarity;
}aili_trigger_user_channel_param_t;

typedef struct aili_trigger_user_param
{
    uint32_t trigger_mode;
    uint32_t period_us;
    uint32_t param_count;
    aili_trigger_user_channel_param_t user_chanel_param[MAX_OUT_TRIGGER_NUMBER];
}aili_trigger_user_param_t;

typedef struct aili_tirgger_read_io_param
{
    uint32_t exit_extern_io_flag;
    uint32_t extern_io;
    uint32_t trigger_io_count;
    uint32_t trigger_io[MAX_OUT_TRIGGER_NUMBER];
}aili_trigger_read_io_param_t;

#define AILI_TRIGGER_ON_CMD            _IO(AILI_TRIGGER_IOCTL_TYPE, 0x01)
#define AILI_TRIGGER_OFF_CMD           _IO(AILI_TRIGGER_IOCTL_TYPE, 0x02)
#define AILI_TRIGGER_SET_PARAM_CMD     _IOW(AILI_TRIGGER_IOCTL_TYPE, 0x03, aili_trigger_user_param_t)
#define AILI_TRIGGER_GET_PARAM_CMD     _IOR(AILI_TRIGGER_IOCTL_TYPE, 0x04, aili_trigger_user_param_t)
#define AILI_TRIGGER_GET_IO_PARAM_CMD  _IOR(AILI_TRIGGER_IOCTL_TYPE, 0x04, aili_trigger_read_io_param_t)
#define AILI_PPS_IRQ_ON_CMD            _IO(AILI_TRIGGER_IOCTL_TYPE, 0x05)
#define AILI_PPS_IRQ_OFF_CMD            _IO(AILI_TRIGGER_IOCTL_TYPE, 0x06)

#endif