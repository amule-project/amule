
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

PUPNPDIR := $(SRCROOT)/pUPnP
PUPNPREPO := https://github.com/mrjimenez/pupnp.git
PUPNPBRANCH := branch-1.6.x

SRCDIR := $(PUPNPDIR)/src
BUILDDIR := $(PUPNPDIR)/build
DESTDIR := $(PUPNPDIR)/dest

DEPLOY_CONF := -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
TMP_ARCH := $(shell echo $(ARCHS) | sed 's/ / -arch /g' )
CFLAGS := -DNDEBUG -g2 -Os -fPIC -pipe -arch

$(PUPNPDIR)/.updated:
	git clone $(PUPNPREPO) $(SRCDIR) --depth 1 --branch $(PUPNPBRANCH)
	touch $@

$(PUPNPDIR)/.configured: $(PUPNPDIR)/.updated
	if ! [ -d $(BUILDDIR) ]; then mkdir $(BUILDDIR); fi
	autoreconf --install $(SRCDIR)
	cd $(BUILDDIR) && CFLAGS="$(CFLAGS) $(TMP_ARCH) $(DEPLOY_CONF)" \
		$(SRCDIR)/configure \
		--prefix=$(DESTDIR) \
		--disable-debug \
		--enable-static \
		--disable-shared \
		--enable-ipv6
	touch $@

$(PUPNPDIR)/.installed: $(PUPNPDIR)/.configured
	$(MAKE) -C $(BUILDDIR) install
	touch $@

install: $(PUPNPDIR)/.installed

clean:
	$(RM) -rf $(DESTDIR)
	if [ -d $(BUILDDIR) ]; then $(MAKE) -C $(BUILDDIR) clean; fi

.PHONY: install
.DEFAULT_GOAL := install
