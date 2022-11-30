// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#include "spdlog/spdlog.h"
#include "broker_connection.hpp"
#include "adapter_core.hpp"

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

BrokerConnection::BrokerConnection(std::string uri, std::string token)
    : server_uri(uri)
    , amp_token(token) {

    // WebSocket++ logging: pretty verbose (everything except message payloads).
    // m_endpoint.set_access_channels(websocketpp::log::alevel::all);
    // m_endpoint.clear_access_channels(websocketpp::log::alevel::frame_payload);
    // m_endpoint.set_error_channels(websocketpp::log::elevel::all);

    // WebSocket++ no logging, we now use spdlog for that
    m_endpoint.set_access_channels(websocketpp::log::alevel::none);
    m_endpoint.clear_access_channels(websocketpp::log::alevel::none);
    m_endpoint.set_error_channels(websocketpp::log::elevel::none);

    // Initialize ASIO.
    m_endpoint.init_asio();

    // Marks the endpoint as perpetual, stopping it from exiting when empty.
    m_endpoint.start_perpetual();

    // Register the callback handlers.
    m_endpoint.set_socket_init_handler(bind(&BrokerConnection::on_socket_init,this,::_1));
    m_endpoint.set_tls_init_handler(bind(&BrokerConnection::on_tls_init,this,::_1));
    m_endpoint.set_open_handler(bind(&BrokerConnection::on_open,this,::_1));
    m_endpoint.set_close_handler(bind(&BrokerConnection::on_close,this,::_1));
    m_endpoint.set_fail_handler(bind(&BrokerConnection::on_fail,this,::_1));
    m_endpoint.set_message_handler(bind(&BrokerConnection::on_message,this,::_1,::_2));

    // Start a thread in the background which calls run.
    // This will start the ASIO io_service run loop. This will cause a single connection
    // to be made to the server. c.run() will exit when the connection is closed.
    m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(
        &client::run, &m_endpoint);
}

BrokerConnection::~BrokerConnection() {
    // A BrokerConnection does not "own" the AdapterCore, so we should *not* delete it.

    m_endpoint.stop_perpetual();

    websocketpp::lib::error_code ec;
    m_endpoint.close(m_hdl, websocketpp::close::status::going_away, "", ec);
    if (ec) {
        spdlog::error("BrokerConnection: error closing connection: " + ec.message());
    }

    m_thread->join();
}

void BrokerConnection::close(int code, std::string message) {
    spdlog::info("BrokerConnection::close");

    websocketpp::lib::error_code ec;
    m_endpoint.close(m_hdl, code, message, ec);
    if (ec) {
        spdlog::error("BrokerConnection: error closing connection: " + ec.message());
    }
}

void BrokerConnection::connect() {
    spdlog::info("BrokerConnection::connect");

    // Create connection and connect to server.
    websocketpp::lib::error_code ec;
    client::connection_ptr con = m_endpoint.get_connection(server_uri, ec);
    if (ec) {
        spdlog::error("BrokerConnection: connect initialization error" + ec.message());
        return;
    }

    con->append_header("Authorization", "Bearer " + amp_token);
    m_hdl = con->get_handle();
    m_endpoint.connect(con);
}

void BrokerConnection::on_socket_init(connection_hdl hdl) {
    spdlog::info("BrokerConnection::on_socket_init");
}

// TLS init handler. Do nothing special.
// See print_client_tls.cpp for more advanced TLS handling.
context_ptr BrokerConnection::on_tls_init(connection_hdl hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(
        boost::asio::ssl::context::sslv23);
    ctx -> set_default_verify_paths();
    return ctx;
}

void BrokerConnection::on_open(connection_hdl hdl) {
    spdlog::info("BrokerConnection::on_open");
    spdlog::info("BrokerConnection: connected to AMP: " + server_uri);
    adapter_core_ptr->on_open();
}

void BrokerConnection::on_close(connection_hdl hdl) {
    spdlog::info("BrokerConnection::on_close");

    connection_ptr con = m_endpoint.get_con_from_hdl(hdl);
    int code = con->get_remote_close_code();
    std::string status = websocketpp::close::status::get_string(code);
    std::string reason = con->get_remote_close_reason();

    std::stringstream s;
    s << "close code: " << code << " (" << status << "), "
      << "close reason: " << reason << "." ;
    spdlog::info("BrokerConnection: " + s.str());

    adapter_core_ptr->on_close(code, reason);
}

void BrokerConnection::on_fail(connection_hdl hdl) {
    spdlog::error("BrokerConnection::on_fail");
    connection_ptr con = m_endpoint.get_con_from_hdl(hdl);
    std::string msg = con->get_ec().message();
    spdlog::error("Error message: " + msg);
}

void BrokerConnection::on_message(connection_hdl hdl, message_ptr msg) {
    spdlog::info("BrokerConnection::on_message");

    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        std::stringstream s;
        s << "BrokerConnection: communication with AMP is binary\n"
          << "text message received from AMP: " + msg->get_payload();
        spdlog::error(s.str());
    } else {
        adapter_core_ptr->handle_message(msg->get_payload());
    }
}

void BrokerConnection::send(std::string message) {
    websocketpp::lib::error_code ec;
    m_endpoint.send(m_hdl, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        spdlog::error("BrokerConnection: error sending message: " + ec.message());
        return;
    }
}

void BrokerConnection::send(void const * payload, size_t len) {
    websocketpp::lib::error_code ec;

    m_endpoint.send(m_hdl, payload, len, websocketpp::frame::opcode::binary, ec);
    if (ec) {
        spdlog::error("BrokerConnection: error sending message: " + ec.message());
        return;
    }
}

websocketpp::lib::shared_ptr<websocketpp::lib::thread> BrokerConnection::get_thread() {
    return m_thread;
}

void BrokerConnection::register_adapter_core(AdapterCore* adapter_core_ptr) {
    this->adapter_core_ptr = adapter_core_ptr;
}
