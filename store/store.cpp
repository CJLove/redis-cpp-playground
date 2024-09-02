#include <spdlog/spdlog.h>
#include "store.h"

Store::Store(std::shared_ptr<sw::redis::Redis> redis, std::shared_ptr<spdlog::logger> logger): m_redis(redis), m_logger(logger)
{

}

Store::~Store()
{

}

void Store::storeEntry(const std::string &key, Entry &entry)
{
    try {

        std::unordered_map<std::string, StringView> records {
            { "record1", { entry.data(0), entry.size(0) } }
        };

        m_redis->hmset(key, records.begin(), records.end());
        
        if (m_entries.count(key) == 0) {
            m_entries[key] = new Entry(entry);
        } else {
            *m_entries[key] = entry;
        }

        m_logger->info("Stored entry {}", key);

    } catch (const sw::redis::Error &e) {
        m_logger->error("Caught Redis exception {}", e.what());
    } catch (std::exception &e) {
        m_logger->error("Caught std::exception {}", e.what());
    }
}

void Store::deleteEntry(const std::string &key)
{
    try {
        m_logger->info("Removed entry {}", key);
        m_redis->unlink(key);

    } catch (const sw::redis::Error &e) {
        m_logger->error("Caught Redis exception {}", e.what());
    } catch (std::exception &e) {
        m_logger->error("Caught std::exception {}", e.what());
    }

}

bool Store::queryEntry(const std::string &key, Entry *&entry)
{
    if (m_entries.count(key)) {
        // TODO: Refresh existing entry from Redis
        try {
            std::unordered_map<std::string, std::string> fields;
            m_redis->hgetall(key, std::inserter(fields, fields.end()));
            m_entries.at(key)->refresh(fields);
            entry = m_entries[key];
            

        } catch (const sw::redis::Error &e) {
            m_logger->error("Caught Redis exception {}", e.what());
        } catch (std::exception &e) {
            m_logger->error("Caught std::exception {}", e.what());
        }
        return true;
    }
    try {
        std::unordered_map<std::string, std::string> fields;
        m_redis->hgetall(key, std::inserter(fields, fields.end()));
        m_entries[key] = new Entry(key, fields);
        entry = m_entries[key];
        return true;
    } catch (const sw::redis::Error &e) {
        m_logger->error("Caught Redis exception {}", e.what());
    } catch (std::exception &e) {
        m_logger->error("Caught std::exception {}", e.what());
    }
    return false;
}
