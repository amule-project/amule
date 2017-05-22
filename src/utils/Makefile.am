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

if XAS
  OPT_XAS = xas
endif

if PLASMAMULE
  OPT_PLASMAMULE = plasmamule
endif

if FILEVIEW
  OPT_FILEVIEW = fileview
endif

SUBDIRS = $(OPT_FILEVIEW) $(OPT_XAS) $(OPT_CAS) $(OPT_WXCAS) $(OPT_ALC) ${OPT_PLASMAMULE}
DIST_SUBDIRS = fileview xas cas wxCas aLinkCreator plasmamule

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = mkFileSum.c
EXTRA__DIST__SUBDIRS = amps scripts
include $(top_srcdir)/automake/dist-hook.am

DISTCLEANFILES = scripts/denoiser.sed
