// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#ifndef ADAPTER_CORE_HPP
#define ADAPTER_CORE_HPP

#include <string>
#include "handler.hpp"

#include "pa_protobuf.hpp"
using namespace PluginAdapter::Api;

class BrokerConnection;

enum State { DISCONNECTED, CONNECTED, ANNOUNCED, CONFIGURED, READY, ERROR };

// The AdapterCore keeps the State of the adapter. It communicates with the
// BrokerConnection and the Handler, which connects to the SUT.
class AdapterCore {
public:
    AdapterCore(std::string name, BrokerConnection* broker_connection_ptr,
                Handler* handler_ptr);
    ~AdapterCore();

    void start();
    void on_open();
    void on_close(int code, std::string reason);
    void handle_message(std::string msg);
    void send_response(Label label, std::string, long);
    void send_ready();

private:
    void on_configuration(Configuration configuration);
    void on_label(Label label);
    void on_reset();
    void on_error(std::string message);

    void send_message(Message message);
    void send_stimulus(Label label, std::string, long, long);
    void send_error(std::string message);

private:
    std::string        adapter_name;
    BrokerConnection*  broker_connection_ptr;
    Handler*           handler_ptr;
    State              state;
};

#endif // ADAPTER_CORE_HPP
