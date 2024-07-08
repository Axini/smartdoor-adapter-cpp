// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#ifndef ADAPTER_CORE_HPP
#define ADAPTER_CORE_HPP

#include <string>
#include <thread>

#include "qthread.hpp"
#include "handler.hpp"

#include "pa_protobuf.hpp"
using namespace PluginAdapter::Api;

class BrokerConnection;

enum State { DISCONNECTED, CONNECTED, ANNOUNCED, CONFIGURED, READY, ERROR };

// This class implements the core of a plugin-adapter. It handles the
// connection to the broker (BrokerConnection) and the connection to
// the SUT (via the Handler).
// Initially the adapter is in a DISCONNECTED state.
class AdapterCore {
public:
    AdapterCore(std::string name, BrokerConnection* broker_connection_ptr,
                Handler* handler_ptr);
    ~AdapterCore();

    void start();
    void on_open();
    void on_close(int code, std::string reason);
    void handle_message_from_amp(std::string msg);
    void send_response(Label label, std::string, long);
    void send_ready();
    void send_stimulus_confirmation(Label confirmation);

private:
    void handle_message(std::string msg);
    void on_configuration(Configuration configuration);
    void on_label(Label label);
    void on_reset();
    void on_error(std::string message);
    void send_error(std::string message);
    void queue_message_to_amp(Message message);
    void send_message_to_amp(Message message);
    void clear_qthread_queues();

private:
    std::string        adapter_name;
    BrokerConnection*  broker_connection_ptr;
    Handler*           handler_ptr;
    State              state;

    QThread<Message>        qthread_to_amp;
    QThread<std::string>    qthread_handle_message;
};

#endif // ADAPTER_CORE_HPP
