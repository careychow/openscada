EXTRA_DIST = *.h po/*

noinst_LTLIBRARIES = libOPC_UA.la
libOPC_UA_la_SOURCES = libOPC_UA.cpp
libOPC_UA_la_CXXFLAGS = -fpic
libOPC_UA_la_LIBADD  = $(LIB_OpenSSL)
#libOPC_UA_la_LIBTOOLFLAGS = --tag=disable-shared

oscd_modul_LTLIBRARIES = daq_OPC_UA.la
daq_OPC_UA_la_LDFLAGS = -module -avoid-version -no-undefined
daq_OPC_UA_la_SOURCES = moduls.cpp mod_daq.cpp mod_prt.cpp libOPC_UA.cpp
daq_OPC_UA_la_LIBADD  = $(LIB_OpenSSL)
daq_OPC_UA_la_LIBTOOLFLAGS = --tag=disable-static

I18N_mod = $(oscd_modulpref)OPC_UA
include ../../../../I18N.mk
