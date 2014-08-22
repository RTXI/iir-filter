PLUGIN_NAME = iir_filter

HEADERS = iir-filter.h\
          /usr/local/lib/rtxi_includes/DSP/iir_dsgn.h\
          /usr/local/lib/rtxi_includes/DSP/dir1_iir.h\
          /usr/local/lib/rtxi_includes/DSP/unq_iir.h\
          /usr/local/lib/rtxi_includes/DSP/buttfunc.h\
          /usr/local/lib/rtxi_includes/DSP/chebfunc.h\
          /usr/local/lib/rtxi_includes/DSP/elipfunc.h\
          /usr/local/lib/rtxi_includes/DSP/bilinear.h\
          /usr/local/lib/rtxi_includes/DSP/filtfunc.h\
          /usr/local/lib/rtxi_includes/DSP/cmpxpoly.h\
          /usr/local/lib/rtxi_includes/DSP/poly.h\
          /usr/local/lib/rtxi_includes/DSP/pause.h\
          /usr/local/lib/rtxi_includes/DSP/complex.h\
          /usr/local/lib/rtxi_includes/DSP/unwrap.h\
          /usr/local/lib/rtxi_includes/DSP/laguerre.h\
          /usr/local/lib/rtxi_includes/DSP/log2.h

SOURCES = iir-filter.cpp \
          moc_iir-filter.cpp\
          /usr/local/lib/rtxi_includes/DSP/iir_dsgn.cpp\
          /usr/local/lib/rtxi_includes/DSP/dir1_iir.cpp\
          /usr/local/lib/rtxi_includes/DSP/unq_iir.cpp\
          /usr/local/lib/rtxi_includes/DSP/buttfunc.cpp\
          /usr/local/lib/rtxi_includes/DSP/chebfunc.cpp\
          /usr/local/lib/rtxi_includes/DSP/elipfunc.cpp\
          /usr/local/lib/rtxi_includes/DSP/bilinear.cpp\
          /usr/local/lib/rtxi_includes/DSP/filtfunc.cpp\
          /usr/local/lib/rtxi_includes/DSP/cmpxpoly.cpp\
          /usr/local/lib/rtxi_includes/DSP/poly.cpp\
          /usr/local/lib/rtxi_includes/DSP/pause.cpp\
          /usr/local/lib/rtxi_includes/DSP/complex.cpp\
          /usr/local/lib/rtxi_includes/DSP/unwrap.cpp\
          /usr/local/lib/rtxi_includes/DSP/laguerre.cpp\
          /usr/local/lib/rtxi_includes/DSP/log2.cpp

LIBS = -lqwt

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
