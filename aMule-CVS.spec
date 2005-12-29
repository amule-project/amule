%define cvsdate %(date +%Y%m%d)
%define optflags %{nil}

Summary:        aMule - another eMule p2p client
Name:           aMule
Version:        CVS
Release:        %{cvsdate}
License:        GPL
Group:          Applications/Internet
Packager:       The aMule Team (http://forum.amule.org/)
Vendor:         The aMule Project
URL:            http://www.amule.org/
Source:         %{name}-%{version}-%{release}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%description
aMule is a peer to peer file sharing client, based on the well known eMule.
Starting with 2.0.0 aMule works on Linux, Mac, *BSD and Windows, which makes it
the first multi-platform edonkey network client.

%pre
echo "****************************************************************************"
echo "Warning: This is a cvs release!"
echo "This release is made for testing purpose and it may cause several problems,"
echo "burn your house, kill your dog, etc, but it *should* be safe to use anyway."
echo "If you would like to test some of the great new features go on and install."
echo "Otherwise you may press ctrl-c within the next 10 seconds to abort the"
echo "installation."
echo -n "Waiting for user... "
for i in $(seq 10 -1 1); do
    echo -n "$i, "
    sleep 1
done
echo " 0, ok, here we go then... Muhahaha :), installing."

%prep
%setup -q -n amule-cvs

%build
%configure \
        --disable-optimize \
        --enable-debug \
        --enable-cas \
        --enable-wxcas \
        --enable-amulecmd \
        --enable-webserver \
        --enable-ccache
%{__make} %{?_smp_mflags}

%install
[ ! "$RPM_BUILD_ROOT" = "/" ] && %{__rm} -rf "$RPM_BUILD_ROOT"
%makeinstall
%find_lang amule

%clean
[ ! "$RPM_BUILD_ROOT" = "/" ] && %{__rm} -rf "$RPM_BUILD_ROOT"

%files -f amule.lang
%defattr(-,root,root,-)
%{_bindir}/amule
%{_bindir}/ed2k
%{_bindir}/amulecmd
%{_bindir}/cas
%{_bindir}/wxcas
%{_bindir}/amuleweb
%{_libdir}/xchat/plugins/xas.pl
%{_datadir}/applications/*
%{_datadir}/pixmaps/*
%{_mandir}/man1/*
%{_mandir}/*/man1/*
%docdir %{_datadir}/doc/%{name}-%{version}
%{_datadir}/doc/%{name}-%{version}
%{_datadir}/cas
%{_datadir}/amule

%changelog
* Sun Dec 25 2005 Gerd78 <aMule forum>
- Really disable optimizations by defining optflags to nil.

* Mon Apr 19 2005 Marcelo Jimenez <phoenix@amule.org>
- Removed curl dependency, aMule now uses wxHTTP.

* Mon Mar 26 2005 Marcelo Jimenez <phoenix@amule.org>
- Added a distro test, so we know the distro.
- Tests for UTF-8 enabled LANG to use UTF-8 systray.

* Mon Mar 21 2005 Marcelo Jimenez <phoenix@amule.org>
- Removed krb5-libs require and krb5-devel buildprereq. curl-lib and
  curl-devel is enough.

* Tue Mar 08 2005 Marcelo Jimenez <phoenix@amule.org>
- Made it work with cvs snapshots at their very same day.

* Wed Jun 16 2004 Ariano Bertacca <ariano@hirnriss.net>
- added tool package.

* Sat May 22 2004 Ariano Bertacca <ariano@hirnriss.net>
- added webserver package.

* Sun Mar 28 2004 Ariano Bertacca <ariano@hirnriss.net>
- added libcryptopp to dependencies/BuildPreReq
- getting wx-config and wxbase-config from installed rpm to avoid problems
  with people using wxGTK-2.5.
  Idea taken from aMule-2.0.0rc1.spec done by
  deltaHF <deltahf@users.sourceforge.net> and
  pure_ascii <pure_ascii@users.sourceforge.net>

* Tue Feb 10 2004 Ariano Bertacca <ariano@hirnriss.net>
- modified the BuildPreReq to satisfy amulecmd build requirements.

* Sat Jan 23 2004 Ariano Bertacca <ariano@hirnriss.net>
- initial amule.spec release

