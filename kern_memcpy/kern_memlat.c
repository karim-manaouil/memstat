#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/mm_inline.h>
#include <linux/mman.h>
#include <linux/pagewalk.h>
#include <linux/memory.h>
#include <linux/memory_hotplug.h>
#include <linux/vmalloc.h>
#include <linux/mmu_notifier.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <asm/timer.h>
#include <asm/msr.h>

MODULE_DESCRIPTION("Measure kernel page copy performance");
MODULE_AUTHOR("Karim Manaouil <k.manaouil@gmail.com>");
MODULE_LICENSE("GPL");

static unsigned long long do_page_copy(void *pdest, void *psrc, int psize, int *cpu)
{
	unsigned long long start, end;
	unsigned long flags;

	local_irq_save(flags);

	*cpu = smp_processor_id();

	/* Calls rdtsc then accurately convert cycles to ns */
	start = rdtsc_ordered();
	memcpy(pdest, psrc, psize);
	end = rdtsc_ordered();

	local_irq_restore(flags);

	end -= start;
	return end;
}

static void __measure_page_copy_vmalloc(int src, int dest, int psize)
{
	void *psrc, *pdest;
	int processor_id;
	unsigned long long cycles;

	psrc = vmalloc_node(psize, src);
	if (!psrc) {
		pr_err("failed to allocate source page");
		return;
	}
	pdest = vmalloc_node(psize, dest);
	if (!pdest) {
		pr_err("failed to allocate destination page");
		return;
	}
	cycles = do_page_copy(pdest, psrc, psize, &processor_id);

	pr_info("[%d/%d] Memory copy from node %d to node %d with size %d KiB took %llu cyc (%llu kcyc)\n", processor_id, cpu_to_node(processor_id), src, dest, psize/1024, cycles, cycles/1000);

	vfree(psrc);
	vfree(pdest);
}

static void __measure_page_copy_order(int src, int dest, int order)
{
	struct page *spage, *dpage;
	void *psrc, *pdest;
	int processor_id;
	unsigned long long cycles;

	spage = alloc_pages_node(src, GFP_KERNEL, order);
	if (!spage) {
		pr_err("failed to allocate source page");
		return;
	}
	dpage = alloc_pages_node(dest, GFP_KERNEL, order);
	if (!dpage) {
		pr_err("failed to allocate destination page");
		return;
	}
	psrc = page_to_virt(spage);
	pdest = page_to_virt(dpage);

	cycles = do_page_copy(pdest, psrc, 4096 * (1 << order), &processor_id);

	pr_info("[%d/%d] Memory copy from node %d to node %d with size %d KiB took %llu cyc (%llu kcyc)\n", processor_id, cpu_to_node(processor_id), src, dest, (1 << order)*4, cycles, cycles/1000);

	__free_pages(spage, order);
	__free_pages(dpage, order);
}

static void measure_page_copy_vmalloc(int src, int dest)
{
	int page_sizes[] = {4 << 10, 1 << 20, 2 << 20, 16 << 20, 32 << 20};
	int i;

	for (i = 0; i < sizeof(page_sizes)/sizeof(*page_sizes); i++)
		__measure_page_copy_vmalloc(src, dest, page_sizes[i]);
}

static void measure_page_copy_order(int src, int dest)
{
	int page_orders[] = {0, 8, 9};
	int i;

	for (i = 0; i < sizeof(page_orders)/sizeof(*page_orders); i++)
		__measure_page_copy_order(src, dest, page_orders[i]);
}

static void measure_numa_page_copy(bool vmalloc)
{
	int src, dest;
        const struct cpumask *mask;
        int retval;

	for_each_node(src) {
                mask = cpumask_of_node(src);
                retval = sched_setaffinity(0, mask);
                if (retval) {
                        pr_err("Couldn't move process to node %d\n", src);
                        continue;
                }
                schedule();
                for_each_node(dest)
			if (vmalloc)
				measure_page_copy_vmalloc(src, dest);
			else
				measure_page_copy_order(src, dest);
	}
}
/*************************** sysfs *******************************/
#define MEMLAT_ATTR(_name) \
       static struct kobj_attribute _name##_attr = __ATTR_RW(_name)

static ssize_t memlat_show(struct kobject *kobj,
			  struct kobj_attribute *attr,
			  char *buf)
{
		return 0;
}

static ssize_t memlat_store(struct kobject *kobj,
			   struct kobj_attribute *attr,
			   const char *buf, size_t count)
{
	char memlat_cmd;

	memlat_cmd = buf[0];

	switch(memlat_cmd) {
	case '0':
		measure_numa_page_copy(0);
		break;
	case '1':
		measure_numa_page_copy(1);
		break;
	}
	return count;
}
MEMLAT_ATTR(memlat);

static struct attribute *memlat_attrs[] = {
	&memlat_attr.attr,
};

static const struct attribute_group memlat_attr_group = {
	.attrs = memlat_attrs,
	.name = "memlat",
};

static int memlat_init(void)
{
	int ret;
#ifdef CONFIG_SYSFS
	ret = sysfs_create_group(mm_kobj, &memlat_attr_group);
	if (ret) {
		pr_err("register sysfs failed\n");
		return ret;
	}
#endif
        return 0;
}

static void memlat_exit(void)
{
	sysfs_remove_group(mm_kobj, &memlat_attr_group);
	return;
}

module_init(memlat_init);
module_exit(memlat_exit);
