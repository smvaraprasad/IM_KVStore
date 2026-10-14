#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <string>
#include <optional>
#include <vector>
#include <utility>
#include <shared_mutex>
#include "kvstore.hpp"
#include "bptree.hpp"
#include "wal.hpp"

class Engine final : public KeyValueStore {
private:
    Bptree<std::string, std::string> index;
    Wal wal;
    mutable std::shared_mutex mutex;

public:
    explicit Engine(const std::string& db_path);
    ~Engine() = default;

        //if some one tries to get a new engine and assign it to something else
        //that means, engine class is getting initiated with engine
        //we are blocking it by calling delete
        //copy blocker
        Engine(const Engine&) = delete;
        //assignment blocker
        Engine& operator=(const Engine&)=delete;

    bool put(const std::string& key, const std::string& value) override;
    bool get(const std::string& key, std::string& out_value) override;
    bool del(const std::string& key) override;
    std::vector<std::pair<std::string, std::string>> scan(
        const std::string& start_key = "", const std::string& end_key = "") override;
    bool compact() override;
};

#endif
