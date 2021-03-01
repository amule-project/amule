
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

BOOSTSRC := $(SRCROOT)/Boost
BOOSTREPO := https://github.com/boostorg

LIST = array asio assert bind config core date_time mpl preprocessor predef
LIST += regex smart_ptr static_assert system throw_exception type_traits utility

$(LIST):
	git clone $(BOOSTREPO)/$@.git $(BOOSTSRC)/$@ --depth 1
	touch $(BOOSTSRC)/$@

$(BOOSTSRC)/.updated: $(LIST)
	touch $@

install: $(BOOSTSRC)/.updated

clean:
	echo 'no clean target defined'

.PHONY: install
.DEFAULT_GOAL := install
