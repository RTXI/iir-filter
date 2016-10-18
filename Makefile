PLUGIN_NAME = iir_filter

RTXI_INCLUDES =

HEADERS = iir-filter.h\

SOURCES = iir-filter.cpp \
          moc_iir-filter.cpp

LIBS = -lrtdsp

### Do not edit below this line ###

include $(shell rtxi_plugin_config --pkgdata-dir)/Makefile.plugin_compile
