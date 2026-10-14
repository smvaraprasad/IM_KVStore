#include "server.hpp"

#include "protocol.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <thread>

DbServer::DbServer(const std::string& wal_path, uint16_t port) : engine_(wal_path), port_(port) {}

void DbServer::run() {
    const int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        std::cerr << "[Server Error] Could not create listening socket\n";
        return;
    }
    int reuse_address = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);
    if (bind(listen_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 || listen(listen_fd, 64) < 0) {
        std::cerr << "[Server Error] Could not bind to port " << port_ << ": " << std::strerror(errno) << "\n";
        close(listen_fd);
        return;
    }

    std::cout << "[Server] Listening on 0.0.0.0:" << port_ << "\n";
    while (true) {
        const int client_fd = accept(listen_fd, nullptr, nullptr);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            std::cerr << "[Server Error] accept failed: " << std::strerror(errno) << "\n";
            continue;
        }
        std::thread(&DbServer::serve_client, this, client_fd).detach();
    }
}

void DbServer::serve_client(int client_fd) {
    Request request;
    std::string error;
    if (!receive_request(client_fd, request, error)) {
        close(client_fd);
        return;
    }

    Response response;
    switch (request.command) {
    case CommandType::PUT:
        response.status = engine_.put(request.key, request.value) ? ResponseStatus::OK : ResponseStatus::INTERNAL_ERROR;
        break;
    case CommandType::GET:
        response.status = engine_.get(request.key, response.value) ? ResponseStatus::OK : ResponseStatus::NOT_FOUND;
        break;
    case CommandType::DEL:
        response.status = engine_.del(request.key) ? ResponseStatus::OK : ResponseStatus::NOT_FOUND;
        break;
    case CommandType::SCAN:
        response.entries = engine_.scan(request.start_key, request.end_key);
        break;
    case CommandType::COMPACT:
        response.status = engine_.compact() ? ResponseStatus::OK : ResponseStatus::INTERNAL_ERROR;
        break;
    case CommandType::SHUTDOWN:
        response.status = ResponseStatus::INVALID_REQUEST;
        response.message = "server shutdown is managed by its host process";
        break;
    }
    if (!send_response(client_fd, response, error)) {
        std::cerr << "[Server Error] Failed sending response: " << error << "\n";
    }
    close(client_fd);
}
