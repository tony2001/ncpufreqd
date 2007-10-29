
project(ncpufreqd)

add_subdirectory(src)
add_subdirectory(progs)
add_subdirectory(man8)

install(FILES ncpufreqd.conf DESTINATION /etc/)
