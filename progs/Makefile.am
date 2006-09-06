
sbin_PROGRAMS = ncpuauto

ncpuauto_SOURCES = ncpuauto.c

install-exec-hook:
	cd $(DESTDIR)$(sbindir) && $(LN_S) ncpuauto ncpupowersave
	cd $(DESTDIR)$(sbindir) && $(LN_S) ncpuauto ncpuperformance
