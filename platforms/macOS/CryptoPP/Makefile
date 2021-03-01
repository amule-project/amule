
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

CRYPTODIR := $(SRCROOT)/CryptoPP
CRYPTOREPO := https://github.com/weidai11/cryptopp.git
CRYPTOBRANCH := master

SRCDIR := $(CRYPTODIR)/src
DESTDIR := $(CRYPTODIR)/dest

DEPLOY_CONF := -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
TMP_ARCH := $(shell echo $(ARCHS) | sed 's/ / -arch /g' )
CXXFLAGS := -DNDEBUG -g2 -Os -fPIC -pipe -arch

$(CRYPTODIR)/.updated:
	git clone $(CRYPTOREPO) $(SRCDIR) --depth 1 --branch $(CRYPTOBRANCH)
	touch $@

$(CRYPTODIR)/.installed: $(CRYPTODIR)/.updated
	CXXFLAGS="$(CXXFLAGS) $(TMP_ARCH) $(DEPLOY_CONF)" \
		PREFIX=$(DESTDIR) \
		$(MAKE) -C $(SRCDIR) -f GNUmakefile-cross static all
	CXXFLAGS="$(CXXFLAGS) $(TMP_ARCH) $(DEPLOY_CONF)" \
		PREFIX=$(DESTDIR) \
		$(MAKE) -C $(SRCDIR) -f GNUmakefile-cross install
	touch $@

install: $(CRYPTODIR)/.installed

clean:
	$(RM) -rf $(DESTDIR)
	if [ -d $(SRCDIR) ]; then $(MAKE) -C $(SRCDIR) clean; fi

.PHONY: install
.DEFAULT_GOAL := install
