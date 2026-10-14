#ifndef SERVER_HPP
#define SERVER_HPP

#include "engine.hpp"

#include <cstdint>

class DbServer {
public:
    DbServer(const std::string& wal_path, uint16_t port);
    void run();

private:
    void serve_client(int client_fd);

    Engine engine_;
    uint16_t port_;
};

#endif
