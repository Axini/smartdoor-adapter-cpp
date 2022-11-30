// Copyright 2023 Axini B.V. https://www.axini.com, see: LICENSE.txt.

#include <string>

#include "spdlog/spdlog.h"

#include "adapter_core.hpp"
#include "broker_connection.hpp"
#include "handler.hpp"
#include "smartdoor_handler.hpp"

void run_test(std::string name, std::string url, std::string token) {
    BrokerConnection broker_connection(url, token);
    Handler* handler_ptr = new SmartDoorHandler();
    AdapterCore adapter_core(name, &broker_connection, handler_ptr);

    broker_connection.register_adapter_core(&adapter_core);
    handler_ptr -> register_adapter_core(&adapter_core);
    adapter_core.start();

    // Wait for the thread of the BrokerConnection to be terminated (which is never).
    broker_connection.get_thread()->join();
}

// The adapter should connect to a server running AMP, announce itself with a name, and
// supply a valid adapter token. You can fill in your own adapter configuration here,
// or provide the parameters when starting the adapter.

const std::string ADAPTER_NAME = "smartdoor-adapter-cpp@machine-name";
const std::string URL = "wss://course02.axini.com:443/adapters";
const std::string TOKEN = "adapter token from AMP's adapter page";

int main(int argc, char* argv[]) {
    std::string name  = ADAPTER_NAME;
    std::string url   = URL;
    std::string token = TOKEN;

    if (argc == 4) {
        name  = argv[1];
        url   = argv[2];
        token = argv[3];
    } else if (argc != 1) {
        std::cout << "usage: adapter <name> <url> <token>" << std::endl;
        exit(1);
    }

    spdlog::info("Starting adapter: " + ADAPTER_NAME);
    run_test(name, url, token);

    // Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();
}
