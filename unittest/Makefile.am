bin_PROGRAMS = gtests-libmm-camcorder

gtests_libmm_camcorder_SOURCES = gtests_libmm_camcorder.cpp

gtests_libmm_camcorder_CXXFLAGS = \
	-I$(top_srcdir)/src/include\
	$(GST_CFLAGS)\
	$(MM_COMMON_CFLAGS)\
	$(GMOCK_CFLAGS)

gtests_libmm_camcorder_DEPENDENCIES = \
	$(top_srcdir)/src/libmmfcamcorder.la

gtests_libmm_camcorder_LDADD = \
	$(GMOCK_LIBS) \
	$(top_srcdir)/src/libmmfcamcorder.la

