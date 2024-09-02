#include <memory>
#include <sw/redis++/redis++.h>
#include "entry.h"

#pragma once

using namespace sw::redis;

namespace spdlog {
    class logger;
}

class Store {
public:
    Store(std::shared_ptr<Redis> redis, std::shared_ptr<spdlog::logger> logger);

    ~Store();

    void storeEntry(const std::string &key, Entry &entry);

    void deleteEntry(const std::string &key);

    bool queryEntry(const std::string &key, Entry *&entry);

private:
    std::shared_ptr<Redis> m_redis;
    std::shared_ptr<spdlog::logger> m_logger;
    std::map<std::string, Entry*> m_entries;

};