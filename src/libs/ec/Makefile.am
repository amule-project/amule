SUBDIRS = cpp

# Sources

MAINTAINERCLEANFILES = Makefile.in

# The normal order of processing is to build subdirectories
# first, but we need the files being generated *before* make
# descends into subdirectories. Using BUILT_SOURCES will
# ensure the correct behaviour.

BUILT_SOURCES = ECGeneratedFiles

ECGeneratedFilesSources = \
	abstracts/ECTagTypes.abstract \
	abstracts/ECCodes.abstract

ECGeneratedFiles: $(ECGeneratedFilesSources)
	$(srcdir)/file_generator.pl $(srcdir) $(ECGeneratedFilesSources);
	echo $(ECGeneratedFilesSources) > ECGeneratedFiles


EXTRA_DIST = file_generator.pl
EXTRA__DIST__SUBDIRS = abstracts java
include $(top_srcdir)/automake/dist-hook.am

CLEANFILES = $(BUILT_SOURCES)
