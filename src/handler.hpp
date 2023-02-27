// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#ifndef HANDLER_HPP
#define HANDLER_HPP

#include "pa_protobuf.hpp"
using namespace PluginAdapter::Api;

class AdapterCore;

// The Handler is an abstract base class, which declares the functions
// that the specific handler should implement. It communicates with the
// AdapterCore.

class Handler {
public:
    Handler();
    ~Handler();

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void reset() = 0;

    virtual std::string stimulate(Label stimulus) = 0;
    void send_ready_to_amp();

    void register_adapter_core(AdapterCore* adapter_core_ptr);

    void set_configuration(Configuration configuration);
    Configuration get_configuration();
    virtual Configuration default_configuration() = 0;

    // The labels supported by the plugin adapter.
    virtual std::vector<Label> get_supported_labels() = 0;

protected:
    AdapterCore*    adapter_core_ptr;
    Configuration   configuration;
};

#endif // HANDLER_HPP
