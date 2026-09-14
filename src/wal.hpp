#ifndef WAL_HPP
#define WAL_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <filesystem>

enum class WalOp : uint8_t {
    PUT=0x01,
    DELETE=0x02
};

#pragma pack(push, 1)
struct WalHeader {
    uint8_t op;
    uint32_t key_len;
    uint32_t val_len;
};
#pragma pack(pop)

struct WalRecord {
    WalOp op;
    std::string key;
    std::string value;
};

class Wal{
    private:
        std::string wal_path;
        std::ofstream log_stream;
    public:
        explicit Wal(const std::string& filepath);
        ~Wal();

        Wal(const Wal&) = delete;
        Wal& operator=(const Wal&)=delete;

        bool log_put(const std::string& key, const std::string& value);
        bool log_delete(const std::string& key);

        std::vector<WalRecord> recover();
        bool compact();
        
};
#endif