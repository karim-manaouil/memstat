#include <linux/perf_event.h>

k:perf_pmu_enable {
	$s = ksym(((struct pmu *)arg0)->pmu_enable);
	printf("%s\n", $s);
}
