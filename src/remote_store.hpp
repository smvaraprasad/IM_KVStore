#ifndef REMOTE_STORE_HPP
#define REMOTE_STORE_HPP

#include "kvstore.hpp"

#include <cstdint>
#include <string>

struct Request;
struct Response;

class RemoteStore final : public KeyValueStore {
public:
    RemoteStore(std::string host, uint16_t port);

    bool put(const std::string& key, const std::string& value) override;
    bool get(const std::string& key, std::string& value) override;
    bool del(const std::string& key) override;
    std::vector<std::pair<std::string, std::string>> scan(
        const std::string& start_key = "", const std::string& end_key = "") override;
    bool compact() override;

private:
    bool execute(const Request& request, Response& response);

    std::string host_;
    uint16_t port_;
};

#endif
