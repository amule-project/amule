if CAS
  OPT_CAS = cas
endif

if WXCAS
  OPT_WXCAS = wxCas
endif

if ALC
  OPT_ALC = aLinkCreator
endif

if ALCC
  OPT_ALC = aLinkCreator
endif

SUBDIRS = xas $(OPT_CAS) $(OPT_WXCAS) $(OPT_ALC)
DIST_SUBDIRS = fileview

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = mkFileSum.c
EXTRA__DIST__SUBDIRS = amps scripts
include $(top_srcdir)/automake/dist-hook.am

DISTCLEANFILES = scripts/denoiser.sed
