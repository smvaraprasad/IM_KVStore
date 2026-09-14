#include "engine.hpp"
#include "cli.hpp"
#include <iostream>

int main() {
    std::cout << "Starting High-Performance Key-value Storage Engine...\n";
    Engine db("data/storage.wal");
    Cli cli(db);
    cli.run();
    return 0;
}

