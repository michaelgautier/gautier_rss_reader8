Name:           gautierrss
Version:        8.0.0
Release:        1%{?dist}
Summary:        Ultra simple desktop feed reader

#All files included in the source RPM build are to be single licensed under LGPLv2+
License:        LGPLv2+
URL:            https://michaelgautier.github.io/gautier_rss_reader8/
Source0:        %{name}-%{version}.tar.gz

BuildRequires:	pkgconfig(gio-2.0)
BuildRequires:	pkgconfig(glib-2.0)
BuildRequires:	pkgconfig(gdk-3.0)
BuildRequires:	pkgconfig(gdk-pixbuf-2.0)
BuildRequires:	pkgconfig(gtk+-3.0)
BuildRequires:	pkgconfig(sqlite3)
BuildRequires:	pkgconfig(libcurl)
BuildRequires:	pkgconfig(webkit2gtk-4.0)
BuildRequires:	pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(appstream-glib)
Requires:	gtk3 sqlite curl webkit2gtk4.0 libxml2

%description
Shows Internet feeds in a clean and obvious way. Comes with a screen that helps  
you add new feeds or purge those you no longer need. 

The program uses a native code UI that runs smooth with no bloat.

NOTE:

- Feed providers prefer their site is accessed no earlier than once per hour.
- The program respects those conditions by default.

- This version 8 is an exact match of version 7.
- Upgraded to use Autotools for build and distribution.

%prep
%autosetup
%configure
%make_build
make %_smp_mflags -j1
%install
%make_install %_smp_mflags -j1
desktop-file-install --dir=%{buildroot}%{_datadir}/applications/ %{_builddir}/%{name}-%{version}/desktop/newsreader.desktop
desktop-file-validate %{_builddir}/%{name}-%{version}/desktop/newsreader.desktop
appstream-util validate-relax --nonet %{_builddir}/%{name}-%{version}/desktop/newsreader.appdata.xml
%clean
%files
%license LICENSE
%{_bindir}/%{name}
%{_bindir}/newsdownld
%{_bindir}/rss_io_test
%{_datadir}/applications/newsreader.desktop
%{_datadir}/icons/HighContrast/16x16/apps/newsreader.png
%{_datadir}/icons/HighContrast/22x22/apps/newsreader.png
%{_datadir}/icons/HighContrast/24x24/apps/newsreader.png
%{_datadir}/icons/HighContrast/256x256/apps/newsreader.png
%{_datadir}/icons/HighContrast/32x32/apps/newsreader.png
%{_datadir}/icons/HighContrast/48x48/apps/newsreader.png
%{_datadir}/icons/hicolor/128x128/apps/newsreader.png
%{_datadir}/icons/hicolor/16x16/apps/newsreader.png
%{_datadir}/icons/hicolor/32x32/apps/newsreader.png
%{_datadir}/icons/hicolor/48x48/apps/newsreader.png
%{_datadir}/icons/hicolor/64x64/apps/newsreader.png
%{_datadir}/pixmaps/newsreader.xpm
%{_datadir}/desktop/newsreader.appdata.xml

%changelog
* Mon Jan 16 2023 Michael Gautier <michaelgautier.wordpress.com> - 8.8.0
- Adding RPM spec file
* Sat Jan 14 2023 Michael Gautier <michaelgautier.wordpress.com> - 7.7.1
- Completed Autotools conversion
