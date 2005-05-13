
Summary:        aMule - another eMule p2p client
Name:           aMule

Version:        2.0.0
Release:        1

Group:          Applications/Internet
License:        GPL
URL:            http://www.amule.org
Source0:        aMule-%{version}.tar.bz2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  grep, automake >= 1.7, wxGTK2-devel
Requires:       wxGTK2

%description
aMule is a peer to peer file sharing client, based on the well known eMule.
Starting with 2.0.0 aMule works on Linux, Mac, *BSD and Windows, which makes it the first multi-platform edonkey network client.


%prep
%setup -q


%build
#
# Tests for Fedora Core distro and then for UTF-8 enabled locale.
# For some reason, rpmbuild sets LANG="C" before executing prep section,
# so, don't waste your time :)
#
# I can put other hacks here to identify the distro and the LANG, just provide me the test.
#
if test -f /etc/redhat-release; then 
	DISTRO=FedoraCore
elif test -f /etc/whatever; then
	DISTRO=Whatever
fi

case $DISTRO in
	FedoraCore)
		if grep -i "LANG" /etc/sysconfig/i18n | grep -i "UTF-8"; then
			UTF8_SYSTRAY="--enable-utf8-systray"
		fi
		;;
	Whatever)
		# Do whatever.
		;;
	*)
		# Unable to determine system locale, will not use UTF-8 systray.
		;;
esac
#
# ./configure
#
%configure \
        $UTF8_SYSTRAY \
        --enable-optimize \
        --disable-debug \
        --enable-cas \
	--enable-wxcas \
        --enable-amulecmd \
        --enable-webserver \
	--enable-ccache \
        --with-wx-config=`rpm -ql wxGTK-devel|grep 'wxgtk-2\.[0-9]-config'`
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%{_bindir}/amule
%{_bindir}/ed2k
%{_bindir}/amulecmd
%{_bindir}/cas
%{_bindir}/wxcas
%{_bindir}/amuleweb
%{_libdir}/xchat/plugins/xas.pl
%{_datadir}/applications/*
%{_datadir}/locale/*
%{_datadir}/pixmaps/*
%{_prefix}/man/*/*
%dir %{_datadir}/doc/%{name}-%{version}
%doc %{_datadir}/doc/%{name}-%{version}/*
%dir %{_datadir}/cas
%{_datadir}/cas/*
%dir %{_datadir}/amule
%dir %{_datadir}/amule/webserver
%{_datadir}/amule/webserver/*


%changelog
* Fri May 13 2005 Marcelo Jimenez <phoenix@amule.org>
- New spec for aMule-2.0.0 compliant with Fedora Core specifications.

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

