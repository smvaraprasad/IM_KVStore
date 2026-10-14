#include "wal.hpp"
#include <iostream>
#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <fcntl.h>
#endif

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

void Wal::flush(){
    if(!log_stream.is_open()) return;
    log_stream.flush();

#if defined(__unix__) || defined(__APPLE__)
    int fd=open(wal_path.c_str(), O_WRONLY | O_APPEND);
    if(fd!=-1){
        fsync(fd);
        close(fd);
    }
#endif
}


bool Wal::write_record(WalOp op, const std::string &key, const std::string& value) {
    if(!log_stream.is_open()) return false;
    WalHeader header;
    header.op=static_cast<uint8_t>(op);
    header.key_len=static_cast<uint32_t>(key.size());
    header.val_len=(op==WalOp::PUT)?static_cast<uint32_t>(value.size()):0;
    log_stream.write(reinterpret_cast<const char*>(&header), sizeof(WalHeader));
    log_stream.write(key.data(), key.size());

    if(op==WalOp::PUT && !value.empty()){
        log_stream.write(value.data(), value.size());
    }
    flush();
    return log_stream.good();
}
bool Wal::log_put(const std::string& key, const std::string& value){
    return write_record(WalOp::PUT , key, value);
}

bool Wal::log_delete(const std::string&key){
    return write_record(WalOp::DELETE, key, "");
}


std::vector<WalRecord> Wal::recover(){
    std::vector<WalRecord> records;
    std::ifstream in_stream(wal_path, std::ios::in | std::ios::binary);
    if(!in_stream.is_open()){
        return records;
    }

    while(in_stream.peek()!=EOF){
        WalHeader header;
        //reads the WalHeader sized bytes into the header variable initialised above
        //&header is the address of the header variable, of type WalHeader* ptr, pointer arithmetic over this
        //mem add would be in terms of sizeof(WalHeader) bytes, so we cast it to char* to read the bytes correctly
        //to allow in_stream.read to read the bytes one by one, as char* conversion, makes the pointer arithmetic go one by one, since char is of size 1

        in_stream.read(reinterpret_cast<char*>(&header), sizeof(WalHeader));

        //gcount, is the successful read bytes, if it is less than the size of WalHeader, then we have an incomplete record, so we break the loop
        //streamsize is the signed int type returned by in_stream, sizeof just returns unsigned int type, hence the conversion.
        if(in_stream.gcount() < static_cast<std::streamsize>(sizeof(WalHeader))){
            break;
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


bool Wal::compact(const std::vector<std::pair<std::string, std::string>> & snapshot){
    std::string temp_path =wal_path + ".tmp";
    {
        std::ofstream truncate_stream(temp_path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!truncate_stream.is_open()) {
            std::cerr << "[WAL Error] Failed to create temporary WAL: " << temp_path << "\n";
            return false;
        }
    }
    {
        Wal temp_wal(temp_path);
        for(const auto & [key, value]:snapshot){
            if(!temp_wal.log_put(key, value)){
                std::cerr << "[WAL Error] Completion failed writing record to temp WAL.\n";
                return false;
            }
        }
    }

    if(log_stream.is_open()){
        log_stream.close();
    }
    std::error_code ec;
    std::filesystem::rename(temp_path, wal_path, ec);
    if(ec){
        std::cerr << "[WAL Error] Failed atomically renaming temp WAL: " << ec.message() << "\n";
        log_stream.open(wal_path, std::ios::out | std::ios::binary | std::ios::app);
        return false;
    }
    log_stream.open(wal_path, std::ios::out | std::ios::binary | std::ios::app);
    return log_stream.is_open();
}
