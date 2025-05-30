// SPDX-License-Identifier: GPL-2.0
/*
 * Implement the manual drop-all-pagecache function
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/writeback.h>
#include <linux/sysctl.h>
#include <linux/gfp.h>
#include "internal.h"
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>

/* A global variable is a bit ugly, but it keeps the code simple */
int sysctl_drop_caches;

static void drop_pagecache_sb(struct super_block *sb, void *unused)
{
	struct inode *inode, *toput_inode = NULL;

	spin_lock(&sb->s_inode_list_lock);
	list_for_each_entry(inode, &sb->s_inodes, i_sb_list) {
		spin_lock(&inode->i_lock);
		/*
		 * We must skip inodes in unusual state. We may also skip
		 * inodes without pages but we deliberately won't in case
		 * we need to reschedule to avoid softlockups.
		 */
		if ((inode->i_state & (I_FREEING|I_WILL_FREE|I_NEW)) ||
		    (inode->i_mapping->nrpages == 0 && !need_resched())) {
			spin_unlock(&inode->i_lock);
			continue;
		}
		__iget(inode);
		spin_unlock(&inode->i_lock);
		spin_unlock(&sb->s_inode_list_lock);

		invalidate_mapping_pages(inode->i_mapping, 0, -1);
		iput(toput_inode);
		toput_inode = inode;

		cond_resched();
		spin_lock(&sb->s_inode_list_lock);
	}
	spin_unlock(&sb->s_inode_list_lock);
	iput(toput_inode);
}

void mm_drop_caches(int val)
{
	if (val & 1) {
		iterate_supers(drop_pagecache_sb, NULL);
		count_vm_event(DROP_PAGECACHE);
	}
	if (val & 2) {
		drop_slab();
		count_vm_event(DROP_SLAB);
	}
}

int drop_caches_sysctl_handler(struct ctl_table *table, int write,
	void __user *buffer, size_t *length, loff_t *ppos)
{
	int ret;

	ret = proc_dointvec_minmax(table, write, buffer, length, ppos);
	if (ret)
		return ret;
	if (write) {
		static int stfu;

		mm_drop_caches(sysctl_drop_caches);

		if (!stfu) {
			pr_info("%s (%d): drop_caches: %d\n",
				current->comm, task_pid_nr(current),
				sysctl_drop_caches);
		}
		stfu |= sysctl_drop_caches & 4;
	}
	return 0;
}

static void drop_caches_suspend(struct work_struct *work);
static DECLARE_DELAYED_WORK(drop_caches_suspend_work, drop_caches_suspend);

static unsigned long last_screen_off = 0;
static bool cache_drop_in_progress = false;

static void drop_caches_suspend(struct work_struct *work)
{
	unsigned long free_mem, total_mem;
	
	if (system_state != SYSTEM_RUNNING) {
		return;
	}
	
	if (cache_drop_in_progress) {
		return;
	}
	
	cache_drop_in_progress = true;
	
	free_mem = global_zone_page_state(NR_FREE_PAGES);
	total_mem = totalram_pages;
	
	if (free_mem > (total_mem / 5)) {
		cache_drop_in_progress = false;
		return;
	}
	
	wakeup_flusher_threads(0, WB_REASON_FREE_MORE_MEM);
	
	msleep(100);
	
	iterate_supers(drop_pagecache_sb, NULL);
	
	if (free_mem < (total_mem / 10)) {
		drop_slab();
	}
	
	cache_drop_in_progress = false;
}

static int fb_notifier(struct notifier_block *self,
			unsigned long event, void *data)
{
	struct fb_event *evdata = (struct fb_event *)data;
	if ((event == FB_EVENT_BLANK) && evdata && evdata->data) {
		int blank = *(int *)evdata->data;
		if (blank == FB_BLANK_POWERDOWN) {
			unsigned long now = jiffies;
			if (time_after(now, last_screen_off + HZ * 5)) {  // 5 second minimum interval
				last_screen_off = now;
				schedule_delayed_work(&drop_caches_suspend_work, msecs_to_jiffies(500));
			}
			return NOTIFY_OK;
		} else if (blank == FB_BLANK_UNBLANK) {
			cancel_delayed_work(&drop_caches_suspend_work);
			return NOTIFY_OK;
		}
		return NOTIFY_OK;
	}
	return NOTIFY_DONE;
}

static struct notifier_block fb_notifier_block = {
	.notifier_call = fb_notifier,
	.priority = -1,
};

static int __init drop_caches_init(void)
{
	if (totalram_pages > (8UL << (30 - PAGE_SHIFT))) {  
		return 0;
	}
	
	fb_register_client(&fb_notifier_block);
	printk(KERN_INFO "Smart cache drop on screen-off enabled\n");
	return 0;
}

static void __exit drop_caches_exit(void)
{
	fb_unregister_client(&fb_notifier_block);
	cancel_delayed_work_sync(&drop_caches_suspend_work);
}

module_init(drop_caches_init);
module_exit(drop_caches_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("tew404");
MODULE_DESCRIPTION("Smart cache drop on screen off(custom version)");
