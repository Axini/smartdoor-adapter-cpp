# Description

This project defines a C++ implementation of a plugin adapter (PA) for Axini's standalone SmartDoor SUT. It connects the Axini Modeling Platform (AMP) to the standalone SmartDoor SUT. It has been developed to serve as an example implementation for the Axini Adapter Training.

Disclaimer: C++ is not the lingua franca at Axini. Without a doubt we will have violated some C++ conventions and guidelines that we are not aware of. 

The organization and architecture of the C++ adapter is strongly based on Axini's Ruby plugin-adapter-api version 4.x and a similar Java implementation of a plugin adapter for the standalone SmartDoor SUT. 

This is an initial version of the implementation; it is still work in progress.

The software is distributed under the MIT license, see LICENSE.txt.


# External libraries

The adapter uses several libraries from external parties.

## Boost
https://www.boost.org

Boost is well-known set of libraries for the C++ programming language. 

## Protocol Buffers (protobuf)
https://developers.google.com/protocol-buffers

Google Protocol Buffers is a free and open-source cross-platform data format used to serialize structured data. 

The directory ./proto contains the Protobuf .proto files defining the Protobuf messages of Axini's 'Plugin Adapter Protocol'. 

## WebSocket++
https://github.com/zaphoyd/websocketpp

WebSocket++ is a header only C++ library that implements RFC6455 The WebSocket Protocol. 

## spdlog
https://github.com/gabime/spdlog

spdlog is a header-only (or compiled), C++ logging library.


# Compilation of the plugin-adapter

The adapter can be compiled with a C++11 compiler, e.g., Clang or gcc. 

To compile the plugin adapter, the libraries *boost*, *protobuf*, *websocket++* and *spdlog* are expected to be installed globally. The makefile expects the include files of these libraries in /usr/local/include and the compiled libraries to be installed into /usr/local/lib. Furthermore, protobuf's `protoc` compiler should be installed.

The source distribution in ./src contains a makefile with two targets: 

* pa_protobuf_lib. This target calls the binary protoc compiler to generate support files for the Protobuf messages and compiles these C++ files into ./pa_protobuf, and creates the library ./pa_protobuf/pa_protobuf.a. It also creates the include file ../pa_protobuf/pa_protobuf.hpp which is included by the source files of the adapter.

* adapter. After generating ./pa_protobuf/pa_protobuf.a, the 'adapter' target can be used to compile all .cpp files and build the ./adapter executable.

When using the default configuration, the adapter expects the standalone SmartDoor SUT to run locally and listening to port 3001.

## Versions used

This C++ adapter has been built succesfully on macOS 11.7.1 (Big Sur), using:
    Apple Clang C++ version 13.0.0
    boost 1.80.0 
    protobuf 21.9 (aka 3.21.9)
    WebSocket++ 0.8.2
    spdlog 1.10.0


# Current limitations

- Documentation is lacking. No comments for the classes and methods.
- The C++ application is developed by a non-native C++ programmer; the application may include Ruby-style constructs.
- The application (esp. the AdapterCore class) is not yet Thread safe.
- The BrokerConnection and SmartDoorConnection share similar code; they could be defined as subclasses of the same (abstract) Connection class which defines the overlapping methods. Note that this is only possible for the adapter for the SmartDoor SUT as both the connection to AMP and the SUT is over WebSockets.
- The logging of the adapter is rather verbose. Several of the spdlog::info calls could be replaced by spdlog::debug calls.
- Error handling should be improved upon.
- Virtual stimuli to inject bad weather behavior have to be added.
- (Unit) tests are missing.


# License

The source code of the adapter is distributed under the BSD License. See LICENSE.txt.
