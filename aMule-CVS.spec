%define cvsdate %(date +%Y%m%d)

Summary:     aMule - another eMule p2p client
Name:        aMule
Version:     CVS
Release:     %{cvsdate}
License:     GPL
Group:       Applications/Internet
Packager:    The aMule Team (http://forum.amule.org/)
Vendor:      The aMule Project
URL:         http://www.amule.org
Source:      aMule-%{version}-%{release}.tar.bz2
Prefix:      %{_prefix}
BuildRoot:   %{_builddir}/%{name}-%{version}-root
Requires:    wxGTK >= 2.4.2, wxBase >= 2.4.2, krb5-libs >= 1.3.1, curl >= 7.10.6
BuildPreReq: wxGTK-devel >= 2.4.2, krb5-devel >= 1.3.1, grep, curl-devel >= 7.10.6, automake >= 1.7
Provides:    %{name}
Obsoletes:   %{name}

AutoReq:     0

%description
aMule is a peer to peer file sharing client, based on the well known eMule.
Starting with 2.0.0 aMule is supposed to work on Linux, and *BSD,
which makes it the first multi-platform edonkey network client. A Windows port
is planned soon :)

%pre
echo "***"
echo "This is a cvs release!"
echo "This release is made for testing purpose and it may cause several problems."
echo "If you like to test some of the great new features go on and install."
echo "Otherwise you may press ctrl-c within the next 10 seconds to abort the installation."
echo -n "Waiting for user"
for l in $(seq 1 10); do
    echo -n "."
    sleep 1
done
echo " looks good, installing."

%prep
%setup -q -n amule-cvs
CFLAGS="$RPM_OPT_FLAGS" ./configure \
        --prefix=%{_prefix} \
        --disable-optimize \
        --enable-debug \
        --enable-cas \
	--enable-wxcas \
        --enable-amulecmd \
        --enable-webserver \
	--enable-ccache \
        --with-wx-config=`rpm -ql wxGTK-devel|grep 'wxgtk-2\.[0-9]-config'` \
        --with-wxbase-config=`rpm -ql wxBase|grep 'wxbase-2\.[0-9]-config'`

%build
%{__make}

%install
[ ! "$RPM_BUILD_ROOT" = "/" ] && rm -rf $RPM_BUILD_ROOT
%{__make} prefix=$RPM_BUILD_ROOT%{_prefix} \
        exec_prefix=$RPM_BUILD_ROOT%{_prefix} \
        install

%clean
[ ! "$RPM_BUILD_ROOT" = "/" ] && %{__rm} -rf $RPM_BUILD_ROOT
%{__rm} -rf %{_builddir}/%{name}-%{version}

%files
%defattr(-,root,root)
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

