#include "remote_store.hpp"

#include "protocol.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

namespace {

int connect_to(const std::string& host, uint16_t port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* addresses = nullptr;
    const std::string service = std::to_string(port);
    if (getaddrinfo(host.c_str(), service.c_str(), &hints, &addresses) != 0) return -1;

    int socket_fd = -1;
    for (addrinfo* address = addresses; address != nullptr; address = address->ai_next) {
        socket_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (socket_fd >= 0 && connect(socket_fd, address->ai_addr, address->ai_addrlen) == 0) break;
        if (socket_fd >= 0) close(socket_fd);
        socket_fd = -1;
    }
    freeaddrinfo(addresses);
    return socket_fd;
}

} // namespace

RemoteStore::RemoteStore(std::string host, uint16_t port) : host_(std::move(host)), port_(port) {}

bool RemoteStore::execute(const Request& request, Response& response) {
    const int socket_fd = connect_to(host_, port_);
    if (socket_fd < 0) {
        std::cerr << "[Client Error] Unable to connect to " << host_ << ':' << port_ << "\n";
        return false;
    }
    std::string error;
    const bool success = send_request(socket_fd, request, error) && receive_response(socket_fd, response, error);
    close(socket_fd);
    if (!success) std::cerr << "[Client Error] " << error << "\n";
    return success;
}

bool RemoteStore::put(const std::string& key, const std::string& value) {
    Request request;
    request.command = CommandType::PUT;
    request.key = key;
    request.value = value;
    Response response;
    return execute(request, response) && response.status == ResponseStatus::OK;
}

bool RemoteStore::get(const std::string& key, std::string& value) {
    Request request;
    request.command = CommandType::GET;
    request.key = key;
    Response response;
    if (!execute(request, response) || response.status != ResponseStatus::OK) return false;
    value = std::move(response.value);
    return true;
}

bool RemoteStore::del(const std::string& key) {
    Request request;
    request.command = CommandType::DEL;
    request.key = key;
    Response response;
    return execute(request, response) && response.status == ResponseStatus::OK;
}

std::vector<std::pair<std::string, std::string>> RemoteStore::scan(
    const std::string& start_key, const std::string& end_key) {
    Request request;
    request.command = CommandType::SCAN;
    request.start_key = start_key;
    request.end_key = end_key;
    Response response;
    if (!execute(request, response) || response.status != ResponseStatus::OK) return {};
    return response.entries;
}

bool RemoteStore::compact() {
    Request request;
    request.command = CommandType::COMPACT;
    Response response;
    return execute(request, response) && response.status == ResponseStatus::OK;
}
