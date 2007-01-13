SUBDIRS = cpp
DEFS = 

# Sources

MAINTAINERCLEANFILES = Makefile.in

BUILT_SOURCES = ECGeneratedFiles

ECGeneratedFilesSources = \
	abstracts/ECTagTypes.abstract \
	abstracts/ECCodes.abstract

ECGeneratedFiles: $(ECGeneratedFilesSources)
	$(srcdir)/file_generator.pl $(srcdir) $(ECGeneratedFilesSources);
	echo $(ECGeneratedFilesSources) > ECGeneratedFiles

