# Makefile for the C++ implementation of the SmartDoor adapter

default:
	@echo "makefile does not have a default target"

GENERIC_DIR   = ./generic
SMARTDOOR_DIR = ./smartdoor
BUILD_DIR     = ../build

# Directories that make will search for source files
VPATH = $(GENERIC_DIR)/:$(SMARTDOOR_DIR)/:$(BUILD_DIR)/

# Compiler settings
CPP = c++
CPP_FLAGS = -std=c++17 -Wall -v

# Include paths
CPP_INCLUDE = -I/usr/include/google/protobuf \
              -I/usr/include/openssl \
              -I../build -I./generic -I./smartdoor

# Library paths and linker flags
LINKER_FLAGS = -L/usr/lib/x86_64-linux-gnu -lssl -lcrypto \
               -lprotobuf -lfmt -lwebsockets -lcpprest \
               -lboost_system -lboost_thread

# Object file rules
$(BUILD_DIR)/%.o: %.cpp
	$(CPP) $(CPP_FLAGS) $(CPP_INCLUDE) -o $@ -c $<

# Generate protobuf C++ files and build pa_protobuf.a
PROTO_DIR = proto
PROTOC = protoc -I$(PROTO_DIR) --cpp_out=$(BUILD_DIR)

PROTOS = announcement.proto configuration.proto label.proto message.proto
PA_CPP_H  = $(patsubst %.proto,$(BUILD_DIR)/%.pb.h,$(PROTOS))
PA_CPP_CC = $(patsubst %.proto,$(BUILD_DIR)/%.pb.cc,$(PROTOS))
PA_OBJS   = $(patsubst %.proto,$(BUILD_DIR)/%.pb.o,$(PROTOS))

$(BUILD_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	$(PROTOC) $<

$(BUILD_DIR)/pa_protobuf.a: $(PA_CPP_CC) $(PA_OBJS)
	ar r $@ $(PA_OBJS)
	ranlib $@

# Compile and link the plugin adapter
INCLUDES = broker_connection.hpp adapter_core.hpp handler.hpp \
           smartdoor_handler.hpp smartdoor_connection.hpp \
           axini_protobuf.hpp pa_protobuf.hpp

OBJS = broker_connection.o adapter_core.o handler.o axini_protobuf.o \
       smartdoor_handler.o smartdoor_connection.o

BUILD_OBJS = $(patsubst %.o,$(BUILD_DIR)/%.o,$(OBJS))

$(BUILD_DIR)/adapter: adapter.cpp $(INCLUDES) $(BUILD_DIR)/pa_protobuf.a $(BUILD_OBJS)
	$(CPP) $(CPP_FLAGS) $(CPP_INCLUDE) -o $@ $< $(LINKER_FLAGS) $(BUILD_OBJS) $(BUILD_DIR)/pa_protobuf.a

all: $(BUILD_DIR)/adapter

# Cleaning up
clean:
	rm -f $(BUILD_DIR)/*.cc $(BUILD_DIR)/*.h
	rm -f $(BUILD_DIR)/*.o
	rm -f $(BUILD_DIR)/pa_protobuf.a

very_clean: clean
	rm -f $(BUILD_DIR)/adapter

# Make ZIP for distribution
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
