#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <system_error>

enum class CommandType : uint8_t {
    PUT=0x01,
    GET=0x02,
    DEL=0x03,
    SCAN=0x04,
    COMPACT=0x05,
    SHUTDOWN=0x06
};

enum class ResponseStatus : uint8_t {
    OK = 0x00,
    NOT_FOUND = 0x01,
    INVALID_REQUEST = 0x02,
    INTERNAL_ERROR = 0x03
};

struct Request {
    CommandType command{};
    std::string key;
    std::string value;
    std::string start_key;
    std::string end_key;
};

struct Response {
    ResponseStatus status{ResponseStatus::OK};
    std::string value;
    std::string message;
    std::vector<std::pair<std::string, std::string>> entries;
};

bool send_request(int socket_fd, const Request& request, std::string& error);
bool receive_request(int socket_fd, Request& request, std::string& error);
bool send_response(int socket_fd, const Response& response, std::string& error);
bool receive_response(int socket_fd, Response& response, std::string& error);

#endif
