EXTRA_DIST = *.h po/*

if FLibSYSIncl
noinst_LTLIBRARIES = spec_FLibSYS.la
spec_FLibSYS_la_CXXFLAGS = -DMOD_INCL -fpic
spec_FLibSYS_la_LIBTOOLFLAGS = --tag=disable-shared
else
oscd_modul_LTLIBRARIES = spec_FLibSYS.la
spec_FLibSYS_la_LIBTOOLFLAGS = --tag=disable-static
endif

spec_FLibSYS_la_LDFLAGS = -module -avoid-version -no-undefined
spec_FLibSYS_la_SOURCES = statfunc.cpp xmlfnc.cpp varchfnc.cpp
spec_FLibSYS_la_LIBADD = $(LIB_FFTW3)


I18N_mod = $(oscd_modulpref)FLibSYS
include ../../../../I18N.mk
