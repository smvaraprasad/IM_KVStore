#ifndef CLI_HPP
#define CLI_HPP
#include "engine.hpp"

class Cli {
    private:
        Engine& db;
        void print_help() const;

    public:
        explicit Cli(Engine& engine);
        void run(); 
};

#endif
