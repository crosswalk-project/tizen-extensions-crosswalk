Name:       tizen-extensions-crosswalk
Version:    1.0.0
Release:    0
License:    BSD-3-Clause
Group:      Development/Libraries
Summary:    Tizen Web APIs implemented using Crosswalk
URL:        https://github.com/otcshare/tizen-extensions-crosswalk
Source0:    %{name}-%{version}.tar.gz
Source1:    %{name}
Source1001: %{name}.manifest

BuildRequires: python
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
-Dbuild=Debug   \
-Dtype=mobile   \
tizen-wrt.gyp

make %{?_smp_mflags}

%install

# Binary wrapper.
install -m 755 -D %{SOURCE1} %{buildroot}%{_bindir}/%{name}

# Extensions.
mkdir -p %{buildroot}%{_libdir}/%{name}
install -p -m 644 out/Default/libtizen_*.so %{buildroot}%{_libdir}/%{name}

# Examples.
mkdir -p %{buildroot}%{_datarootdir}/%{name}/examples
install -p -m 644 examples/* %{buildroot}%{_datarootdir}/%{name}/examples

%files
# TODO(rakuco): This causes problems on 2.1 when creating the package.
# %license LICENSE
%{_bindir}/%{name}
%{_libdir}/%{name}/libtizen_*.so
%{_datarootdir}/%{name}/examples/*
