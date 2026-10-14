#include "engine.hpp"
#include "cli.hpp"
#include "remote_store.hpp"
#include "server.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>

namespace {

bool parse_port(const char* text, uint16_t& port) {
    try {
        const unsigned long parsed = std::stoul(text);
        if (parsed == 0 || parsed > std::numeric_limits<uint16_t>::max()) return false;
        port = static_cast<uint16_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

void print_usage(const char* executable) {
    std::cerr << "Usage:\n"
              << "  " << executable << "                         # local CLI\n"
              << "  " << executable << " server [port] [wal]     # TCP database server\n"
              << "  " << executable << " client [host] [port]    # remote CLI\n";
}

} // namespace

int main(int argc, char* argv[]) {
    constexpr uint16_t kDefaultPort = 7070;
    if (argc == 1) {
        std::cout << "Starting local key-value store...\n";
        Engine db("data/storage.wal");
        Cli cli(db);
        cli.run();
        return 0;
    }

    const std::string mode = argv[1];
    if (mode == "server") {
        uint16_t port = kDefaultPort;
        if (argc >= 3 && !parse_port(argv[2], port)) {
            print_usage(argv[0]);
            return 1;
        }
        DbServer server(argc >= 4 ? argv[3] : "data/storage.wal", port);
        server.run();
        return 0;
    }
    if (mode == "client") {
        uint16_t port = kDefaultPort;
        if (argc >= 4 && !parse_port(argv[3], port)) {
            print_usage(argv[0]);
            return 1;
        }
        RemoteStore store(argc >= 3 ? argv[2] : "127.0.0.1", port);
        Cli cli(store);
        cli.run();
        return 0;
    }

    print_usage(argv[0]);
    return 1;
}
