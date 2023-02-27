# makefile for the C++ implementation of the SmartDoor adapter

default:
	@echo "makefile does not have a default target"

# ----- compile .proto files to C++ and build the pa_protobuf.a library

PROTO_DIR       = proto
PA_PROTOBUF_DIR = pa_protobuf
PROTOC 		  	= protoc -I$(PROTO_DIR) --cpp_out=$(PA_PROTOBUF_DIR)
MAKEFILE 	  	= makefile.pa_protobuf

pa_protobuf_lib:
	mkdir -p $(PA_PROTOBUF_DIR)
	cp -f $(MAKEFILE) $(PA_PROTOBUF_DIR)
	$(PROTOC) $(PROTO_DIR)/announcement.proto
	$(PROTOC) $(PROTO_DIR)/announcements.proto
	$(PROTOC) $(PROTO_DIR)/configuration.proto
	$(PROTOC) $(PROTO_DIR)/label.proto
	$(PROTOC) $(PROTO_DIR)/message.proto
	pushd $(PA_PROTOBUF_DIR); make -f $(MAKEFILE) lib; popd

# ----- compile adapter

CPP = c++
CPP_FLAGS = -std=c++11 -Wall
CPP_INCLUDE = -I/usr/local/include -I$(PA_PROTOBUF_DIR) \
			  -I/usr/local/opt/openssl@3/include

LINKER_FLAGS = -L/usr/local/opt/openssl@3/lib -lssl -lcrypto \
			   -L/usr/local/lib -lprotobuf -lfmt $(PA_PROTOBUF_DIR)/pa_protobuf.a

OBJS = broker_connection.o adapter_core.o handler.o \
			smartdoor_handler.o smartdoor_connection.o axini_protobuf.o
INCLUDES = broker_connection.hpp adapter_core.hpp handler.hpp \
			smartdoor_handler.hpp smartdoor_connection.hpp axini_protobuf.hpp

%.o : %.cpp
	$(CPP) $(CPP_FLAGS) $(CPP_INCLUDE) -c $<

broker_connection.o: broker_connection.cpp broker_connection.hpp
adapter_core.o: adapter_core.cpp adapter_core.hpp
handler.o: handler.cpp handler.hpp
axini_protobuf.o: axini_protobuf.cpp axini_protobuf.hpp
smartdoor_handler.o: smartdoor_handler.cpp smartdoor_handler.hpp handler.hpp
smartdoor_connection.o: smartdoor_connection.cpp smartdoor_connection.hpp

adapter: adapter.cpp $(INCLUDES) $(OBJS)
	$(CPP) $(CPP_FLAGS) $(CPP_INCLUDE) -o $@ $< $(OBJS) $(LINKER_FLAGS)
	mv adapter ..

all: pa_protobuf_lib adapter

# ----- cleaning up

clean:
	rm -f $(OBJS)
	rm -f VERSION.txt

very_clean: clean
	rm -f adapter
	rm -f -r $(PA_PROTOBUF_DIR)

# ----- make ZIP for distribution

ZIP = /usr/bin/zip
ZIP_NAME = smartdoor-adapter-cpp.zip
ZIP_OPTIONS = -o -rp -q --exclude=*.git*

THIS_DATE=`date`
THIS_COMMIT=`git rev-parse --short HEAD`

VERSION.txt:
	@echo "Version of this smartdoor-cpp adapter: " > $@
	@echo "- created on: ${THIS_DATE}" >> $@
	@echo "- latest git revision: ${THIS_COMMIT}" >> $@

zip: very_clean VERSION.txt
	pushd ..; $(ZIP) $(ZIP_OPTIONS) $(ZIP_NAME) smartdoor-cpp; popd
	rm -f VERSION.txt
