//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2026 aMule Team ( https://amule-org.github.io )
//
// Any parts of this program contributed by third-party developers are
// copyrighted by their respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef LOCALEINIT_H
#define LOCALEINIT_H

// Initialise the libc locale for an aMule binary.
//
// Picks `LC_CTYPE` (and related) up from the environment via
// `setlocale(LC_ALL, "")` so the multibyte conversions used by wxConvLibc
// (e.g. `unicode2char()` in StringFunctions.cpp) emit real UTF-8 instead
// of mangling non-ASCII bytes the way they do under the default `C`
// locale. Then forces `LC_NUMERIC` back to `C` so libc printf/scanf
// decimal handling stays portable across locales (e.g. de_DE.UTF-8 would
// otherwise turn `"%.2f"` of 3.14 into `"3,14"` and break any text-based
// numeric serialisation that relies on libc).
//
// Call this once, as early as possible, from each binary's earliest
// initialisation hook. Safe to call multiple times. Has no effect on
// binaries that already drive their locale through `wxLocale`
// (`amule` / `amule-remote-gui`); the inner setlocale calls are no-ops
// when the resulting state matches.
//
// See amule-org/amule#203 for the original reported bug.
void aMuleInitLocale();

#endif // LOCALEINIT_H
