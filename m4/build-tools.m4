# This file is part of the aMule project.                      -*- Autoconf -*-
#
# This package has been tested with GNU Autoconf 2.59, and not guaranteed to
# work with any other version. However, you only need to worry about it if
# you're cross-compiling.

# _AC_CHECK_BUILD_PREFIX
# -----------------------
# Checks and sets the build prefix, if it is given by --build.
AC_DEFUN([_AC_CHECK_BUILD_PREFIX],
[ac_build_prefix=
test -n "$build_alias" && ac_build_prefix=$build_alias-
]) # AC_CHECK_BUILD_PREFIX


# AC_PROG_BUILD_CC([COMPILER ...])
# --------------------------
# COMPILER ... is a space separated list of C compilers to search for.
# This just gives the user an opportunity to specify an alternative
# search list for the C compiler.
#
# IMPORTANT: Run all other compiler tests *before* calling this macro!
#
# This is a stripped-down check, it checks only what we need,
# i.e. BUILD_CC and BUILD_EXEEXT
#
AC_DEFUN([AC_PROG_BUILD_CC],
[AC_REQUIRE([_AC_CHECK_BUILD_PREFIX])dnl
dnl Set new names of important variables.
pushdef([ac_tool_prefix], [ac_build_prefix])dnl
pushdef([CC], [BUILD_CC])dnl
pushdef([EXEEXT], [BUILD_EXEEXT])dnl
pushdef([ac_cv_exeext], [ac_cv_build_exeext])dnl
pushdef([ac_exeext], [ac_build_exeext])dnl

# Even if we're cross-compiling, we want a compiler here
# that is not a cross-compiler.
saved_cross=$cross_compiling
cross_compiling=no

dnl From now on, this is just a mere copy of Autoconf's AC_PROG_CC macro.
AC_LANG_PUSH(C)dnl
m4_ifval([$1],
      [AC_CHECK_TOOLS(CC, [$1])],
[AC_CHECK_TOOL(CC, gcc)
if test -z "$CC"; then
  AC_CHECK_TOOL(CC, cc)
fi
if test -z "$CC"; then
  AC_CHECK_PROG(CC, cc, cc, , , /usr/ucb/cc)
fi
if test -z "$CC"; then
  AC_CHECK_TOOLS(CC, cl)
fi
])

test -z "$CC" && AC_MSG_FAILURE([no acceptable C compiler found in \$PATH])

# Provide some information about the compiler.
echo "$as_me:$LINENO:" \
     "checking for _AC_LANG compiler version" >&AS_MESSAGE_LOG_FD
ac_compiler=`set X $ac_compile; echo $[2]`
_AC_EVAL([$ac_compiler --version </dev/null >&AS_MESSAGE_LOG_FD])
_AC_EVAL([$ac_compiler -v </dev/null >&AS_MESSAGE_LOG_FD])
_AC_EVAL([$ac_compiler -V </dev/null >&AS_MESSAGE_LOG_FD])

dnl Forcibly include _AC_COMPILER_EXEEXT, to determine the build exeext.
_AC_COMPILER_EXEEXT
dnl End of copy, here some parts are stripped out. We only want a working C
dnl compiler, and doesn't need the objext, the preprocessor, the dependency
dnl style, whether it's gnu or not, etc.

# Restore configuration environment
cross_compiling=$saved_cross

dnl Restore variable names.
popdef([ac_exeext])dnl
popdef([ac_cv_exeext])dnl
popdef([EXEEXT])dnl
popdef([CC])dnl
popdef([ac_tool_prefix])dnl
dnl
dnl AC_LANG_POP(C) must be called after the variable names are restored, thus
dnl it will restore the restore the correct (host) environment, not the build
dnl environment.
AC_LANG_POP(C)dnl
AC_SUBST(BUILD_EXEEXT)dnl
]) # AC_PROG_BUILD_CC
