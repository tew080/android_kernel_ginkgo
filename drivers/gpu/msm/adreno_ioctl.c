/* Copyright (c) 2002,2007-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/ioctl.h>
#include "kgsl_device.h"
#include "adreno.h"
#include "adreno_a5xx.h"

/*
 * Add a perfcounter to the per-fd list.
 * Call with the device mutex held
 */
static int adreno_process_perfcounter_add(struct kgsl_device_private *dev_priv,
	unsigned int groupid, unsigned int countable)
{
	struct adreno_device_private *adreno_priv = container_of(dev_priv,
		struct adreno_device_private, dev_priv);
	struct adreno_perfcounter_list_node *perfctr;

	perfctr = kzalloc(sizeof(*perfctr), GFP_KERNEL);
	if (!perfctr)
		return -ENOMEM;

	perfctr->groupid = groupid;
	perfctr->countable = countable;

	/* add the pair to process perfcounter list */
	list_add(&perfctr->node, &adreno_priv->perfcounter_list);
	return 0;
}

/*
 * Remove a perfcounter from the per-fd list.
 * Call with the device mutex held
 */
static int adreno_process_perfcounter_del(struct kgsl_device_private *dev_priv,
	unsigned int groupid, unsigned int countable)
{
	struct adreno_device_private *adreno_priv = container_of(dev_priv,
		struct adreno_device_private, dev_priv);
	struct adreno_perfcounter_list_node *p, *tmp;

	list_for_each_entry_safe(p, tmp, &adreno_priv->perfcounter_list, node) {
		if (p->groupid == groupid && p->countable == countable) {
			list_del(&p->node);
			kfree(p);
			return 0;
		}
	}
	return -ENODEV;
}

long adreno_ioctl_perfcounter_get(struct kgsl_device_private *dev_priv,
	unsigned int cmd, void *data)
{
	struct kgsl_device *device = dev_priv->device;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct kgsl_perfcounter_get *get = data;
	int result;

	mutex_lock(&device->mutex);
	result = adreno_perfcntr_active_oob_get(adreno_dev);
	mutex_unlock(&device->mutex);

	if (result)
		return (long)result;

	result = adreno_perfcounter_get(adreno_dev, get->groupid, get->countable,
			&get->offset, &get->offset_hi, PERFCOUNTER_FLAG_NONE);

	if (!result) {
		result = adreno_process_perfcounter_add(dev_priv, get->groupid, get->countable);
		if (result)
			adreno_perfcounter_put(adreno_dev, get->groupid, get->countable, PERFCOUNTER_FLAG_NONE);
	}

	adreno_perfcntr_active_oob_put(adreno_dev);

	return (long) result;
}

long adreno_ioctl_perfcounter_put(struct kgsl_device_private *dev_priv,
		unsigned int cmd, void *data)
{
	struct kgsl_device *device = dev_priv->device;
	struct adreno_device *adreno_dev = ADRENO_DEVICE(device);
	struct kgsl_perfcounter_put *put = data;
	int result;

	mutex_lock(&device->mutex);
	result = adreno_process_perfcounter_del(dev_priv, put->groupid, put->countable);
	mutex_unlock(&device->mutex);

	if (!result)
		adreno_perfcounter_put(adreno_dev, put->groupid, put->countable, PERFCOUNTER_FLAG_NONE);

	return (long) result;
}

long adreno_ioctl_perfcounter_read(struct kgsl_device_private *dev_priv,
		unsigned int cmd, void *data)
{
	struct adreno_device *adreno_dev = ADRENO_DEVICE(dev_priv->device);
	struct kgsl_perfcounter_read *read = data;

	return (long) adreno_perfcounter_read_group(adreno_dev, read->reads,
		read->count);
}

long adreno_ioctl_helper(struct kgsl_device_private *dev_priv,
		unsigned int cmd, unsigned long arg,
		const struct kgsl_ioctl *cmds, int len)
{
	unsigned char data[128] = { 0 };
	long ret;
	int i;

	static DEFINE_RATELIMIT_STATE(_rs,
			DEFAULT_RATELIMIT_INTERVAL,
			DEFAULT_RATELIMIT_BURST);

	for (i = 0; i < len; i++) {
		if (_IOC_NR(cmd) == _IOC_NR(cmds[i].cmd))
			break;
	}

	if (i == len) {
		KGSL_DRV_INFO(dev_priv->device,
			"invalid ioctl code 0x%08X\n", cmd);
		return -ENOIOCTLCMD;
	}

	if (_IOC_SIZE(cmds[i].cmd) > sizeof(data)) {
		dev_err_ratelimited(dev_priv->device->dev,
			"data too big for ioctl 0x%08x: %d/%zu\n",
			cmd, _IOC_SIZE(cmds[i].cmd), sizeof(data));
		return -EINVAL;
	}

	if (_IOC_SIZE(cmds[i].cmd)) {
		ret = kgsl_ioctl_copy_in(cmds[i].cmd, cmd, arg, data);
		if (ret)
			return ret;
	} else {
		memset(data, 0, sizeof(data));
	}

	ret = cmds[i].func(dev_priv, cmd, data);

	if (ret == 0 && _IOC_SIZE(cmds[i].cmd))
		ret = kgsl_ioctl_copy_out(cmds[i].cmd, cmd, arg, data);

	return ret;
}

static struct kgsl_ioctl adreno_ioctl_funcs[] = {
	{ IOCTL_KGSL_PERFCOUNTER_GET, adreno_ioctl_perfcounter_get },
	{ IOCTL_KGSL_PERFCOUNTER_PUT, adreno_ioctl_perfcounter_put },
	{ IOCTL_KGSL_PERFCOUNTER_READ, adreno_ioctl_perfcounter_read },
};

long adreno_ioctl(struct kgsl_device_private *dev_priv,
		      unsigned int cmd, unsigned long arg)
{
	return adreno_ioctl_helper(dev_priv, cmd, arg,
		adreno_ioctl_funcs, ARRAY_SIZE(adreno_ioctl_funcs));
}

