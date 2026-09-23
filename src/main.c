#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/slab.h>

#define DEFAULT_INTERVAL 30
#define STOP_AFTER_MIN   5

static struct timer_list my_timer;
static unsigned int interval_sec = DEFAULT_INTERVAL;
static unsigned int tick_count;                         
static const unsigned int max_ticks = (STOP_AFTER_MIN * 60) / DEFAULT_INTERVAL;

static struct kobject *timer_kobj;

static ssize_t interval_show(struct kobject *kobj,
                             struct kobj_attribute *attr, char *buf)
{
    return sysfs_emit(buf, "%u\n", interval_sec);
}

static ssize_t interval_store(struct kobject *kobj,
                              struct kobj_attribute *attr,
                              const char *buf, size_t count)
{
    unsigned int val;
    int ret;

    ret = kstrtouint(buf, 10, &val);
    if (ret)
        return ret;

    if (val == 0 || val > 3600)        // границы: 1..3600 сек
        return -EINVAL;

    interval_sec = val;
    tick_count   = 0;                  /* сброс счётчика при смене интервала */

    /* пересоздаём таймер с новым интервалом */
    mod_timer(&my_timer, jiffies + msecs_to_jiffies(interval_sec * 1000));

    pr_info("Kernel Timer Demo: new interval = %u sec\n", interval_sec);
    return count;
}

static struct kobj_attribute interval_attr =
    __ATTR(interval_sec, 0644, interval_show, interval_store);

static void timer_callback(struct timer_list *t)
{
    unsigned int minutes;

    tick_count++;
    minutes = (tick_count * interval_sec) / 60;

    pr_info("min=%u: Hello, timer!\n", minutes);

    if ((tick_count * interval_sec) >= (STOP_AFTER_MIN * 60)) {
        pr_info("Kernel Timer Demo: %u minutes passed, stopping timer.\n",
                STOP_AFTER_MIN);
        return;                        
    }

    mod_timer(t, jiffies + msecs_to_jiffies(interval_sec * 1000));
}

static int __init kernel_timer(void) {
    pr_info("Kernel Timer Demo: Initializing...\n");

    int ret;

    pr_info("Kernel Timer Demo: loading, interval=%u sec\n", interval_sec);

    timer_kobj = kobject_create_and_add("timer_mod", kernel_kobj);
    if (!timer_kobj)
        return -ENOMEM;

    ret = sysfs_create_file(timer_kobj, &interval_attr.attr);
    if (ret) {
        kobject_put(timer_kobj);
        return ret;
    }

    timer_setup(&my_timer, timer_callback, 0);

    tick_count = 0;
    my_timer.expires = jiffies + msecs_to_jiffies(interval_sec * 1000);
    add_timer(&my_timer);

    pr_info("Kernel Timer Demo: Initializing Done\n");
    return 0;
}

static void __exit kernel_timer_exit(void) {
    pr_info("Kernel Timer Demo: Exiting...\n");

    timer_delete_sync(&my_timer);
    sysfs_remove_file(timer_kobj, &interval_attr.attr);
    kobject_put(timer_kobj);

    pr_info("Kernel Timer Demo: Exiting Done\n");
    return;
}

module_init(kernel_timer);
module_exit(kernel_timer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fedor Kazakov");
MODULE_DESCRIPTION("Otus home work 9 (Timer callback)");
MODULE_VERSION("1.0");

