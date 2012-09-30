#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([tsutils], [20120925_012059], [tim@tim-landscheidt.de])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([gnu color-tests silent-rules -Wall])
AC_CONFIG_SRCDIR([readline/readline.c])
AC_CONFIG_HEADERS([config.h])

# Set OS.
AC_SUBST([TARGET_OS], [$target_os])

# Set conditionals.
AM_CONDITIONAL([TSUTILS_LINUX],   [test x$target_os = xlinux-gnu])
AM_CONDITIONAL([TSUTILS_FREEBSD], [test x$target_os = xfreebsd])
AM_CONDITIONAL([TSUTILS_SUNOS],   [test x$target_os = xsolaris2.10])

# Checks for programs.
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_RANLIB

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lldap':
AC_CHECK_LIB([ldap], [main])
# FIXME: Replace `main' with a function in `-lreadline':
AC_CHECK_LIB([readline], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h shadow.h stdint.h stdlib.h string.h strings.h sys/time.h syslog.h termios.h unistd.h utmp.h utmpx.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT64_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero dup2 gethostname getpass getspnam isascii memset mkdir munmap strchr strdup strerror strstr strtol])

# Require MySQL libraries.
AX_LIB_MYSQL

AC_CONFIG_FILES([Makefile
                 acctexp/Makefile
                 acctrenew/Makefile
                 date2days/Makefile
                 days2date/Makefile
                 getpasstofile/Makefile
                 libtsutils/Makefile
                 listlogins/Makefile
                 listusers/Makefile
                 lsexp/Makefile
                 passwd/Makefile
                 readline/Makefile
                 resetpass/Makefile
                 setlicense/Makefile
                 setmail/Makefile
                 setpass/Makefile
                 summdisk/Makefile
                 watcherd/Makefile
                 whinequota/Makefile
                 whodo/Makefile])
AC_OUTPUT
