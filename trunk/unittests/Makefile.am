SUBDIRS = 
MAINTAINERCLEANFILES = Makefile.in
DIST_SUBDIRS = muleunit tests

# The only targets which we care about
TARGETS = check-recursive clean-recursive
$(TARGETS): SUBDIRS = $(DIST_SUBDIRS)
