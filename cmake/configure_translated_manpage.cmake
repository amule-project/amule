# Build-time variable substitution for a single translated manpage.
#
# po4a renders translated manpages into the build dir verbatim from the English
# masters, which means @MAN_DATE@ and @PACKAGE_VERSION@ come through untouched
# (po4a treats them as ordinary literal text). This script does the deferred
# configure_file pass on a single po4a output, run from add_custom_command so
# the input exists by build time. configure_file itself only runs at
# configure time, so we re-implement its @ONLY semantics via file(READ) +
# string(CONFIGURE) + file(WRITE).
#
# Caller passes -DIN, -DOUT, -DMAN_DATE and -DPACKAGE_VERSION.

if (NOT IN OR NOT OUT)
	message (FATAL_ERROR "configure_translated_manpage.cmake: IN and OUT must be set.")
endif()

file (READ "${IN}" _content)
string (CONFIGURE "${_content}" _content @ONLY)
file (WRITE "${OUT}" "${_content}")
