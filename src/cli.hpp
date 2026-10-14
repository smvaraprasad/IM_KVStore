#ifndef CLI_HPP
#define CLI_HPP
#include "kvstore.hpp"

class Cli {
    private:
        KeyValueStore& db;
        void print_help() const;

    public:
        explicit Cli(KeyValueStore& store);
        void run(); 
};

#endif
