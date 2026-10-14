#include "engine.hpp"
#include <iostream>
#include <mutex>

Engine::Engine(const std::string& db_path) :wal(db_path) {
    std::cout << "[Engine] Initializing storage engine with WAL: " << db_path << "\n";

    std::vector<WalRecord> records=wal.recover();

    size_t replayed_count=0;
    for(const auto&rec: records){
        if(rec.op==WalOp::PUT) {
            index.insert(rec.key, rec.value);
            replayed_count++;
        } else if(rec.op == WalOp::DELETE){
            index.remove(rec.key);
            replayed_count++;
        }
    }

    std::cout<< "[Engine] Recovery complete. Replayed "<<replayed_count<<" log records. \n";
}

bool Engine::put(const std::string& key, const std::string& value){

    //acquire exclusive write lock
    std::unique_lock<std::shared_mutex> lock(mutex);
    if(!wal.log_put(key, value)) {
        std::cerr << "[Engine Error] Failed to write PUT operation to WAL for key: " << key << "\n";
        return false;
    }
    index.insert(key, value);
    return true;
}

bool Engine::get(const std::string& key, std::string& out_value) {
    //acquire shared read lock (multiple concurrent readers allowed)
    std::shared_lock<std::shared_mutex> lock(mutex);
    return index.search(key, out_value);
}

bool Engine::del(const std::string& key) {
    //acquire exclusive write lock
    std::unique_lock<std::shared_mutex> lock(mutex);
    std::string ignored;
    if (!index.search(key, ignored)) {
        return false;
    }
    if(!wal.log_delete(key)){
        std::cerr << "[Engine Error] Failed to write DELETE operation to WAL for key: " << key << "\n";
        return false;
    }
    return index.remove(key);
}

std::vector<std::pair<std::string, std::string>> Engine::scan(
    const std::string& start_key, const std::string& end_key) {
    //acquired a shared read lock across entire scan traversal
    std::shared_lock<std::shared_mutex> lock(mutex);
    auto entries = index.scan_all();
    if (start_key.empty() && end_key.empty()) return entries;

    std::vector<std::pair<std::string, std::string>> result;
    for (auto& entry : entries) {
        if ((!start_key.empty() && entry.first < start_key) ||
            (!end_key.empty() && entry.first > end_key)) continue;
        result.push_back(std::move(entry));
    }
    return result;
}

bool Engine::compact() {
    //acquire exclusive write lock to prevent concurrent reads/writes during compaction
    std::unique_lock<std::shared_mutex> lock(mutex);

    std::cout << "[Engine] Starting WAL compaction...\n";
    auto snapshot=index.scan_all();
    if(wal.compact(snapshot)){
        std::cout << "[Engine] WAL compaction completed successfull. \n";
        return true;
    }
    std::cerr << "[Engine Error] WAL compaction failed.\n";
    return false;
}
