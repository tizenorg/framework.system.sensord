Name:       sensord
Summary:    Sensor daemon
Version:    1.1.8
Release:    0
Group:      Framework/system
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    sensord.service
Source2:    sensord.socket

%define accel_state ON
%define gyro_state ON
%define proxi_state OFF
%define light_state OFF
%define geo_state OFF
%define pedo_state OFF
%define flat_state OFF
%define context_state ON
%define bio_state ON
%define bio_hrm_state ON

Requires(post): /usr/bin/vconftool

BuildRequires:  cmake
BuildRequires:  vconf-keys-devel
BuildRequires:  libattr-devel
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(deviced)
BuildRequires:  pkgconfig(libsystemd-daemon)

%description
Sensor daemon

%package sensord
Summary:    Sensor daemon
Group:      main
Requires:   %{name} = %{version}-%{release}

%description sensord
Sensor daemon

%package -n libsensord
Summary:    Sensord library
Group:      main
Requires:   %{name} = %{version}-%{release}

%description -n libsensord
Sensord library

%package -n libsensord-devel
Summary:    Sensord library (devel)
Group:      main
Requires:   %{name} = %{version}-%{release}

%description -n libsensord-devel
Sensord library (devel)

%prep
%setup -q

%build
#CFLAGS+=" -fvisibility=hidden "; export CFLAGS
#CXXFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden ";export CXXFLAGS
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DACCEL=%{accel_state} \
	-DGYRO=%{gyro_state} -DPROXI=%{proxi_state} -DLIGHT=%{light_state} \
	-DGEO=%{geo_state} -DPEDO=%{pedo_state} -DCONTEXT=%{context_state} \
	-DFLAT=%{flat_state} -DBIO=%{bio_state} -DBIO_HRM=%{bio_hrm_state}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/usr/share/license

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
mkdir -p %{buildroot}%{_libdir}/systemd/system/sockets.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/
install -m 0644 %SOURCE2 %{buildroot}%{_libdir}/systemd/system/
ln -s ../sensord.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/sensord.service
ln -s ../sensord.socket  %{buildroot}%{_libdir}/systemd/system/sockets.target.wants/sensord.socket

mkdir -p %{buildroot}/etc/smack/accesses2.d

cp sensord.rule %{buildroot}/etc/smack/accesses2.d/sensord.rule

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

systemctl daemon-reload

%files -n sensord
%manifest sensord.manifest
%{_bindir}/sf_server
%attr(0644,root,root)/usr/etc/sf_data_stream.conf
%attr(0644,root,root)/usr/etc/sf_filter.conf
%attr(0644,root,root)/usr/etc/sf_processor.conf
%attr(0644,root,root)/usr/etc/sf_sensor.conf
%attr(0644,root,root)/usr/etc/sensors.xml
%{_libdir}/systemd/system/sensord.service
%{_libdir}/systemd/system/sensord.socket
%{_libdir}/systemd/system/multi-user.target.wants/sensord.service
%{_libdir}/systemd/system/sockets.target.wants/sensord.socket
%{_datadir}/license/sensord
/etc/smack/accesses2.d/sensord.rule

%files -n libsensord
%manifest libsensord.manifest
%defattr(-,root,root,-)
%{_libdir}/libsensor.so.*
%{_libdir}/sensor_framework/*.so*
%{_libdir}/libsf_common.so
%{_datadir}/license/libsensord

%files -n libsensord-devel
%defattr(-,root,root,-)
%{_includedir}/sensor/*.h
%{_includedir}/sf_common/*.h
%{_libdir}/libsensor.so
%{_libdir}/pkgconfig/sensor.pc
%{_libdir}/pkgconfig/sf_common.pc
