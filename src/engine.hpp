#ifndef ENGINE_HPP
#define ENGINE_HPP

#include <string>
#include <optional>
#include "bptree.hpp"
#include "wal.hpp"

class Engine {
    private:
        Bptree<std::string, std::string> index;
        Wal wal;
    public:
        explicit Engine(const std::string& db_path);
        ~Engine()=default;

        //if some one tries to get a new engine and assign it to something else
        //that means, engine class is getting initiated with engine
        //we are blocking it by calling delete
        //copy blocker
        Engine(const Engine&) = delete;
        //assignment blocker
        Engine& operator=(const Engine&)=delete;

        bool put(const std::string& key, const std::string& value);
        bool get(const std::string& key, std::string& out_value);
        bool del(const std::string& key);

        void scan();
        bool compact_log();
};

#endif