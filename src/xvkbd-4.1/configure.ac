#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xvkbd], [4.1], [VEF00200@nifty.com])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([resources.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_MAKE_SET

# Checks for libraries.
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([XAW], [xaw3d],,[
  PKG_CHECK_MODULES([XAW], [xaw7])
])
PKG_CHECK_MODULES([X11], [xmu xtst])

AC_CHECK_LIB([X11], [XCreateWindow])
#AC_CHECK_LIB([Xaw3d], [XawAsciiSave])
AC_CHECK_LIB([Xt], [XtAppMainLoop])
AC_CHECK_LIB([Xtst], [XTestFakeButtonEvent])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([limits.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([isascii putenv setlocale strcasecmp strchr strerror strncasecmp strstr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT


