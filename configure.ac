AC_INIT([Saltarello], [1.0], [sepen@crux-arm.nu])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_PROG_CXX
AC_LANG([C++])

PKG_CHECK_MODULES([SDL2], [sdl2])
PKG_CHECK_MODULES([SDL2_TTF], [SDL2_ttf])
PKG_CHECK_MODULES([SDL2_MIXER], [SDL2_mixer])

# AddressSanitizer is opt-in: it is not available on every toolchain and,
# on macOS, it breaks Homebrew's sdl2-compat at load time.
AC_ARG_ENABLE([asan],
  [AS_HELP_STRING([--enable-asan], [build with AddressSanitizer (debug only)])],
  [enable_asan=$enableval], [enable_asan=no])
AM_CONDITIONAL([ENABLE_ASAN], [test "x$enable_asan" = "xyes"])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
