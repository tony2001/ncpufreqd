
project(ncpufreqd C)

set(HEADERs
	config.h
	defs.h
	functions.h
	handler_acpithr.h
	handler_cpufreq.h
)

set(SOURCEs
	config.c
	functions.c
	handler_acpithr.c
	handler_cpufreq.c
	ncpufreqd.c
)

add_executable(ncpufreqd ${SOURCEs})

install(TARGETS ncpufreqd DESTINATION sbin)
