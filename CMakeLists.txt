
project(ncpufreqd C)

if (CMAKE_BUILD_TYPE MATCHES Debug)
	set(CMAKE_C_FLAGS "-g3 -O0")
endif()

add_subdirectory(src)
add_subdirectory(progs)
add_subdirectory(man8)

install(FILES ncpufreqd.conf DESTINATION /etc/)
