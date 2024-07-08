// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#include "spdlog/spdlog.h"
#include "smartdoor_connection.hpp"

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

SmartDoorConnection::SmartDoorConnection(std::string uri)
    : handler_ptr(0),
      server_uri(uri) {

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
    m_endpoint.set_socket_init_handler(bind(&SmartDoorConnection::on_socket_init,this,::_1));
    m_endpoint.set_open_handler(bind(&SmartDoorConnection::on_open,this,::_1));
    m_endpoint.set_close_handler(bind(&SmartDoorConnection::on_close,this,::_1));
    m_endpoint.set_fail_handler(bind(&SmartDoorConnection::on_fail,this,::_1));
    m_endpoint.set_message_handler(bind(&SmartDoorConnection::on_message,this,::_1,::_2));

    // Start a thread in the background which calls run.
    // This will start the ASIO io_service run loop. This will cause a single connection
    // to be made to the server. c.run() will exit when the connection is closed.
    m_thread = websocketpp::lib::make_shared<websocketpp::lib::thread>(
        &client::run, &m_endpoint);
}

SmartDoorConnection::~SmartDoorConnection() {
    m_endpoint.stop_perpetual();

    websocketpp::lib::error_code ec;
    m_endpoint.close(m_hdl, websocketpp::close::status::going_away, "", ec);
    if (ec) {
        spdlog::info("SmartDoorConnection: error closing connection: "  + ec.message());
    }

    m_thread->join();
}

void SmartDoorConnection::connect() {
    spdlog::info("SmartDoorConnection::connect");

    // Create connection and connect to server.
    websocketpp::lib::error_code ec;
    client::connection_ptr con = m_endpoint.get_connection(server_uri, ec);
    if (ec) {
        spdlog::error("SmartDoorConnection: connect initialization error: " + ec.message());
        return;
    }

    m_hdl = con->get_handle();
    m_endpoint.connect(con);
}

void SmartDoorConnection::close(int code, std::string message) {
    spdlog::info("SmartDoorConnection::close");

    websocketpp::lib::error_code ec;
    m_endpoint.close(m_hdl, code, message, ec);
    if (ec) {
        spdlog::error("SmartDoorConnection: error closing connection: " + ec.message());
    }
}

void SmartDoorConnection::send(std::string message) {
    spdlog::info("SmartDoorConnection::send: " + message);
    websocketpp::lib::error_code ec;
    m_endpoint.send(m_hdl, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        spdlog::error("SmartDoorConnection: error sending message: " + ec.message());
        return;
    }
}

void SmartDoorConnection::on_socket_init(connection_hdl hdl) {
    spdlog::info("SmartDoorConnection::on_socket_init");
}

void SmartDoorConnection::on_open(connection_hdl hdl) {
    spdlog::info("SmartDoorConnection::on_open");
    spdlog::info("SmartDoorConnection: connected to SUT: " + server_uri);
    handler_ptr->send_reset_to_sut();
    handler_ptr->send_ready_to_amp();
}

void SmartDoorConnection::on_close(connection_hdl hdl) {
    spdlog::info("SmartDoorConnection::on_close");

    connection_ptr con = m_endpoint.get_con_from_hdl(hdl);
    int code = con->get_remote_close_code();
    std::string status = websocketpp::close::status::get_string(code);
    std::string reason = con->get_remote_close_reason();

    std::stringstream s;
    s << "close code: " << code << " (" << status << "), "
      << "close reason: " << reason << "." ;
    spdlog::info("SmartDoorConnection: " + s.str());
}

void SmartDoorConnection::on_fail(connection_hdl hdl) {
    spdlog::error("SmartDoorConnection::on_fail");
    connection_ptr con = m_endpoint.get_con_from_hdl(hdl);
    std::string msg = con->get_ec().message();
    spdlog::error("Error message: " + msg);
}

// TODO: check that we only receive string messages
void SmartDoorConnection::on_message(connection_hdl hdl, message_ptr msg) {
    // spdlog::info("SmartDoorConnection::on_message");
    std::string message = msg->get_payload();
    spdlog::info("SmartDoorConnection: received from SUT: " + message);
    if (handler_ptr != 0) {
        handler_ptr->send_response_to_amp(message);
    }
}

void SmartDoorConnection::register_handler(SmartDoorHandler* handler_ptr) {
    this->handler_ptr = handler_ptr;
}
