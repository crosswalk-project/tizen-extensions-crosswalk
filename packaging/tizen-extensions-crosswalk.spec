%define _manifestdir /opt/share/packages
%define _desktop_icondir /opt/share/icons/default/small
%define _bluetooth_demo_package tizen-extensions-crosswalk-bluetooth-demo
%define _examples_package tizen-extensions-crosswalk-examples
%define _system_info_demo_package tizen-extensions-crosswalk-system-info-demo

Name:       tizen-extensions-crosswalk
Version:    0.3
Release:    0
License:    BSD-3-Clause
Group:      Development/Libraries
Summary:    Tizen Web APIs implemented using Crosswalk
URL:        https://github.com/otcshare/tizen-extensions-crosswalk
Source0:    %{name}-%{version}.tar.gz
Source1:    %{name}
Source2:    %{name}.png
Source3:    %{_bluetooth_demo_package}
Source4:    %{_examples_package}
Source5:    %{_system_info_demo_package}
Source1001: %{name}.manifest

BuildRequires: python
BuildRequires: pkgconfig(appcore-common)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-network-bluetooth)
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(capi-system-info)
BuildRequires: pkgconfig(capi-system-power)
BuildRequires: pkgconfig(capi-system-runtime-info)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(capi-telephony-sim)
BuildRequires: pkgconfig(capi-web-url-download)
BuildRequires: pkgconfig(dbus-glib-1)
BuildRequires: pkgconfig(bluez)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(libudev)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(pmapi)
BuildRequires: pkgconfig(x11)
BuildRequires: pkgconfig(xrandr)
BuildRequires: pkgconfig(vconf)
Requires:      crosswalk

%description
Tizen Web APIs implemented using Crosswalk.

%package -n %{_bluetooth_demo_package}
Summary: Tizen Web APIs using Crosswalk demo
Group: Development/Libraries
Requires:      %{name}

%description  -n %{_bluetooth_demo_package}
Tizen Web APIs bluetooth demo implementation using Crosswalk.

%package -n %{_examples_package}
Summary: Tizen Web APIs using Crosswalk examples
Group: Development/Libraries
Requires:      %{name}

%description  -n %{_examples_package}
Tizen Web APIs examples using Crosswalk.

%package -n %{_system_info_demo_package}
Summary: Tizen Web APIs using Crosswalk system info demo
Group: Development/Libraries
Requires:      %{name}

%description  -n %{_system_info_demo_package}
Tizen Web APIs system info demo implementation using Crosswalk.

%prep
%setup -q

cp %{SOURCE1001} .
cp %{SOURCE2} .
cp %{SOURCE3} .
cp %{SOURCE4} .
cp %{SOURCE5} .

%build

export GYP_GENERATORS='make'
./tools/gyp/gyp \
--depth=.       \
-Dextension_build_type=Debug   \
tizen-wrt.gyp

make %{?_smp_mflags}

%install

# Binary wrapper.
install -m 755 -D %{SOURCE1} %{buildroot}%{_bindir}/%{name}
install -m 755 -D %{SOURCE3} %{buildroot}%{_bindir}/%{_bluetooth_demo_package}
install -m 755 -D %{SOURCE4} %{buildroot}%{_bindir}/%{_examples_package}
install -m 755 -D %{SOURCE5} %{buildroot}%{_bindir}/%{_system_info_demo_package}

# Extensions.
mkdir -p %{buildroot}%{_libdir}/%{name}
install -p -m 644 out/Default/libtizen*.so %{buildroot}%{_libdir}/%{name}

# Examples.
mkdir -p %{buildroot}%{_datarootdir}/%{name}/examples
mkdir -p %{buildroot}%{_datarootdir}/%{name}/examples/js
install -p -m 644 examples/*.html %{buildroot}%{_datarootdir}/%{name}/examples
install -p -m 644 examples/js/*.js %{buildroot}%{_datarootdir}/%{name}/examples/js

# Demos
mkdir -p %{buildroot}%{_datarootdir}/%{name}/demos/tizen
mkdir -p %{buildroot}%{_datarootdir}/%{name}/demos/tizen/css
mkdir -p %{buildroot}%{_datarootdir}/%{name}/demos/tizen/js
mkdir -p %{buildroot}%{_datarootdir}/%{name}/demos/tizen/images

install -p -m 644 demos/tizen/*.html %{buildroot}%{_datarootdir}/%{name}/demos/tizen
install -p -m 644 demos/tizen/css/*.css %{buildroot}%{_datarootdir}/%{name}/demos/tizen/css
install -p -m 644 demos/tizen/js/*.js %{buildroot}%{_datarootdir}/%{name}/demos/tizen/js
install -p -m 644 demos/tizen/images/*.png %{buildroot}%{_datarootdir}/%{name}/demos/tizen/images

# Demos - System Info
mkdir -p %{buildroot}%{_datarootdir}/%{name}/demos/system_info
mkdir -p %{buildroot}%{_datarootdir}/%{name}/demos/system_info/css
mkdir -p %{buildroot}%{_datarootdir}/%{name}/demos/system_info/js
mkdir -p %{buildroot}%{_datarootdir}/%{name}/demos/system_info/images

install -p -m 644 demos/system_info/*.html %{buildroot}%{_datarootdir}/%{name}/demos/system_info
install -p -m 644 demos/system_info/css/*.css %{buildroot}%{_datarootdir}/%{name}/demos/system_info/css
install -p -m 644 demos/system_info/js/*.js %{buildroot}%{_datarootdir}/%{name}/demos/system_info/js
install -p -m 644 demos/system_info/images/*.png %{buildroot}%{_datarootdir}/%{name}/demos/system_info/images

# register to the package manager
install -m 644 -D %{_examples_package}.xml %{buildroot}%{_manifestdir}/%{_examples_package}.xml
install -m 644 -D %{_bluetooth_demo_package}.xml %{buildroot}%{_manifestdir}/%{_bluetooth_demo_package}.xml
install -m 644 -D %{_system_info_demo_package}.xml %{buildroot}%{_manifestdir}/%{_system_info_demo_package}.xml
install -p -D %{name}.png %{buildroot}%{_desktop_icondir}/%{_examples_package}.png
install -p -D %{name}.png %{buildroot}%{_desktop_icondir}/%{_bluetooth_demo_package}.png
install -p -D %{name}.png %{buildroot}%{_desktop_icondir}/%{_system_info_demo_package}.png

%files
# TODO(rakuco): This causes problems on 2.1 when creating the package.
# %license LICENSE
%{_bindir}/%{name}
%{_libdir}/%{name}/libtizen*.so

%files -n %{_bluetooth_demo_package}
%{_bindir}/%{_bluetooth_demo_package}
%{_manifestdir}/%{_bluetooth_demo_package}.xml
%{_desktop_icondir}/%{_bluetooth_demo_package}.png
%{_datarootdir}/%{name}/demos/tizen/*.html
%{_datarootdir}/%{name}/demos/tizen/css/*.css
%{_datarootdir}/%{name}/demos/tizen/js/*.js
%{_datarootdir}/%{name}/demos/tizen/images/*.png

%files -n %{_examples_package}
%{_bindir}/%{_examples_package}
%{_manifestdir}/%{_examples_package}.xml
%{_desktop_icondir}/%{_examples_package}.png
%{_datarootdir}/%{name}/examples/*.html
%{_datarootdir}/%{name}/examples/js/*.js

%files -n %{_system_info_demo_package}
%{_bindir}/%{_system_info_demo_package}
%{_manifestdir}/%{_system_info_demo_package}.xml
%{_desktop_icondir}/%{_system_info_demo_package}.png
%{_datarootdir}/%{name}/demos/system_info/*.html
%{_datarootdir}/%{name}/demos/system_info/css/*.css
%{_datarootdir}/%{name}/demos/system_info/js/*.js
%{_datarootdir}/%{name}/demos/system_info/images/*.png
