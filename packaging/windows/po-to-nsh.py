#!/usr/bin/env python3
"""
po-to-nsh.py -- generate installer_strings_generated.nsh

For every MUI_LANGUAGE registered by installer.nsi (excluding ENGLISH,
which is the hand-written baseline in installer.nsi itself), emit a
`LangString MYSTR_KEY ${LANG_NSISNAME} "..."` line for every key in
the KEYS table below.

The string value is:
  1. the translated msgstr from the matching po/<locale>.po, if one
     exists for that NSIS language and a non-empty translation is
     present for the key's English msgid, OR
  2. the English msgid itself, as the fallback.

This guarantees every registered language has a complete LangString
table -- previously, languages without a .po (or with a .po but no
translation for a given key) ended up with empty $(MYSTR_*) at runtime,
which clobbered the section names to empty strings when the
installer.nsi `.onInit` called SectionSetText.

The output file is `!include`d by installer.nsi (with /NONFATAL so the
installer also builds cleanly without running this bridge, e.g. when a
developer compiles the .nsi by hand and only the English baseline
applies).

Standard C escapes in the .po msgstr are translated to NSIS string-
literal escapes:
    \\r -> $\\r        (NSIS CR)
    \\n -> $\\n        (NSIS LF)
    \\t -> $\\t        (NSIS TAB)
    "  -> $\\"        (NSIS escaped double-quote)
    \\  -> $\\\\        (NSIS escape character)

NSIS runtime variables ($INSTDIR, $0, $APPDATA) are passed through
verbatim -- NSIS evaluates them at install time.

Usage:
    po-to-nsh.py <po-dir> <output-nsh-path> [<installer-nsi-path>]

If <installer-nsi-path> is omitted, the script looks for installer.nsi
in the same directory as the output file (the default packaging layout).
"""

import os
import re
import sys


# gettext po locale (filename without .po) -> NSIS Contrib/Language name
# (uppercased; what ${LANG_*} expands to). Only mappings where NSIS
# ships a matching .nlf are listed. When an aMule po catalog doesn't
# have a direct NSIS counterpart we silently skip it (no NSIS language
# means no place to register a LangString for); when an NSIS-registered
# language has no aMule po catalog, the bridge still emits an English
# fallback LangString for it via the iteration below.
LOCALE_TO_NSIS_LANG = {
    "ar":    "ARABIC",
    "ast":   "ASTURIAN",
    "bg":    "BULGARIAN",
    "ca":    "CATALAN",
    "cs":    "CZECH",
    "da":    "DANISH",
    "de":    "GERMAN",
    "el":    "GREEK",
    "es":    "SPANISH",
    "et_EE": "ESTONIAN",
    "eu":    "BASQUE",
    "fi":    "FINNISH",
    "fr":    "FRENCH",
    "gl":    "GALICIAN",
    "he":    "HEBREW",
    "hr":    "CROATIAN",
    "hu":    "HUNGARIAN",
    "it":    "ITALIAN",
    "it_CH": "ITALIAN",        # NSIS has no Swiss-Italian dialect
    "ja":    "JAPANESE",
    "ko_KR": "KOREAN",
    "lt":    "LITHUANIAN",
    "nl":    "DUTCH",
    "nn":    "NORWEGIANNYNORSK",
    "pl":    "POLISH",
    "pt_BR": "PORTUGUESEBR",
    "pt_PT": "PORTUGUESE",
    "ro":    "ROMANIAN",
    "ru":    "RUSSIAN",
    "sl":    "SLOVENIAN",
    "sq":    "ALBANIAN",
    "sv":    "SWEDISH",
    "tr":    "TURKISH",
    "uk":    "UKRAINIAN",
    "zh_CN": "SIMPCHINESE",
    "zh_TW": "TRADCHINESE",
}

# Reverse map: NSIS language name -> preferred po locale. When two po
# locales map to the same NSIS language (it / it_CH both -> ITALIAN),
# the earlier one wins -- so "it" beats "it_CH". Translations under
# the less-preferred locale are unused on the installer side.
NSIS_LANG_TO_LOCALE = {}
for _loc, _nsis in LOCALE_TO_NSIS_LANG.items():
    NSIS_LANG_TO_LOCALE.setdefault(_nsis, _loc)

# NSIS exposes two Spanish variants -- SPANISH (LCID 0x040a, "Spanish,
# Traditional Sort") and SPANISHINTERNATIONAL (LCID 0x0c0a, "Spanish,
# Modern Sort" / Latin America). Modern Windows and Wine both report
# es_ES as the Modern variant, so the installer's MUI auto-detect
# routes those users to SPANISHINTERNATIONAL. Without this alias the
# generated block falls back to English (issue #55). aMule ships a
# single es.po that serves both variants correctly.
NSIS_LANG_TO_LOCALE.setdefault("SPANISHINTERNATIONAL", "es")

# (NSIS LangString key, English msgid). Order matches the LangString
# block in installer.nsi and the array in installer_strings.c. Update
# all three together when adding or renaming an installer string. The
# English msgid IS the fallback value used for any language without a
# translation.
KEYS = [
    ("MYSTR_SEC_CORE",
        "aMule (required)"),
    ("MYSTR_SEC_DESKTOP",
        "Desktop shortcut"),
    ("MYSTR_SEC_AUTOSTART",
        "Start aMule when I log in"),
    ("MYSTR_SEC_UNINSTALL",
        "Uninstall"),
    ("MYSTR_SEC_REMOVE_USERDATA",
        "Remove user data (config, ED2K servers, Kad nodes, partfiles)"),
    ("MYSTR_DESC_CORE",
        "aMule application files (required)."),
    ("MYSTR_DESC_DESKTOP",
        "Place an aMule shortcut on the desktop."),
    ("MYSTR_DESC_AUTOSTART",
        "Launch aMule automatically when the current user logs in (per-user setting)."),
    ("MYSTR_DESC_UNINSTALL",
        "Remove aMule application files, Start Menu / desktop shortcuts, autostart Run-key entry, and Add/Remove Programs entry (required)."),
    ("MYSTR_DESC_REMOVE_USERDATA",
        "Permanently delete %APPDATA%\\aMule for the current user (aMule.conf, ED2K server list, Kad nodes, partfiles, IP filters, friends list). Leave unchecked to keep your settings."),
    ("MYSTR_MSG_AMULE_RUNNING",
        "aMule appears to be running from $INSTDIR.\r\nPlease close aMule (and aMuleD / aMuleGUI) and try again."),
    ("MYSTR_MSG_AMULED_RUNNING",
        "aMule daemon (amuled.exe) appears to be running from $INSTDIR.\r\nPlease stop it and try again."),
    ("MYSTR_MSG_REMOVING_PRIOR",
        "Removing previous aMule installation at $0..."),
    ("MYSTR_MSG_PRIOR_FAILED",
        "Could not remove the previous aMule installation at $0.\r\nPlease close any running aMule processes and try again, or uninstall the previous version manually first."),
    ("MYSTR_MSG_RUNNING_FOR_UNINST",
        "aMule appears to be running. Please close it and re-run the uninstaller."),
    ("MYSTR_MSG_REMOVING_USERDATA",
        "Removing user data at $APPDATA\\aMule..."),
]


_QUOTED = re.compile(r'"((?:\\.|[^\\"])*)"')
_BLOCK = re.compile(
    r'(?:^|\n)msgid\s+((?:"(?:\\.|[^\\"])*"\s*)+)'
    r'msgstr\s+((?:"(?:\\.|[^\\"])*"\s*)+)',
    re.MULTILINE,
)
_MUI_LANGUAGE = re.compile(
    r'^\s*!insertmacro\s+MUI_LANGUAGE\s+"([^"]+)"\s*$',
    re.MULTILINE,
)


def parse_po(path):
    """Return {msgid: msgstr} for a .po file, skipping the header and
    untranslated (empty msgstr) entries."""
    with open(path, encoding="utf-8") as f:
        text = f.read()
    entries = {}
    for m in _BLOCK.finditer(text):
        msgid = _join_quoted(m.group(1))
        msgstr = _join_quoted(m.group(2))
        if msgid and msgstr:
            entries[msgid] = msgstr
    return entries


def _join_quoted(raw):
    """Concatenate the body of consecutive "..."-quoted segments and
    unescape the standard C string escapes that .po uses (\\n, \\r,
    \\t, \\\\, \\", \\xNN, \\OOO)."""
    return "".join(_c_unescape(p) for p in _QUOTED.findall(raw))


def _c_unescape(s):
    out = []
    i = 0
    while i < len(s):
        c = s[i]
        if c != "\\" or i + 1 >= len(s):
            out.append(c)
            i += 1
            continue
        n = s[i + 1]
        if n == "n":
            out.append("\n"); i += 2
        elif n == "r":
            out.append("\r"); i += 2
        elif n == "t":
            out.append("\t"); i += 2
        elif n == "\\":
            out.append("\\"); i += 2
        elif n == '"':
            out.append('"'); i += 2
        else:
            # Unknown escape -- pass through both characters.
            out.append(c); out.append(n); i += 2
    return "".join(out)


def nsis_quote_body(s):
    """Convert a runtime string into the body of an NSIS string literal
    (i.e. what goes between the surrounding double-quotes in the .nsh
    output).

    NSIS's escape character is $ followed by a single special letter:
    $\\r / $\\n / $\\t / $\\" are CR / LF / TAB / double-quote, and $$ is
    a literal $. Backslashes are NOT escaped -- in an NSIS string,
    `\\` represents a literal backslash, not an escape introducer (the
    NSIS escape introducer is `$\\`, not `\\`). The earlier version of
    this function emitted `$\\\\` for `\\`, which NSIS lexed as an
    incomplete escape sequence and emitted a warning 6000 on every
    backslash-containing string.

    NSIS runtime variables ($INSTDIR, $0, $APPDATA, ...) and language-
    string refs ($(MYSTR_X)) are passed through verbatim; NSIS
    evaluates them at install time."""
    out = []
    for c in s:
        if c == "\r":
            out.append("$\\r")
        elif c == "\n":
            out.append("$\\n")
        elif c == "\t":
            out.append("$\\t")
        elif c == '"':
            out.append('$\\"')
        else:
            out.append(c)
    return "".join(out)


def registered_nsis_languages(installer_nsi_path):
    """Read installer.nsi and return the list of MUI_LANGUAGE names in
    declaration order (uppercased to match the ${LANG_*} constant names
    NSIS exposes). Order matters: the first registered language is what
    NSIS uses as the fallback for missing per-language LangString
    entries on the MUI built-in pages."""
    with open(installer_nsi_path, encoding="utf-8") as f:
        text = f.read()
    return [m.upper() for m in _MUI_LANGUAGE.findall(text)]


def main(argv):
    if len(argv) not in (3, 4):
        print("usage: po-to-nsh.py <po-dir> <output-nsh-path> [<installer-nsi-path>]",
              file=sys.stderr)
        return 2
    po_dir, out_path = argv[1], argv[2]
    installer_nsi = argv[3] if len(argv) == 4 else os.path.join(
        os.path.dirname(os.path.abspath(out_path)), "installer.nsi")

    nsis_langs = registered_nsis_languages(installer_nsi)
    if not nsis_langs:
        print(f"fatal: no !insertmacro MUI_LANGUAGE entries found in "
              f"{installer_nsi}", file=sys.stderr)
        return 1

    # Pre-load each po catalog we might need exactly once.
    po_cache = {}
    for locale in set(NSIS_LANG_TO_LOCALE.values()):
        po_path = os.path.join(po_dir, f"{locale}.po")
        if os.path.isfile(po_path):
            po_cache[locale] = parse_po(po_path)

    with open(out_path, "w", encoding="utf-8") as out:
        out.write("; Auto-generated by packaging/windows/po-to-nsh.py.\n")
        out.write("; Source LangString keys: packaging/windows/installer_strings.c.\n")
        out.write("; Per-language values: po/<lang>.po when available, otherwise the\n")
        out.write("; English msgid as a fallback so every registered NSIS language has\n")
        out.write("; a complete LangString table (and no runtime $(MYSTR_*) ever\n")
        out.write("; resolves to an empty string -- which would clobber section names\n")
        out.write("; via SectionSetText).\n")
        out.write("; Do not edit by hand -- changes will be overwritten on the next\n")
        out.write("; installer build.\n\n")

        for nsis_lang in nsis_langs:
            if nsis_lang == "ENGLISH":
                # The hand-written ${LANG_ENGLISH} block in installer.nsi
                # is the authoritative source for English; skip emitting
                # a duplicate (NSIS treats a re-declared LangString as
                # an override warning).
                continue

            locale = NSIS_LANG_TO_LOCALE.get(nsis_lang)
            entries = po_cache.get(locale, {}) if locale else {}

            if locale and entries:
                origin = f"from po/{locale}.po, English fallback for untranslated keys"
            elif locale:
                origin = f"po/{locale}.po has no usable translations; English fallback"
            else:
                origin = "no matching aMule po catalog; English fallback"
            out.write(f"; --- ${{LANG_{nsis_lang}}} ({origin}) ---\n")

            for key, msgid in KEYS:
                value = entries.get(msgid) or msgid
                literal = nsis_quote_body(value)
                out.write(f'LangString {key:<32} ${{LANG_{nsis_lang}}} "{literal}"\n')
            out.write("\n")

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
