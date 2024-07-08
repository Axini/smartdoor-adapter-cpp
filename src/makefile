# makefile for the C++ implementation of the SmartDoor adapter

default:
	@echo "makefile does not have a default target"

GENERIC_DIR   = ./generic
SMARTDOOR_DIR = ./smartdoor
BUILD_DIR     = ../build

# directories that make will search for source files
VPATH = $(GENERIC_DIR)/:$(SMARTDOOR_DIR)/:$(BUILD_DIR)/

# ----- third-party directories

# Google protobuf 21 and OpenSSL 3 are *not* installed globally.

# directory where protobuf 21 is installed
GOOGLE_PROTOBUF_DIR = /usr/local/opt/protobuf@21

# directory where OpenSSL 3 is installed
OPEN_SSL_DIR = /usr/local/opt/openssl@3

# ----- C++ settings

CPP = c++
CPP_FLAGS = -std=c++17 -Wall
CPP_INCLUDE = -I$(GOOGLE_PROTOBUF_DIR)/include \
			  -I$(OPEN_SSL_DIR)/include \
			  -I$(BUILD_DIR) -I$(GENERIC_DIR) -I$(SMARTDOOR_DIR)

LINKER_FLAGS = -L$(OPEN_SSL_DIR)/lib -lssl -lcrypto \
			   -L$(GOOGLE_PROTOBUF_DIR)/lib -lprotobuf \
			   -L/usr/local/lib -lfmt

$(BUILD_DIR)/%.o: %.cc
	$(CPP) $(CPP_FLAGS) $(CPP_INCLUDE) -o $@ -c $<

$(BUILD_DIR)/%.o: %.cpp
	$(CPP) $(CPP_FLAGS) $(CPP_INCLUDE) -o $@ -c $<

# ----- generate protobuf C++ files and build pa_protobuf.a

PROTO_DIR = proto
PROTOC = $(GOOGLE_PROTOBUF_DIR)/bin/protoc -I$(PROTO_DIR) --cpp_out=$(BUILD_DIR)

PROTOS = announcement.proto configuration.proto label.proto message.proto
PA_CPP_H  = ${patsubst %.proto,$(BUILD_DIR)/%.pb.h,$(PROTOS)}
PA_CPP_CC = ${patsubst %.proto,$(BUILD_DIR)/%.pb.cc,$(PROTOS)}
PA_OBJS   = ${patsubst %.proto,$(BUILD_DIR)/%.pb.o,$(PROTOS)}

$(BUILD_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	$(PROTOC) $<

$(BUILD_DIR)/pa_protobuf.a: $(PA_CPP_CC) $(PA_OBJS)
	ar ru $@ $(PA_OBJS)
	ranlib $@

# ----- compile and link the plugin adapter

INCLUDES = broker_connection.hpp adapter_core.hpp handler.hpp \
			smartdoor_handler.hpp smartdoor_connection.hpp \
			axini_protobuf.hpp pa_protobuf.hpp

OBJS = broker_connection.o adapter_core.o handler.o axini_protobuf.o \
	 smartdoor_handler.o smartdoor_connection.o

BUILD_OBJS = ${patsubst %.o,$(BUILD_DIR)/%.o,$(OBJS)}

$(BUILD_DIR)/broker_connection.o: broker_connection.cpp broker_connection.hpp
$(BUILD_DIR)/adapter_core.o: adapter_core.cpp adapter_core.hpp \
	broker_connection.hpp handler.hpp qthread.hpp
$(BUILD_DIR)/handler.o: handler.cpp handler.hpp
$(BUILD_DIR)/axini_protobuf.o: axini_protobuf.cpp axini_protobuf.hpp
$(BUILD_DIR)/smartdoor_handler.o: smartdoor_handler.cpp smartdoor_handler.hpp \
	handler.hpp adapter_core.hpp
$(BUILD_DIR)/smartdoor_connection.o: smartdoor_connection.cpp \
	smartdoor_connection.hpp smartdoor_handler.cpp

$(BUILD_DIR)/adapter: adapter.cpp $(INCLUDES) $(BUILD_DIR)/pa_protobuf.a $(BUILD_OBJS)
	$(CPP) $(CPP_FLAGS) $(CPP_INCLUDE) -o $@ $< $(LINKER_FLAGS) $(BUILD_OBJS) $(BUILD_DIR)/pa_protobuf.a

all: $(BUILD_DIR)/adapter

# ----- cleaning up

clean:
	rm -f $(BUILD_DIR)/*.cc $(BUILD_DIR)/*.h
	rm -f $(BUILD_DIR)/*.o
	rm -f $(BUILD_DIR)/pa_protobuf.a

very_clean: clean
	rm -f $(BUILD_DIR)/adapter

# ----- make ZIP for distribution

ZIP = /usr/bin/zip
ZIP_NAME = smartdoor-adapter-cpp.zip
ZIP_OPTIONS = -o -rp -q --exclude=*.git* --exclude=*build*

THIS_DATE=`date`
THIS_COMMIT=`git rev-parse --short HEAD`

VERSION.txt:
	@echo "Version of this smartdoor-cpp adapter: " > $@
	@echo "- created on: ${THIS_DATE}" >> $@
	@echo "- latest git revision: ${THIS_COMMIT}" >> $@

zip: very_clean VERSION.txt
	pushd ../..; $(ZIP) $(ZIP_OPTIONS) $(ZIP_NAME) smartdoor-cpp; popd
	rm -f VERSION.txt
