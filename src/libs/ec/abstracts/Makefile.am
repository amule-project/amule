DEFS = 

# Sources

MAINTAINERCLEANFILES = Makefile.in

BUILT_SOURCES = ECGeneratedFiles

ECGeneratedFilesSources = \
	ECTagTypes.abstract \
	ECCodes.abstract

ECGeneratedFiles: $(ECGeneratedFilesSources)
	$(srcdir)/file_generator.pl $(srcdir) $(ECGeneratedFilesSources);

