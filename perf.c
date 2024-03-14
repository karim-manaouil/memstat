/* GPL v2 Software
 *
 * Author: Karim Manaouil <k.manaouil@gmail.com>
 *		Edinburgh University - 2024
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>

#define len(a)	(sizeof(a)/sizeof(*a))

#define EVENT_SOURCE_PATH "/sys/bus/event_source/devices"

#define MAX_CPUS	256
#define MAX_EVENTS	16

int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
		    int group_fd, unsigned long flags);

int perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
		    int group_fd, unsigned long flags)
{
	return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

struct pmu {
	const char *name;
	int type;
};

struct event {
	const char *name;
	const char *unit;
	unsigned long long umask;
	unsigned long long evsel;
};

#define for_each_event(e, events) \
	for((e) = events; e < (events) + len(events); (e)++)

#define AMD_UMASK(e)	(((e)->umask & 0xff) << 8)
#define AMD_EVSEL(e)	(((e)->evsel & 0xff) | (((e)->evsel & 0xf00) << 32))
#define AMD_CONFIG(e)	(AMD_EVSEL(e) | AMD_UMASK(e))

struct perf_event {
	int cpu;
	struct pmu *pmu;
	struct event *event;
	struct perf_event_attr attr;
	unsigned long long counter;
};

#define for_each_perf_event(e, cpu, pcp) \
	for((e) = pcp->events; e < (pcp)->events + (pcp)->nr_events; (e)++)


struct pcp_event_list {
	int cpu;
	int nr_events;
	struct perf_event events[MAX_EVENTS];
};

struct pmu pmus[] = {
	{ "cpu", -1 },
	{ "amd_l3", -1 },
};

#define for_each_pmu(pmu, pmus) \
	for ((pmu) = pmus; pmu < (pmus) + len(pmus); (pmu)++)

struct pcp_event_list pcp_evl[MAX_CPUS];

/* Those events are for amd 19h family aka zen4 */
struct event events[] = {
	/* ls_any_fills_from_sys.dram_io_near */
	{.name = "local_dram_cache_fill", .unit = "cpu", .umask = 0x08, .evsel = 0x44},
	/* ls_any_fills_from_sys.dram_io_far */
	{.name = "remote_dram_cache_fill", .unit = "cpu", .umask = 0x40, .evsel = 0x44},
	/* l3_lookup_state.l3_miss */
	{.name = "l3_miss", .unit = "amd_l3", .umask = 0x01, .evsel = 0x04},
	/* l3_lookup_state.l3_miss */
	{.name = "l3_hit", .unit = "amd_l3", .umask = 0xfe, .evsel = 0x04},
};

struct pmu *find_pmu(const char *name)
{
	struct pmu *pmu;
	char buf[256];
	FILE *file;

	for_each_pmu(pmu, pmus)
		if (!strcmp(pmu->name, name))
			goto found;
	return NULL;
found:
	if (pmu->type != -1)
		return pmu;

	sprintf(buf, "%s/%s/type", EVENT_SOURCE_PATH, pmu->name);

	file = fopen(buf, "r");
	if (file == NULL)
		return NULL;

	fread(buf, sizeof(*buf), 16, file);
	pmu->type = atoi(buf);

	return pmu;
}

int perf_init()
{
	struct pmu *pmu;
	struct event *event;
	struct perf_event *pevent;

	for_each_event(event, events) {
		pmu = find_pmu(events->unit);
		if (!pmu)
			exit(1);
		printf("name=%s, unit=%s, type=%d, umask=0x%llx, evsel=0x%llx, "
		       "config=0x%llx\n",
		       event->name, event->unit, pmu->type,
		       event->umask, event->evsel, AMD_CONFIG(event));
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct perf_event_attr attr = {0};
	int pid = 0, cpu = -1, kern = 1;
	int opt, fd, ret, msec = 0;

	if (sizeof(__u64) != sizeof(unsigned long)) {
		fprintf(stderr, "Machine not supported\n");
		exit(EXIT_FAILURE);
	}

	while ((opt = getopt(argc, argv, "p:c:e:s:k")) != -1) {
		switch (opt) {
		case 'p':
			pid = atoi(optarg);
			break;
		case 'c':
			cpu = atoi(optarg);
			break;
		case 's':
			msec = atoi(optarg);
			break;
		case 'k':
			kern = 0;
			break;
		default:
			fprintf(stderr, "Option not recognized!\n");
			exit(EXIT_FAILURE);
		}
	}

	printf("pid=%d, cpu=%d, sleep=%d\n", pid, cpu, msec);

	ret = perf_init();
	if (ret) {
		fprintf(stderr, "Error initialisig %d\n", ret);
		exit(1);
	}

	attr.type = 13;
	attr.size = sizeof(attr);
	attr.config = 0;
	attr.disabled = 1;
	attr.exclude_kernel = kern;
	attr.exclude_hv = 1;

	fd = perf_event_open(&attr, pid, cpu, -1, 0);
	if (fd == -1) {
		fprintf(stderr, "Error opening leader %llx\n", attr.config);
		exit(EXIT_FAILURE);
	}

	ioctl(fd, PERF_EVENT_IOC_RESET, 0);
	ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

	if (msec)
		usleep(msec * 1000);

	ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);

	close(fd);
}
