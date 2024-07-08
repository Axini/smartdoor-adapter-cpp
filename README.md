# Description

This project defines a C++ implementation of a plugin adapter (PA) for Axini's standalone SmartDoor SUT. It connects the Axini Modeling Platform (AMP) to the standalone SmartDoor SUT. It has been developed to serve as an example implementation for the Axini Adapter Training.

Disclaimer: C++ is not the lingua franca at Axini. Without a doubt we will have violated some C++ conventions and guidelines that we are not aware of. 

The organization and architecture of the C++ adapter is strongly based on Axini's Ruby plugin-adapter-api version 4.x and a similar Java, Python and Ruby implementations of a plugin adapter for the standalone SmartDoor SUT. 

This is still an early version of the implementation; it is still work in progress.

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

Starting with version 22, protobuf depends on the Google's Abseil Common Libraries.

## WebSocket++
https://github.com/zaphoyd/websocketpp

WebSocket++ is a header only C++ library that implements RFC6455 The WebSocket Protocol. 

## spdlog
https://github.com/gabime/spdlog

spdlog is a header-only (or compiled), C++ logging library.


# Compilation of the plugin-adapter

The adapter can be compiled with a C++ 17 compiler, e.g., Clang or gcc. 

To compile the plugin adapter, the libraries *boost*, *protobuf*, *websocket++* and *spdlog* are expected to be installed globally. The makefile expects the include files of these libraries either in `/usr/local/include` and the compiled libraries to be installed into `/usr/local/lib`, or in the specific directories of the installed libraries. Furthermore, protobuf's `protoc` compiler should be installed.

The source distribution in `./src` contains a `makefile` with the target 'all': 
```
$ make all
```
This will will generate all necessary C++ Protobuf files, will compile all 
C++ files and will build the plugin-adapter executable as `../build/adapter`.

The plugin-adapter expects three arguments:
```
../build/adapter <name> <url> <token>
```
where `<name>` is the name of the apdater (will be shown in AMP's adapter page),
`<url>` is the Websocket URL of AMP, and `<token>` the adapter token.

When using the default configuration, the adapter expects the standalone SmartDoor SUT to run locally and listening to port 3001.

## Versions used

This C++ adapter has been built succesfully on macOS 13.6.6 (Ventura) on an Intel MacBook Pro, using:

    Apple Clang C++ version 15.0.0 (using C++ version 17)
    boost 1.85.0 
    protobuf 21.11
    WebSocket++ 0.8.2
    spdlog 1.14.1


# Current limitations

- Documentation is lacking. Minor comments for the classes and methods.
- The C++ application is developed by a non-native C++ programmer; the application may include Ruby-style constructs.
- The application (esp. the AdapterCore class) is not yet Thread safe.
- The BrokerConnection and SmartDoorConnection share similar code; they could be defined as subclasses of the same (abstract) Connection class which defines the overlapping methods. Note that this is only possible for the adapter for the SmartDoor SUT as both the connection to AMP and the SUT is over WebSockets.
- The logging of the adapter is rather verbose. Several of the spdlog::info calls could be replaced by spdlog::debug calls.
- Error handling should be improved upon.
- Virtual stimuli to inject bad weather behavior have to be added.
- (Unit) tests are missing.


# Some notes on the implementation
The AMP related code is stored in src/adapter/generic and can be used as-is for **any** Python plugin adapter. All SUT specific code (in this case for the SmartDoor SUT) is stored in src/adapter/smartdoor and should be modified for any new SUT.

## Threads
The main thread of the adapter ensures that messages from AMP are received and handled. A SmartdoorConnection object (in ./src/smartdoor_handler.cpp) starts a separate thread which is used for the messages from the SmartDoor SUT over the WebSocket connection between the SUT and the adapter. 

The class QThread (in ./src/adapter/generic) manages a queue of items and a Thread. Items can be added to the queue and the thread processes items from the queue in a FIFO manner. The queue can also be emptied. The plugin adapter (class AdapterCore in src/adapter/generic) uses two QThreads for (i) handling messages from AMP and (ii) sending messages to AMP. This ensures that messages from AMP (stimuli) and the SUT (responses) are serviced immediately: any resulting message is added to a queue of pending messages which is processed by either one of the two QThreads.

Using a separate QThread for sending the responses to AMP ensures that only a single WebSocket message can be in transit to AMP. 

The QThread for the messages from AMP (Configuration, Ready, stimuli) is needed for a different reason. The processing of actual ProtoBuf messages from AMP may take some (considerable) time. For instance, after a Configuration message, the SUT has to be started and after a Reset message the SUT has to be reset to its initial state. And even the handling of a stimulus at the SUT may take some time. The WebSocket library is single threaded which means that as long as the BrokerConnection's on_message method is being executed, the websocket library cannot handle any new WebSocket message from AMP, including heartbeat (ping) messages. Therefore, the AdapterCore uses a separate QThread to handle ProtoBuf messages from AMP. When a ProtoBuf message is received from AMP, the on_message method calls the AdapterCore's handle_message method which only adds this message to the queue of pending messages. This ensures that the WebSocket thread is always ready to react on new WebSocket messages from AMP.

The plugin adapter and all its threads are set to run forever. No code is added to gracefully terminate the adapter and its threads. 
