SUBDIRS = cpp

# Sources

MAINTAINERCLEANFILES = Makefile.in

BUILT_SOURCES = ECGeneratedFiles

ECGeneratedFilesSources = \
	abstracts/ECTagTypes.abstract \
	abstracts/ECCodes.abstract

ECGeneratedFiles: $(ECGeneratedFilesSources)
	$(srcdir)/file_generator.pl $(srcdir) $(ECGeneratedFilesSources);
	echo $(ECGeneratedFilesSources) > ECGeneratedFiles


EXTRA_DIST = file_generator.pl
EXTRA__DIST__SUBDIRS = abstracts java		# c# cannot be included because of the # in the name
include $(top_srcdir)/automake/dist-hook.am

DISTCLEANFILES = $(BUILT_SOURCES)
