
project(ncpufreqd-progs)

set(SOURCEs
	ncpuauto.c
)

add_custom_target(
	ncpupowersave ALL
	COMMAND ln -sf ncpuauto ncpupowersave
	DEPENDS ncpuauto
	VERBATIM)

add_custom_target(
	ncpuperformance ALL
	COMMAND ln -sf ncpuauto ncpuperformance
	DEPENDS ncpuauto
	VERBATIM)

add_executable(ncpuauto ${SOURCEs})

add_executable(ncpupowersave ncpuauto)
add_executable(ncpuperformance ncpuauto)

install(TARGETS ncpuauto DESTINATION bin/)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/ncpuperformance ${CMAKE_CURRENT_BINARY_DIR}/ncpupowersave DESTINATION bin/)
