#include "wal.hpp"


Wal::Wal(const std::string& filepath): wal_path(filepath){

    //bitwise or of write mode , binary mode, append mode
    log_stream.open(wal_path, std::ios::out | std::ios::binary | std::ios::app);
    if(!log_stream.is_open()){
        std::cerr<< "[WAL Error] failed to open/create WAL file: " << wal_path << "\n";
    }
}

Wal::~Wal(){
    if(log_stream.is_open()){
        log_stream.flush();
        log_stream.close();
    }
}


bool Wal::log_put(const std::string& key, const std::string& value){
    if(!log_stream.is_open()) return false;
    WalHeader header;
    header.op=static_cast<uint8_t>(WalOp::PUT);
    header.key_len=static_cast<uint32_t>(key.size());
    header.val_len=static_cast<uint32_t>(value.size());

    log_stream.write(reinterpret_cast<const char*>(&header), sizeof(WalHeader));
    log_stream.write(key.data(), key.size());
    log_stream.write(value.data(), value.size());
    log_stream.flush();
    return log_stream.good();
}

bool Wal::log_delete(const std::string&key){
    if(!log_stream.is_open()) return false;
    WalHeader header;
    header.op=static_cast<uint8_t>(WalOp::DELETE);
    header.key_len=static_cast<uint32_t>(key.size());
    header.val_len=0;
    log_stream.write(reinterpret_cast<const char*>(&header) , sizeof(WalHeader));
    log_stream.write(key.data(), key.size());
    log_stream.flush();
    return log_stream.good();
}


std::vector<WalRecord> Wal::recover(){
    std::vector<WalRecord> records;
    std::ifstream in_stream(wal_path, std::ios::in | std::ios::binary);
    if(!in_stream.is_open()){
        return records;
    }

    while(in_stream.peek()!=EOF){
        WalHeader header;
        in_stream.read(reinterpret_cast<char*>(&header), sizeof(WalHeader));
        if(in_stream.gcount() < static_cast<std::streamsize>(sizeof(WalHeader))){
            break;
            std::cout<<"read failed midway"<<"\n";
        }
        WalRecord record;
        record.op=static_cast<WalOp>(header.op);

        //resizes and reads
        record.key.resize(header.key_len);
        in_stream.read(&record.key[0], header.key_len);


        if(record.op==WalOp::PUT && header.val_len>0){
            record.value.resize(header.val_len);
            in_stream.read(&record.value[0], header.val_len);
        }


        if(in_stream.good()){
            records.push_back(std::move(record));
        }else{
            break;
        }
    }
    in_stream.close();
    return records;
}


bool Wal::compact(){
    std::vector<WalRecord> records=recover();
    std::unordered_map<std::string, std::string> active_state;
    for(const auto&rec: records){
        if(rec.op==WalOp::PUT){
            active_state[rec.key]=rec.value;
        }else if(rec.op==WalOp::DELETE){
            active_state.erase(rec.key);
        }
    }
    std::string temp_path=wal_path+ ".tmp";
    {   
        Wal temp_wal(temp_path);
        for(const auto&[key, value] : active_state){
            temp_wal.log_put(key, value);
        }
    }
    if(log_stream.is_open()){
        log_stream.close();
    }
    std::filesystem::rename(temp_path, wal_path);
    log_stream.open(wal_path, std::ios::out|std::ios::binary|std::ios::app);
    return log_stream.is_open();
}