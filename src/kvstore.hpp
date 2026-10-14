#ifndef KVSTORE_HPP
#define KVSTORE_HPP

#include <string>
#include <vector>
#include <utility>

class KeyValueStore {
public:
    virtual ~KeyValueStore() = default;

    virtual bool put(const std::string& key, const std::string& value) = 0;
    virtual bool get(const std::string& key, std::string& value) = 0;
    virtual bool del(const std::string& key) = 0;
    // Empty bounds mean unbounded. Both non-empty bounds are inclusive.
    virtual std::vector<std::pair<std::string, std::string>> scan(
        const std::string& start_key = "", const std::string& end_key = "") = 0;
    virtual bool compact() = 0;
};

#endif
