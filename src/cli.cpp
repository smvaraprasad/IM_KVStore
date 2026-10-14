#include "cli.hpp"
#include <iostream>
#include <sstream>
#include <string>

Cli::Cli(KeyValueStore& store) : db(store) {}

void Cli::print_help() const {
    std::cout << "\n=== Storage Engine Commands ===\n"
              << "  PUT <key> <val>  : Insert or update a key-value pair\n"
              << "  GET <key>        : Fetch value for a given key\n"
              << "  DEL <key>        : Remove key from database\n"
              << "  SCAN [start] [end]: Display sorted pairs in an inclusive range\n"
              << "  COMPACT          : Rewrite WAL file to reclaim space\n"
              << "  HELP             : Show command list\n"
              << "  EXIT             : Shutdown storage engine\n"
              << "================================\n\n";
}

void Cli::run(){
    print_help();
    std::string line;
    while(true) {
        std::cout << "db> ";
        if(!std::getline(std::cin , line)) {
            break;
        }

        std::stringstream ss(line);
        std::string command;
        ss>>command;

        if(command.empty()) continue;

        for(char& c: command) c=static_cast<char>(toupper(c));

        if(command=="EXIT") {
            std::cout << "Shutting down engine gracefully...\n";
            break;
        }else if(command=="HELP"){
            print_help();
        }else if(command=="PUT"){
            std::string key, val;
            ss >> key;
            std::getline(ss>>std::ws, val);

            if(key.empty() || val.empty()){
                std::cout <<"Usage: PUT <key> <value> \n";
            }else{
                if(db.put(key, val)) {
                    std::cout << "OK \n";
                }
            }
        }else if(command=="GET"){
            std::string key;
            ss>>key;
            if(key.empty()){
                std::cout <<"Usage: GET <key> \n";
            }else{
                std::string result;
                if(db.get(key, result)){
                    std::cout << "--> " << result << "\n"; 
                }else{
                    std::cout << "(error) Key not found \n";
                }
            }
        }else if(command=="DEL"){
            std::string key;
            ss >> key;

            if(key.empty()){
                std::cout << "Usage: DEL <key> \n";
            }else {
                if(db.del(key)){
                    std::cout << "OK\n";
                }else{
                    std::cout << "error Key not found \n";
                }
            }
        }else if(command=="SCAN"){
            std::string start_key;
            std::string end_key;
            ss >> start_key >> end_key;
            const auto entries = db.scan(start_key, end_key);
            for (const auto& [key, value] : entries) {
                std::cout << key << " -> " << value << "\n";
            }
        }else if(command=="COMPACT"){
            if (db.compact()) {
                std::cout << "OK\n";
            } else {
                std::cout << "(error) Compaction failed\n";
            }
        }else{
            std::cout << "Unknown commad: '" << command << "'. Type HELP for command list. \n";
        }
    }
}
