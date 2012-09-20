bin_PROGRAMS += apps/ssactst/ssactst
apps_ssactst_ssactst_SOURCES = 	\
	apps/ssactst/ebb/Test.C \
	apps/ssactst/lib/SSACSimpleSharedArray.C \
	apps/ssactst/lib/CacheSimple.C \
	apps/ssactst/ssactst.cpp 
apps_ssactst_ssactst_CPPFLAGS = $(AM_CPPFLAGS) -DEBBLIB
apps_ssactst_ssactst_LDADD = $(ebbos_libs) $(LDADD)
apps_ssactst_ssactst_DEPENDENCIES = $(ebbos_libs)

