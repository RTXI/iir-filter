PLUGIN_NAME = iir_filter

RTXI_INCLUDES = /usr/local/lib/rtxi_includes

HEADERS = iir-filter.h\
          $(RTXI_INCLUDES)/DSP/iir_dsgn.h\
          $(RTXI_INCLUDES)/DSP/dir1_iir.h\
          $(RTXI_INCLUDES)/DSP/unq_iir.h\
          $(RTXI_INCLUDES)/DSP/buttfunc.h\
          $(RTXI_INCLUDES)/DSP/chebfunc.h\
          $(RTXI_INCLUDES)/DSP/elipfunc.h\
          $(RTXI_INCLUDES)/DSP/bilinear.h\
          $(RTXI_INCLUDES)/DSP/filtfunc.h\
          $(RTXI_INCLUDES)/DSP/cmpxpoly.h\
          $(RTXI_INCLUDES)/DSP/poly.h\
          $(RTXI_INCLUDES)/DSP/pause.h\
          $(RTXI_INCLUDES)/DSP/complex.h\
          $(RTXI_INCLUDES)/DSP/unwrap.h\
          $(RTXI_INCLUDES)/DSP/laguerre.h\
          $(RTXI_INCLUDES)/DSP/log2.h

SOURCES = iir-filter.cpp \
          moc_iir-filter.cpp\
          $(RTXI_INCLUDES)/DSP/iir_dsgn.cpp\
          $(RTXI_INCLUDES)/DSP/dir1_iir.cpp\
          $(RTXI_INCLUDES)/DSP/unq_iir.cpp\
          $(RTXI_INCLUDES)/DSP/buttfunc.cpp\
          $(RTXI_INCLUDES)/DSP/chebfunc.cpp\
          $(RTXI_INCLUDES)/DSP/elipfunc.cpp\
          $(RTXI_INCLUDES)/DSP/bilinear.cpp\
          $(RTXI_INCLUDES)/DSP/filtfunc.cpp\
          $(RTXI_INCLUDES)/DSP/cmpxpoly.cpp\
          $(RTXI_INCLUDES)/DSP/poly.cpp\
          $(RTXI_INCLUDES)/DSP/pause.cpp\
          $(RTXI_INCLUDES)/DSP/complex.cpp\
          $(RTXI_INCLUDES)/DSP/unwrap.cpp\
          $(RTXI_INCLUDES)/DSP/laguerre.cpp\
          $(RTXI_INCLUDES)/DSP/log2.cpp

LIBS = -lqwt

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
