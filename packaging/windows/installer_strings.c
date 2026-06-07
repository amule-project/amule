/*
 * Windows installer translatable strings -- visible to xgettext only.
 *
 * This file's sole purpose is to make the Windows NSIS installer's
 * translatable strings show up in po/amule.pot via the existing
 * xgettext sweep, so translators can pick them up through their
 * normal po/<lang>.po editing workflow next to the app strings.
 *
 * The strings here mirror the LangString MYSTR_* declarations in
 * packaging/windows/installer.nsi. At installer build time the
 * per-language LangString blocks are regenerated from the matching
 * .po entries by packaging/windows/po-to-nsh.py (see installer.nsi
 * for the include).
 *
 * Notes for translators reading this in po/<lang>.po:
 *
 *   - $INSTDIR, $0, $APPDATA, $\r, $\n in the source strings are NSIS
 *     runtime escapes / variables. Keep them verbatim in your
 *     translation -- NSIS evaluates them at install time, and the
 *     po-to-nsh bridge passes them through.
 *
 *   - %APPDATA% (with percent signs) in the "Permanently delete..."
 *     string is the literal Windows environment-variable reference
 *     shown to the user, not a printf placeholder. Keep it as text.
 *
 * This file is not compiled into anything; the local _() macro below
 * makes the array syntactically valid C so editors with C language
 * servers don't flag it, but the file is never handed to a real
 * compiler -- it only ever passes through xgettext's lexer.
 */

#define _(s) (s)

static const char *const installer_strings[] = {
	/* Section names (Components page). */
	_("aMule (required)"),
	_("Desktop shortcut"),
	_("Start aMule when I log in"),
	_("Uninstall"),
	_("Remove user data (config, ED2K servers, Kad nodes, partfiles)"),

	/* Section descriptions (Components page hover text). */
	_("aMule application files (required)."),
	_("Place an aMule shortcut on the desktop."),
	_("Launch aMule automatically when the current user logs in (per-user setting)."),
	_("Remove aMule application files, Start Menu / desktop shortcuts, autostart Run-key entry, and Add/Remove Programs entry (required)."),
	_("Permanently delete %APPDATA%\\aMule for the current user (aMule.conf, ED2K server list, Kad nodes, partfiles, IP filters, friends list). Leave unchecked to keep your settings."),

	/* Runtime messages (MessageBox / DetailPrint).
	 * \r\n here is the standard C escape for CR LF; the po-to-nsh
	 * bridge converts each occurrence into NSIS's $\r$\n line-break
	 * escape when emitting the LangString. */
	_("aMule appears to be running from $INSTDIR.\r\nPlease close aMule (and aMuleD / aMuleGUI) and try again."),
	_("aMule daemon (amuled.exe) appears to be running from $INSTDIR.\r\nPlease stop it and try again."),
	_("Removing previous aMule installation at $0..."),
	_("Could not remove the previous aMule installation at $0.\r\nPlease close any running aMule processes and try again, or uninstall the previous version manually first."),
	_("aMule appears to be running. Please close it and re-run the uninstaller."),
	_("Removing user data at $APPDATA\\aMule..."),
};
/* File_checked_for_headers */
