PLUGIN_NAME = IIRfilter

HEADERS = IIRfilter.h

LIBS = -lqwt

SOURCES = IIRfilter.cpp \
    moc_IIRfilter.cpp\
		../include/DSP/iir_dsgn.cpp\
		../include/DSP/dir1_iir.cpp\
		../include/DSP/unq_iir.cpp\
		../include/DSP/buttfunc.cpp\
		../include/DSP/chebfunc.cpp\
		../include/DSP/elipfunc.cpp\
		../include/DSP/bilinear.cpp\
		../include/DSP/filtfunc.cpp\
		../include/DSP/cmpxpoly.cpp\
		../include/DSP/poly.cpp\
		../include/DSP/pause.cpp\
		../include/DSP/complex.cpp\
		../include/DSP/unwrap.cpp\
		../include/DSP/laguerre.cpp\
		../include/DSP/log2.cpp\
		../include/DSP/iir_dsgn.h\
		../include/DSP/dir1_iir.h\
		../include/DSP/unq_iir.h\
		../include/DSP/buttfunc.h\
		../include/DSP/chebfunc.h\
		../include/DSP/elipfunc.h\
		../include/DSP/bilinear.h\
		../include/DSP/filtfunc.h\
		../include/DSP/cmpxpoly.h\
		../include/DSP/poly.h\
		../include/DSP/pause.h\
		../include/DSP/complex.h\
		../include/DSP/unwrap.h\
		../include/DSP/laguerre.h\
		../include/DSP/log2.h


### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile