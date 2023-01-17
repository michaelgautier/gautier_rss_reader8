ACLOCAL_AMFLAGS = -I m4

srcdir = @srcdir@
VPATH = @srcdir@

#############
#############  PROGRAMS //////////////////////////////////////////
#############

bin_PROGRAMS = newsdownld rss_io_test gautierrss

iconH1dir = $(datadir)/icons/HighContrast/16x16/apps/
dist_iconH1_DATA = desktop/icons/newsreader.png

iconH2dir = $(datadir)/icons/HighContrast/16x16/apps/
dist_iconH2_DATA = desktop/icons/newsreader.png

iconH3dir = $(datadir)/icons/HighContrast/22x22/apps/
dist_iconH3_DATA = desktop/icons/newsreader.png

iconH4dir = $(datadir)/icons/HighContrast/24x24/apps/
dist_iconH4_DATA = desktop/icons/newsreader.png

iconH5dir = $(datadir)/icons/HighContrast/256x256/apps/
dist_iconH5_DATA = desktop/icons/newsreader.png

iconH6dir = $(datadir)/icons/HighContrast/32x32/apps/
dist_iconH6_DATA = desktop/icons/newsreader.png

iconH7dir = $(datadir)/icons/HighContrast/48x48/apps/
dist_iconH7_DATA = desktop/icons/newsreader.png

iconC1dir = $(datadir)/icons/hicolor/128x128/apps/
dist_iconC1_DATA = desktop/icons/newsreader.png

iconC2dir = $(datadir)/icons/hicolor/16x16/apps/
dist_iconC2_DATA = desktop/icons/newsreader.png

iconC3dir = $(datadir)/icons/hicolor/32x32/apps/
dist_iconC3_DATA = desktop/icons/newsreader.png

iconC4dir = $(datadir)/icons/hicolor/48x48/apps/
dist_iconC4_DATA = desktop/icons/newsreader.png

iconC5dir = $(datadir)/icons/hicolor/64x64/apps/
dist_iconC5_DATA = desktop/icons/newsreader.png

pixmapdir = $(datadir)/pixmaps/newsreader.xpm
dist_pixmap_DATA = desktop/pixmaps/newsreader.xpm

appdatadir = ${datadir}/desktop/
dist_appdata_DATA = desktop/newsreader.appdata.xml

#############
#############  DEPENDENT OBJECTS //////////////////////////////////////////
#############
#	FOUNDATION
src/external/argtable/argtable3.c: Makefile
src/rss_lib/rss/impl/rss_article.cpp: Makefile
src/rss_lib/rss/impl/rss_feed.cpp: Makefile
src/rss_lib/rss/impl/rss_reader.cpp: Makefile
src/rss_lib/rss/impl/rss_util.cpp: Makefile
src/rss_lib/rss/impl/rss_writer.cpp: Makefile
src/rss_lib/db/impl/db.cpp: Makefile
src/rss_lib/rss_download/impl/feed_download.cpp: Makefile
src/rss_lib/rss_parse/impl/feed_parse.cpp: Makefile

#	USER INTERFACE
src/rss_ui/impl/application.cpp: Makefile
src/rss_ui/rss_manage/impl/rss_manage.cpp: Makefile
src/rss_ui/app_win/impl/app_win.cpp: Makefile
src/rss_ui/app_win/impl/article_frame.cpp: Makefile
src/rss_ui/app_win/impl/article_header.cpp: Makefile
src/rss_ui/app_win/impl/headlines_frame.cpp: Makefile

#############
#############  News Downloader //////////////////////////////////////////
#############

newsdownld_SOURCES = src/rss_cli/newsdownld.cpp \
 src/rss_lib/rss/impl/rss_article.cpp \
 src/rss_lib/rss/impl/rss_feed.cpp \
 src/rss_lib/rss/impl/rss_reader.cpp \
 src/rss_lib/rss/impl/rss_util.cpp \
 src/rss_lib/rss/impl/rss_writer.cpp \
 src/rss_lib/db/impl/db.cpp \
 src/rss_lib/rss_download/impl/feed_download.cpp \
 src/rss_lib/rss_parse/impl/feed_parse.cpp \
 src/external/argtable/argtable3.c

newsdownld_CPPFLAGS = -I$(top_srcdir)/src

#############
#############  RSS Test //////////////////////////////////////////
#############

rss_io_test_SOURCES = src/rss_cli/rss_io_test.cpp \
 src/rss_lib/rss/impl/rss_article.cpp \
 src/rss_lib/rss/impl/rss_feed.cpp \
 src/rss_lib/rss/impl/rss_reader.cpp \
 src/rss_lib/rss/impl/rss_util.cpp \
 src/rss_lib/rss/impl/rss_writer.cpp \
 src/rss_lib/db/impl/db.cpp \
 src/rss_lib/rss_download/impl/feed_download.cpp \
 src/rss_lib/rss_parse/impl/feed_parse.cpp \
 src/external/argtable/argtable3.c

rss_io_test_CPPFLAGS = -I$(top_srcdir)/src

#############
#############  Gautier RSS //////////////////////////////////////////
#############

gautierrss_SOURCES = src/rss_ui/impl/application.cpp \
 src/rss_ui/app_resources.c \
 src/rss_ui/app_resources.h \
 src/rss_ui/rss_manage/rss_manage.hpp \
 src/rss_ui/rss_manage/impl/rss_manage.cpp \
 src/rss_ui/app_win/impl/app_win.cpp \
 src/rss_ui/app_win/app_win.hpp \
 src/rss_ui/app_win/impl/article_frame.cpp \
 src/rss_ui/app_win/article_frame.hpp \
 src/rss_ui/app_win/impl/article_header.cpp \
 src/rss_ui/app_win/article_header.hpp \
 src/rss_ui/app_win/impl/headlines_frame.cpp \
 src/rss_lib/rss/impl/rss_article.cpp \
 src/rss_lib/rss/impl/rss_feed.cpp \
 src/rss_lib/rss/impl/rss_reader.cpp \
 src/rss_lib/rss/impl/rss_util.cpp \
 src/rss_lib/rss/impl/rss_writer.cpp \
 src/rss_lib/db/impl/db.cpp \
 src/rss_lib/rss_download/impl/feed_download.cpp \
 src/rss_lib/rss_parse/impl/feed_parse.cpp \
 src/rss_ui/app_win/headlines_frame.hpp \
 src/rss_lib/rss/rss_article.hpp \
 src/rss_lib/rss/rss_feed.hpp \
 src/rss_lib/rss/rss_reader.hpp \
 src/rss_lib/rss/rss_util.hpp \
 src/rss_lib/rss/rss_writer.hpp \
 src/rss_lib/db/db.hpp \
 src/rss_lib/rss_download/feed_download.hpp \
 src/rss_lib/rss_parse/feed_parse.hpp

gautierrss_CPPFLAGS = -I$(top_srcdir)/src

#############

