#include "protocol.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cstring>
#include <limits>

namespace {

constexpr uint32_t kMaxFrameSize = 16U * 1024U * 1024U;

bool send_all(int socket_fd, const void* data, size_t size, std::string& error) {
    const auto* bytes = static_cast<const char*>(data);
    while (size > 0) {
        const ssize_t written = send(socket_fd, bytes, size, MSG_NOSIGNAL);
        if (written <= 0) {
            error = "socket write failed";
            return false;
        }
        bytes += written;
        size -= static_cast<size_t>(written);
    }
    return true;
}

bool receive_all(int socket_fd, void* data, size_t size, std::string& error) {
    auto* bytes = static_cast<char*>(data);
    while (size > 0) {
        const ssize_t received = recv(socket_fd, bytes, size, 0);
        if (received <= 0) {
            error = received == 0 ? "peer closed the connection" : "socket read failed";
            return false;
        }
        bytes += received;
        size -= static_cast<size_t>(received);
    }
    return true;
}

void append_u32(std::vector<uint8_t>& buffer, uint32_t value) {
    value = htonl(value);
    const auto* bytes = reinterpret_cast<const uint8_t*>(&value);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(value));
}

bool read_u32(const std::vector<uint8_t>& buffer, size_t& offset, uint32_t& value) {
    if (offset > buffer.size() || buffer.size() - offset < sizeof(uint32_t)) return false;
    std::memcpy(&value, buffer.data() + offset, sizeof(value));
    value = ntohl(value);
    offset += sizeof(value);
    return true;
}

bool append_string(std::vector<uint8_t>& buffer, const std::string& value, std::string& error) {
    if (value.size() > std::numeric_limits<uint32_t>::max()) {
        error = "string exceeds protocol limit";
        return false;
    }
    append_u32(buffer, static_cast<uint32_t>(value.size()));
    buffer.insert(buffer.end(), value.begin(), value.end());
    return true;
}

bool read_string(const std::vector<uint8_t>& buffer, size_t& offset, std::string& value) {
    uint32_t length = 0;
    if (!read_u32(buffer, offset, length) || length > buffer.size() - offset) return false;
    value.assign(reinterpret_cast<const char*>(buffer.data() + offset), length);
    offset += length;
    return true;
}

bool send_frame(int socket_fd, const std::vector<uint8_t>& body, std::string& error) {
    if (body.empty() || body.size() > kMaxFrameSize) {
        error = "invalid protocol frame size";
        return false;
    }
    const uint32_t network_size = htonl(static_cast<uint32_t>(body.size()));
    return send_all(socket_fd, &network_size, sizeof(network_size), error) &&
           send_all(socket_fd, body.data(), body.size(), error);
}

bool receive_frame(int socket_fd, std::vector<uint8_t>& body, std::string& error) {
    uint32_t network_size = 0;
    if (!receive_all(socket_fd, &network_size, sizeof(network_size), error)) return false;
    const uint32_t size = ntohl(network_size);
    if (size == 0 || size > kMaxFrameSize) {
        error = "invalid protocol frame size";
        return false;
    }
    body.resize(size);
    return receive_all(socket_fd, body.data(), body.size(), error);
}

bool is_valid_command(uint8_t value) {
    return value >= static_cast<uint8_t>(CommandType::PUT) &&
           value <= static_cast<uint8_t>(CommandType::SHUTDOWN);
}

bool is_valid_status(uint8_t value) {
    return value <= static_cast<uint8_t>(ResponseStatus::INTERNAL_ERROR);
}

} // namespace

bool send_request(int socket_fd, const Request& request, std::string& error) {
    std::vector<uint8_t> body{static_cast<uint8_t>(request.command)};
    switch (request.command) {
    case CommandType::PUT:
        if (!append_string(body, request.key, error) || !append_string(body, request.value, error)) return false;
        break;
    case CommandType::GET:
    case CommandType::DEL:
        if (!append_string(body, request.key, error)) return false;
        break;
    case CommandType::SCAN:
        if (!append_string(body, request.start_key, error) || !append_string(body, request.end_key, error)) return false;
        break;
    case CommandType::COMPACT:
    case CommandType::SHUTDOWN:
        break;
    }
    return send_frame(socket_fd, body, error);
}

bool receive_request(int socket_fd, Request& request, std::string& error) {
    std::vector<uint8_t> body;
    if (!receive_frame(socket_fd, body, error)) return false;
    if (!is_valid_command(body[0])) {
        error = "unknown command";
        return false;
    }
    size_t offset = 1;
    request = {};
    request.command = static_cast<CommandType>(body[0]);
    bool valid = true;
    switch (request.command) {
    case CommandType::PUT:
        valid = read_string(body, offset, request.key) && read_string(body, offset, request.value);
        break;
    case CommandType::GET:
    case CommandType::DEL:
        valid = read_string(body, offset, request.key);
        break;
    case CommandType::SCAN:
        valid = read_string(body, offset, request.start_key) && read_string(body, offset, request.end_key);
        break;
    case CommandType::COMPACT:
    case CommandType::SHUTDOWN:
        break;
    }
    if (!valid || offset != body.size()) {
        error = "malformed request";
        return false;
    }
    return true;
}

bool send_response(int socket_fd, const Response& response, std::string& error) {
    std::vector<uint8_t> body{static_cast<uint8_t>(response.status)};
    if (!append_string(body, response.value, error) || !append_string(body, response.message, error)) return false;
    if (response.entries.size() > std::numeric_limits<uint32_t>::max()) {
        error = "too many scan entries";
        return false;
    }
    append_u32(body, static_cast<uint32_t>(response.entries.size()));
    for (const auto& [key, value] : response.entries) {
        if (!append_string(body, key, error) || !append_string(body, value, error)) return false;
    }
    return send_frame(socket_fd, body, error);
}

bool receive_response(int socket_fd, Response& response, std::string& error) {
    std::vector<uint8_t> body;
    if (!receive_frame(socket_fd, body, error)) return false;
    if (!is_valid_status(body[0])) {
        error = "unknown response status";
        return false;
    }
    size_t offset = 1;
    uint32_t entry_count = 0;
    response = {};
    response.status = static_cast<ResponseStatus>(body[0]);
    if (!read_string(body, offset, response.value) || !read_string(body, offset, response.message) ||
        !read_u32(body, offset, entry_count)) {
        error = "malformed response";
        return false;
    }
    response.entries.reserve(entry_count);
    for (uint32_t i = 0; i < entry_count; ++i) {
        std::string key;
        std::string value;
        if (!read_string(body, offset, key) || !read_string(body, offset, value)) {
            error = "malformed scan response";
            return false;
        }
        response.entries.emplace_back(std::move(key), std::move(value));
    }
    if (offset != body.size()) {
        error = "trailing response bytes";
        return false;
    }
    return true;
}
