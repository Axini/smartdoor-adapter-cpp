// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#ifndef SMARTDOOR_HANDLER_HPP
#define SMARTDOOR_HANDLER_HPP

#include "handler.hpp"
#include "smartdoor_handler.hpp"

#include "pa_protobuf.hpp"
using namespace PluginAdapter::Api;

const std::string SMARTDOOR_URL          = "ws://localhost:3001";
const std::string SMARTDOOR_MANUFACTURER = "Axini";

const std::string RESET           = "RESET";
const std::string RESET_PERFORMED = "RESET_PERFORMED";

class SmartDoorConnection;

// The SmartDoorHandler is a specific implementation of Handler for the
// standalone SmartDoor SUT. The communication with the SUT is handled
// by a separate SmartDoorConnection object.

class SmartDoorHandler: public Handler {
public:
    SmartDoorHandler();
    ~SmartDoorHandler();

    void start();
    void stop();
    void reset();

    std::string stimulate(Label stimulus);

    Configuration default_configuration();
    std::vector<Label> get_supported_labels();

    void send_response_to_amp(std::string message);
    void send_reset_to_sut();

private:
    static Label       sut_message_to_label(std::string message);
    static std::string label_to_sut_message(Label stimulus);

private:
    SmartDoorConnection* smartdoor_connection_ptr;
};

#endif // SMARTDOOR_HANDLER_HPP
