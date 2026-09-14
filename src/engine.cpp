#include "engine.hpp"
#include <iostream>

Engine::Engine(const std::string& db_path) :wal(db_path) {
    std::cout << "[Engine] Initializing storage engine with WAL: " << db_path << "\n";

    std::vector<WalRecord> records=wal.recover();

    size_t replayed_count=0;
    for(const auto&rec: records){
        if(rec.op==WalOp::PUT) {
            index.insert(rec.key, rec.value);
        } else if(rec.op == WalOp::DELETE){
            index.remove(rec.key);
            replayed_count++;
        }
    }

    std::cout<< "[Engine] Recovery complete. Replayed "<<replayed_count<<" log records. \n";
}

bool Engine::put(const std::string& key, const std::string& value){
    if(!wal.log_put(key, value)) {
        std::cerr << "[Engine Error] Failed to write PUT operation to WAL for key: " << key << "\n";
        return false;
    }
    index.insert(key, value);
    return true;
}

bool Engine::get(const std::string& key, std::string& out_value) {
    return index.search(key, out_value);
}

bool Engine::del(const std::string& key) {
    if(!wal.log_delete(key)){
        std::cerr << "[Engine Error] Failed to write DELETE operation to WAL for key: " << key << "\n";
        return false;
    }
    return index.remove(key);
}

void Engine::scan(){
    index.scan_all();
}

bool Engine::compact_log() {
    std::cout << "[Engine] Starting WAL compaction...\n";
    if(wal.compact()){
        std::cout << "[Engine] WAL compaction completed successfull. \n";
        return true;
    }
    std::cerr << "[Engine Error] WAL compaction failed.\n";
    return false;
}
