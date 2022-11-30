// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#ifndef BROKER_CONNECTION_HPP
#define BROKER_CONNECTION_HPP

#include <string>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

#include <websocketpp/common/thread.hpp>
#include <websocketpp/common/memory.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::connection_hdl connection_hdl;
typedef client::connection_ptr connection_ptr;

class AdapterCore;

// The BrokerConnection is responsible for the WebSocket connection to AMP.
class BrokerConnection {
public:
    BrokerConnection(std::string uri, std::string token);
    ~BrokerConnection();

    void connect();
    void close(int, std::string);
    void send(std::string message);
    void send(void const * payload, size_t len);

    websocketpp::lib::shared_ptr<websocketpp::lib::thread> get_thread();
    void register_adapter_core(AdapterCore* adapter_core_ptr);

private:
    void on_socket_init(connection_hdl hdl);
    context_ptr on_tls_init(connection_hdl hdl);

    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_fail(connection_hdl hdl);
    void on_message(connection_hdl hdl, message_ptr msg);

private:
    client m_endpoint;
    websocketpp::connection_hdl m_hdl;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;

    AdapterCore* adapter_core_ptr;
    std::string server_uri;
    std::string amp_token;
};

#endif // BROKER_CONNECTION_HPP
