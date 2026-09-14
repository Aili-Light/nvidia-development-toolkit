// SPDX-License-Identifier: GPL-2.0
/*
 * aili-trigger.c - 8 通道同步触发器输出驱动
 *
 * 从设备树加载 8 个 GPIO，用 hrtimer 同步输出 PWM 脉冲。
 * 周期由 period-ns 指定，脉宽由 pluse-width-ns 指定。
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio/consumer.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/slab.h>

#define DRV_NAME	"aili-trigger"
#define NUM_GPIOS	8

struct aili_trigger {
	struct device		*dev;
	struct gpio_descs	*gpios;      /* 8 个 GPIO */
	struct hrtimer		timer;       /* 高精度定时器 */
	ktime_t			period;      /* 周期 */
	ktime_t			pulse_width; /* 脉宽 */
	bool			pulse_active;/* 当前是否处于脉冲有效阶段 */
};

/*
 * 定时器回调：状态机
 *   !pulse_active -> 输出有效电平(脉冲开始)，重挂到 pulse_width 之后
 *    pulse_active -> 输出无效电平(脉冲结束)，重挂到 period - pulse_width 之后
 */
static enum hrtimer_restart aili_trigger_cb(struct hrtimer *tmr)
{
	struct aili_trigger *trig = container_of(tmr, struct aili_trigger, timer);
	unsigned int i;

	if (!trig->pulse_active) {
		/* 脉冲开始：8 路同时置为逻辑 1 (由 GPIO_ACTIVE_LOW 决定实际电平) */
		for (i = 0; i < trig->gpios->ndescs; i++)
			gpiod_set_value(trig->gpios->desc[i], 1);

		trig->pulse_active = true;
		hrtimer_forward_now(tmr, trig->pulse_width);
	} else {
		/* 脉冲结束：8 路同时置为逻辑 0 */
		for (i = 0; i < trig->gpios->ndescs; i++)
			gpiod_set_value(trig->gpios->desc[i], 0);

		trig->pulse_active = false;
		hrtimer_forward_now(tmr,
				    ktime_sub(trig->period, trig->pulse_width));
	}

	return HRTIMER_RESTART;
}

static int aili_trigger_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct aili_trigger *trig;
	u32 period_ns, pulse_ns;
	int ret;

	trig = devm_kzalloc(dev, sizeof(*trig), GFP_KERNEL);
	if (!trig)
		return -ENOMEM;
	trig->dev = dev;

	/* 1. 从设备树获取 triggerout-gpios 数组 */
	trig->gpios = devm_gpiod_get_array(dev, "triggerout", GPIOD_OUT_LOW);
	if (IS_ERR(trig->gpios)) {
		ret = PTR_ERR(trig->gpios);
		dev_err(dev, "failed to get 'triggerout-gpios': %d\n", ret);
		return ret;
	}
	if (trig->gpios->ndescs != NUM_GPIOS) {
		dev_err(dev, "expected %d GPIOs, got %u\n",
			NUM_GPIOS, trig->gpios->ndescs);
		return -EINVAL;
	}

	/* 2. 读取周期与脉宽 */
	ret = of_property_read_u32(dev->of_node, "period-ns", &period_ns);
	if (ret) {
		dev_err(dev, "missing 'period-ns' property\n");
		return ret;
	}

	/* 设备树里是 pluse-width-ns（拼写错误），两种都兼容 */
	ret = of_property_read_u32(dev->of_node, "pluse-width-ns", &pulse_ns);
	if (ret)
		ret = of_property_read_u32(dev->of_node, "pulse-width-ns", &pulse_ns);
	if (ret) {
		dev_err(dev, "missing 'pluse-width-ns' property\n");
		return ret;
	}

	if (period_ns == 0 || pulse_ns == 0 || pulse_ns >= period_ns) {
		dev_err(dev, "invalid period(%u) / pulse(%u)\n",
			period_ns, pulse_ns);
		return -EINVAL;
	}

	trig->period      = ns_to_ktime(period_ns);
	trig->pulse_width = ns_to_ktime(pulse_ns);
	trig->pulse_active = false;

	/* 3. 初始化 hrtimer */
	hrtimer_init(&trig->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	trig->timer.function = aili_trigger_cb;

	platform_set_drvdata(pdev, trig);

	/* 4. 启动定时器，周期时间后触发第一个脉冲 */
	hrtimer_start(&trig->timer, trig->period, HRTIMER_MODE_REL);

	dev_info(dev, "started: %u GPIOs, period=%u ns, pulse=%u ns\n",
		 trig->gpios->ndescs, period_ns, pulse_ns);

	return 0;
}

static int aili_trigger_remove(struct platform_device *pdev)
{
	struct aili_trigger *trig = platform_get_drvdata(pdev);
	unsigned int i;

	/* 停止定时器，并把所有 GPIO 拉回无效状态 */
	hrtimer_cancel(&trig->timer);
	for (i = 0; i < trig->gpios->ndescs; i++)
		gpiod_set_value(trig->gpios->desc[i], 0);

	return 0;
}

static const struct of_device_id aili_trigger_of_match[] = {
	{ .compatible = "aili-light,trigger" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, aili_trigger_of_match);

static struct platform_driver aili_trigger_driver = {
	.probe  = aili_trigger_probe,
	.remove = aili_trigger_remove,
	.driver = {
		.name           = DRV_NAME,
		.of_match_table = aili_trigger_of_match,
	},
};
module_platform_driver(aili_trigger_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("you");
MODULE_DESCRIPTION("Aili 8-channel synchronized trigger output");