Name:       libmm-camcorder
Summary:    Camera and recorder library
Version:    0.10.192
Release:    0
Group:      Multimedia/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gio-2.0)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(libexif)
BuildRequires:  pkgconfig(mmutil-imgp)
BuildRequires:  pkgconfig(mmutil-jpeg)
BuildRequires:  pkgconfig(gstreamer-1.0)
BuildRequires:  pkgconfig(gstreamer-base-1.0)
BuildRequires:  pkgconfig(gstreamer-allocators-1.0)
BuildRequires:  pkgconfig(gstreamer-video-1.0)
BuildRequires:  pkgconfig(gstreamer-app-1.0)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(libtbm)
BuildRequires:  pkgconfig(storage)
%if "%{tizen_profile_name}" == "tv"
BuildRequires:  pkgconfig(tv-resource-manager)
BuildRequires:  pkgconfig(aul)
%else
BuildRequires:  pkgconfig(mm-resource-manager)
%endif
BuildRequires:  pkgconfig(ttrace)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(dpm)
BuildRequires:  pkgconfig(dlog)
%if "%{gtests}" == "1"
BuildRequires:  pkgconfig(gmock)
%endif

%description
Camera and recorder function supported library.


%package devel
Summary:    Camera and recorder library for development
Group:      libdevel
Version:    %{version}
Requires:   %{name} = %{version}-%{release}

%description devel
Camera and recorder function supported library for development.


%prep
%setup -q


%build
export CFLAGS+=" -D_LARGEFILE64_SOURCE -DGST_USE_UNSTABLE_API -DSYSCONFDIR=\\\"%{_sysconfdir}\\\" -DTZ_SYS_ETC=\\\"%{TZ_SYS_ETC}\\\""
./autogen.sh
%configure \
%if "%{tizen_profile_name}" == "tv"
	--enable-rm \
	--enable-product-tv \
%else
	--enable-mm-resource-manager \
%endif
%if "%{gtests}" == "1"
	--enable-gtests \
%endif
	--disable-static
make %{?jobs:-j%jobs}

%install
%make_install


%post
/sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest libmm-camcorder.manifest
%license LICENSE.APLv2
%defattr(-,root,root,-)
%{_libdir}/*.so.*
%{_datadir}/sounds/mm-camcorder/*
%if "%{gtests}" == "1"
%{_bindir}/gtests-libmm-camcorder
%endif

%files devel
%defattr(-,root,root,-)
%{_includedir}/mmf/mm_camcorder.h
%{_libdir}/pkgconfig/mm-camcorder.pc
%{_libdir}/*.so
