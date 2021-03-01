
#
# Copyright © 2016 Tommaso Gragnato <gragnato.tommaso@icloud.com>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

WXDIR := $(SRCROOT)/wxWidgets
WXREPO := https://github.com/wxWidgets/wxWidgets.git
WXBRANCH := WX_3_0_BRANCH

SRCDIR := $(WXDIR)/src
BUILDDIR := $(WXDIR)/build
DESTDIR := $(WXDIR)/dest

STORE_ARCH := $(shell echo $(ARCHS) | sed 's/ /,/g' )

$(WXDIR)/.updated:
	git clone $(WXREPO) $(SRCDIR) --depth 1 --branch $(WXBRANCH)
	touch $@

$(WXDIR)/.configured: $(WXDIR)/.updated
	if ! [ -d $(BUILDDIR) ]; then mkdir $(BUILDDIR); fi
	cd $(BUILDDIR) && $(SRCDIR)/configure \
		--prefix=$(DESTDIR) \
		--disable-debug \
		--enable-unicode \
		--enable-std_string \
		--enable-display \
		--with-opengl \
		--with-regex \
		--with-osx_cocoa \
		--without-libjpeg \
		--without-libtiff \
		--without-liblzma \
		--without-libpng \
		--with-zlib \
		--enable-dnd \
		--enable-clipboard \
		--enable-webkit \
		--disable-svg \
		--enable-graphics_ctx \
		--enable-controls \
		--enable-dataviewctrl \
		--with-expat \
		--disable-precomp-headers \
		--disable-monolithic \
		--enable-static \
		--disable-shared \
		--disable-mediactrl \
		--with-macosx-version-min=$(MACOSX_DEPLOYMENT_TARGET) \
		--enable-universal_binary=$(STORE_ARCH)
	touch $@

$(WXDIR)/.installed: $(WXDIR)/.configured
	$(MAKE) -C $(BUILDDIR) install
	touch $@

install: $(WXDIR)/.installed

clean:
	$(RM) -rf $(DESTDIR)
	if [ -d $(BUILDDIR) ]; then $(MAKE) -C $(BUILDDIR) clean; fi

.PHONY: install
.DEFAULT_GOAL := install
