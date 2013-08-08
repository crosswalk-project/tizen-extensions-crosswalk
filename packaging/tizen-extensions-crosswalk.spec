Name:       tizen-extensions-crosswalk
Version:    0.2
Release:    0
License:    BSD-3-Clause
Group:      Development/Libraries
Summary:    Tizen Web APIs implemented using Crosswalk
URL:        https://github.com/otcshare/tizen-extensions-crosswalk
Source0:    %{name}-%{version}.tar.gz
Source1:    %{name}
Source1001: %{name}.manifest

BuildRequires: python
BuildRequires: pkgconfig(capi-network-connection)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(capi-system-power)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(dbus-glib-1)
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

%prep
%setup -q

cp %{SOURCE1001} .

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

install -p -m 644 demos/tizen/*.html %{buildroot}%{_datarootdir}/%{name}/demos/tizen
install -p -m 644 demos/tizen/css/*.css %{buildroot}%{_datarootdir}/%{name}/demos/tizen/css
install -p -m 644 demos/tizen/js/*.js %{buildroot}%{_datarootdir}/%{name}/demos/tizen/js

%files
# TODO(rakuco): This causes problems on 2.1 when creating the package.
# %license LICENSE
%{_bindir}/%{name}
%{_libdir}/%{name}/libtizen*.so
%{_datarootdir}/%{name}/examples/*.html
%{_datarootdir}/%{name}/examples/js/*.js

%{_datarootdir}/%{name}/demos/tizen/*.html
%{_datarootdir}/%{name}/demos/tizen/css/*.css
%{_datarootdir}/%{name}/demos/tizen/js/*.js
