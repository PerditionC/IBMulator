SUBDIRS = rocket
AM_CXXFLAGS = @CXX_DEFAULTS@ @BASECFLAGS@ -I$(top_srcdir)/src
 

noinst_LIBRARIES = libgui.a
libgui_a_SOURCES = \
	gui.cpp \
	guifx.cpp \
	vector.cpp \
	matrix.cpp \
	keymap.cpp \
	rocket/file_interface.cpp \
	rocket/rend_interface.cpp \
	rocket/sys_interface.cpp \
	format.cpp \
	window.cpp \
	windows/desktop.cpp \
	windows/interface.cpp \
	windows/normal_interface.cpp \
	windows/realistic_interface.cpp \
	windows/debugtools.cpp \
	windows/status.cpp \
	windows/sysdebugger.cpp \
	windows/sysdebugger286.cpp \
	windows/sysdebugger386.cpp \
	windows/stats.cpp \
	windows/devstatus.cpp \
	windows/fileselect.cpp

noinst_HEADERS = \
	gui.h \
	guifx.h \
	vector.h \
	vector.inl.h \
	matrix.h \
	matrix.inl.h \
	keymap.h \
	rocket/file_interface.h \
	rocket/rend_interface.h \
	rocket/sys_interface.h \
	format.h \
	window.h \
	windows/desktop.h \
	windows/interface.h \
	windows/normal_interface.h \
	windows/realistic_interface.h \
	windows/debugtools.h \
	windows/status.h \
	windows/sysdebugger.h \
	windows/sysdebugger286.h \
	windows/sysdebugger386.h \
	windows/stats.h \
	windows/devstatus.h \
	windows/fileselect.h
	