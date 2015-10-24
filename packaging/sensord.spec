Name:       sensord
Summary:    Sensor daemon
Version:    1.2.0
Release:    0
Group:      System/Sensor Framework
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    sensord.service
Source2:    sensord.socket
Source3:    sensord_event.socket
BuildRequires:  cmake

%define BUILD_PROFILE %{?profile}%{!?profile:%{?tizen_profile_name}}

%if "%{?BUILD_PROFILE}" != "tv"
##### Begin: Normal build spec #####


BuildRequires:  vconf-keys-devel
BuildRequires:  libattr-devel
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(libsystemd-daemon)

%define accel_state ON
%define gyro_state ON
%define gyro_uncal_state ON
%define proxi_state ON
%define light_state ON
%define geo_state ON
%define geo_uncal_state ON
%define pedo_state OFF
%define flat_state OFF
%define context_state ON
%define bio_state ON
%define bio_hrm_state ON
%define auto_rotation_state ON
%define gravity_state ON
%define linear_accel_state ON
%define motion_state OFF
%define rv_state ON
%define geomagnetic_rv_state ON
%define gaming_rv_state ON
%define rv_raw_state OFF
%define orientation_state ON
%define pressure_state ON
%define sensor_fusion_state ON
%define pir_state ON
%define pir_long_state ON
%define temperature_state ON
%define humidity_state ON
%define ultraviolet_state ON
%define dust_state ON
%define bio_led_green_state ON
%define zone_enable ON
%define tilt_state ON
%define build_test_suite ON

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

%if %{build_test_suite} == "ON"
%package -n sensor-test
Summary:    Sensord library
Group:      System/Testing
Requires:   %{name} = %{version}-%{release}

%description -n sensor-test
Sensor functional testing

%endif

%prep
%setup -q

%build
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DFULLVER=%{version} -DMAJORVER=${MAJORVER} \
	-DACCEL=%{accel_state} \
	-DGYRO=%{gyro_state} -DGYRO_UNCAL=%{gyro_uncal_state} -DPROXI=%{proxi_state} -DLIGHT=%{light_state} \
	-DGEO=%{geo_state} -DGEO_UNCAL=%{geo_uncal_state} -DPEDO=%{pedo_state} -DCONTEXT=%{context_state} \
	-DFLAT=%{flat_state} -DBIO=%{bio_state} -DBIO_HRM=%{bio_hrm_state} \
	-DAUTO_ROTATION=%{auto_rotation_state} -DGRAVITY=%{gravity_state} \
	-DLINEAR_ACCEL=%{linear_accel_state} -DMOTION=%{motion_state} \
	-DRV=%{rv_state} -DPRESSURE=%{pressure_state} \
	-DGEOMAGNETIC_RV=%{geomagnetic_rv_state} -DGAMING_RV=%{gaming_rv_state} \
	-DTILT=%{tilt_state} -DTEST_SUITE=%{build_test_suite} \
	-DORIENTATION=%{orientation_state} -DSENSOR_FUSION=%{sensor_fusion_state} \
	-DPIR=%{pir_state} -DPIR_LONG=%{pir_long_state} \
	-DTEMPERATURE=%{temperature_state} -DHUMIDITY=%{humidity_state} \
	-DULTRAVIOLET=%{ultraviolet_state} -DDUST=%{dust_state} \
	-DBIO_LED_GREEN=%{bio_led_green_state} \
	-DRV_RAW=%{rv_raw_state} -DTIZEN_ZONE_ENABLED=%{zone_enable}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/usr/share/license

mkdir -p %{buildroot}%{_libdir}/systemd/system/sockets.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/
install -m 0644 %SOURCE2 %{buildroot}%{_libdir}/systemd/system/
install -m 0644 %SOURCE3 %{buildroot}%{_libdir}/systemd/system/
ln -s ../sensord.socket  %{buildroot}%{_libdir}/systemd/system/sockets.target.wants/sensord.socket
ln -s ../sensord_event.socket  %{buildroot}%{_libdir}/systemd/system/sockets.target.wants/sensord_event.socket

mkdir -p %{buildroot}/etc/smack/accesses.d
cp sensord.efl %{buildroot}/etc/smack/accesses.d/sensord.efl

%post
mkdir -p %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/
ln -sf %{_libdir}/systemd/system/sensord.service %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/

%postun
systemctl daemon-reload

%files -n sensord
%manifest sensord.manifest
%{_bindir}/sensord
%attr(0644,root,root)/usr/etc/sensor_plugins.xml
%attr(0644,root,root)/usr/etc/sensors.xml
%attr(0644,root,root)/usr/etc/virtual_sensors.xml
%{_libdir}/systemd/system/sensord.service
%{_libdir}/systemd/system/sensord.socket
%{_libdir}/systemd/system/sensord_event.socket
%{_libdir}/systemd/system/sockets.target.wants/sensord.socket
%{_libdir}/systemd/system/sockets.target.wants/sensord_event.socket
%{_datadir}/license/sensord
/etc/smack/accesses.d/sensord.efl
%if %{?zone_enable} == ON
/etc/vasum/vsmzone.resource/sensord.res
%endif

%files -n libsensord
%manifest libsensord.manifest
%defattr(-,root,root,-)
%{_libdir}/libsensor.so.*
%{_libdir}/sensord/*.so*
%{_libdir}/libsensord-share.so
%{_libdir}/libsensord-server.so
%{_datadir}/license/libsensord

%files -n libsensord-devel
%defattr(-,root,root,-)
%{_includedir}/sensor/*.h
%{_includedir}/sf_common/*.h
%{_libdir}/libsensor.so
%{_libdir}/pkgconfig/sensor.pc
%{_libdir}/pkgconfig/sf_common.pc
%{_libdir}/pkgconfig/sensord-server.pc

%if %{build_test_suite} == "ON"
%files -n sensor-test
%defattr(-,root,root,-)
%{_bindir}/api-test
%{_bindir}/sensor-test
%{_bindir}/performance-test
%{_bindir}/fusion-data-collection
%endif


##### End: Normal build spec #####
%else
##### Begin: Dummy build spec #####


%description
Sensor daemon

%package -n libsensord
Summary:    Sensord library
Group:      System/Sensor Framework

%description -n libsensord
Sensord library

%package -n libsensord-devel
Summary:    Sensord library (devel)
Group:      System/Sensor Framework

%description -n libsensord-devel
Sensord library dummy

%prep
%setup -q

%build
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
cmake ./dummy -DCMAKE_INSTALL_PREFIX=/usr -DFULLVER=%{version} -DMAJORVER=${MAJORVER}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%files -n sensord
%manifest sensord.manifest
%{_datadir}/license/sensord

%files -n libsensord
%{_datadir}/license/libsensord

%files -n libsensord-devel
%manifest libsensord.manifest
%{_includedir}/sensor/*.h
%{_libdir}/pkgconfig/*.pc


##### End: Dummy build spec #####
%endif
