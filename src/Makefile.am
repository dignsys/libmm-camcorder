ACLOCAL_AMFLAGS='-I m4'

lib_LTLIBRARIES = libmmfcamcorder.la

includelibmmfcamcorderdir = $(includedir)/mmf

includelibmmfcamcorder_HEADERS = include/mm_camcorder.h

noinst_HEADERS = include/mm_camcorder_audiorec.h \
		 include/mm_camcorder_attribute.h \
		 include/mm_camcorder_configure.h \
		 include/mm_camcorder_gstcommon.h \
		 include/mm_camcorder_internal.h \
		 include/mm_camcorder_platform.h \
		 include/mm_camcorder_stillshot.h \
		 include/mm_camcorder_videorec.h \
		 include/mm_camcorder_util.h \
		 include/mm_camcorder_exifinfo.h \
		 include/mm_camcorder_exifdef.h \
		 include/mm_camcorder_sound.h \
		 include/mm_camcorder_resource.h

libmmfcamcorder_la_SOURCES = mm_camcorder.c \
			     mm_camcorder_internal.c \
			     mm_camcorder_attribute.c \
			     mm_camcorder_stillshot.c \
			     mm_camcorder_videorec.c \
			     mm_camcorder_audiorec.c \
			     mm_camcorder_gstcommon.c \
			     mm_camcorder_platform.c \
			     mm_camcorder_configure.c \
			     mm_camcorder_util.c \
			     mm_camcorder_exifinfo.c \
			     mm_camcorder_sound.c

libmmfcamcorder_la_CFLAGS = -I$(srcdir)/include \
			     $(GST_CFLAGS) \
			     $(GST_PLUGIN_BASE_CFLAGS) \
			     $(GST_VIDEO_CFLAGS) \
			     $(GST_APP_CFLAGS) \
			     $(EXIF_CFLAGS) \
			     $(MM_COMMON_CFLAGS) \
			     $(MMSOUND_CFLAGS) \
			     $(VCONF_CFLAGS) \
			     $(MMUTIL_IMGP_CFLAGS) \
			     $(MMUTIL_JPEG_CFLAGS) \
			     $(TBM_CFLAGS) \
			     $(STORAGE_CFLAGS) \
			     $(TTRACE_CFLAGS) \
			     $(DPM_CFLAGS) \
			     $(DLOG_CFLAGS) \
			     $(SYSTEMINFO_CFLAGS)

libmmfcamcorder_la_LIBADD = \
			    $(GLIB_LIBS) \
			    $(GIO_LIBS) \
			    $(GST_LIBS) \
			    $(GST_PLUGIN_BASE_LIBS) \
			    $(GST_VIDEO_LIBS) \
			    $(GST_APP_LIBS) \
			    $(MM_COMMON_LIBS) \
			    $(EXIF_LIBS) \
			    $(MMSOUND_LIBS) \
			    $(VCONF_LIBS) \
			    $(MMUTIL_IMGP_LIBS) \
			    $(MMUTIL_JPEG_LIBS) \
			    $(TBM_LIBS) \
			    $(TTRACE_LIBS) \
			    $(DPM_LIBS) \
			    $(DLOG_LIBS) \
			    $(STORAGE_LIBS)

libmmfcamcorder_la_CFLAGS += -DMMF_LOG_OWNER=0x010 -D_FILE_OFFSET_BITS=64
libmmfcamcorder_la_CFLAGS += -fdata-sections -ffunction-sections -Wl,--gc-sections
libmmfcamcorder_la_LDFLAGS = -Wl,--gc-sections
libmmfcamcorder_la_LIBADD += $(SYSTEMINFO_LIBS)

if MURPHY_SUPPORT
libmmfcamcorder_la_SOURCES += mm_camcorder_resource.c
libmmfcamcorder_la_CFLAGS += $(MURPHY_RESOURCE_CFLAGS) $(MURPHY_GLIB_CFLAGS) -D_MMCAMCORDER_MURPHY_SUPPORT
libmmfcamcorder_la_LIBADD += $(MURPHY_RESOURCE_LIBS) $(MURPHY_GLIB_LIBS)
endif

if RM_SUPPORT
libmmfcamcorder_la_CFLAGS += $(RM_CFLAGS) $(AUL_CFLAGS) -D_MMCAMCORDER_RM_SUPPORT
libmmfcamcorder_la_LIBADD += $(RM_LIBS) $(AUL_LIBS)
endif

if PRODUCT_TV
libmmfcamcorder_la_CFLAGS += -D_MMCAMCORDER_PRODUCT_TV
endif

install-exec-hook:
					mkdir -p $(DESTDIR)$(prefix)/share/sounds/mm-camcorder && \
					cp $(srcdir)/../sounds/* $(DESTDIR)$(prefix)/share/sounds/mm-camcorder/

