#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gautierrss], [8.0.0], [mklinfinity@proton.me])
AM_INIT_AUTOMAKE([gnits] [subdir-objects] [-Wall -Werror])
AM_PROG_AR
LT_PREREQ([2.4])
LT_INIT([dlopen])
AC_LANG([C])
AC_LANG([C++])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([src/rss_ui/impl/application.cpp])
AC_CONFIG_HEADERS([config.h])
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UID_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit mkdir strerror])
AC_CHECK_LIB(gio-2.0,g_application_run)
AC_CHECK_LIB(gobject-2.0,g_signal_connect_data)
AC_CHECK_LIB(glib-2.0,g_main_context_invoke)
AC_CHECK_LIB(gdk-3,gdk_window_get_scale_factor)
AC_CHECK_LIB(gdk_pixbuf-2.0,gdk_pixbuf_new_from_resource)
AC_CHECK_LIB(gtk-3,gtk_bin_get_child)
AC_CHECK_LIB(sqlite3,sqlite3_finalize)
AC_CHECK_LIB(curl,curl_easy_init)
AC_CHECK_LIB(pthread,pthread_create)
AC_CHECK_LIB(webkit2gtk-4.0,webkit_web_view_set_settings)
AC_CHECK_LIB(xml2,xmlCleanupParser)

#Custom Functionality
ACFLAGS="`pkg-config gio-2.0 --cflags`"
ACFLAGS="${ACFLAGS} `pkg-config glib-2.0 --cflags`"
ACFLAGS="${ACFLAGS} `pkg-config gdk-3.0 --cflags`"
ACFLAGS="${ACFLAGS} `pkg-config gdk-pixbuf-2.0 --cflags`"
ACFLAGS="${ACFLAGS} `pkg-config gtk+-3.0 --cflags`"
ACFLAGS="${ACFLAGS} `pkg-config sqlite3 --cflags`"
ACFLAGS="${ACFLAGS} `pkg-config libcurl --cflags`"
ACFLAGS="${ACFLAGS} `pkg-config webkit2gtk-4.0 --cflags`"
ACFLAGS="${ACFLAGS} `pkg-config libxml-2.0 --cflags`"

CFLAGS="${CFLAGS} ${ACFLAGS}"
CPPFLAGS="${CPPFLAGS} ${ACFLAGS}"

GTKRESCFILE="${srcdir}/src/rss_ui/app_resources.xml"
GTKRESCFILE_CSS="${srcdir}/src/rss_ui/app_style.css"
GTKRESCFILE_C="${srcdir}/src/rss_ui/app_resources.c"
GTKRESCFILE_H="${srcdir}/src/rss_ui/app_resources.h"

if [ ! test -f "$GTKRESCFILE_H" ] || [ ! test -f "$GTKRESCFILE_C" ]
then
	glib-compile-resources --generate-source --sourcedir=${srcdir}/src/rss_ui/ "${GTKRESCFILE}"
	glib-compile-resources --generate-header --sourcedir=${srcdir}/src/rss_ui/ "${GTKRESCFILE}"
	glib-compile-resources --generate-dependencies --sourcedir=${srcdir}/src/rss_ui/ "${GTKRESCFILE}"
fi

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
