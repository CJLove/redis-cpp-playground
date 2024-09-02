#include "entry.h"
#include <spdlog/spdlog.h>

Entry::Entry(const std::string &key): m_key(key)
{

}

Entry::Entry(const std::string &key, std::unordered_map<std::string, std::string> &data): m_key(key), 
    m_record1(data["record1"].data(), data["record1"].size())
{

}

void Entry::refresh(std::unordered_map<std::string, std::string> &data)
{
    m_record1.refresh(data["record1"].data(), data["record1"].size());
}



const char *Entry::data(int idx) {
    switch(idx) {
    case 0:
        return m_record1.data();
    default:
        return nullptr;
    }
}

size_t Entry::size(int idx) const {
    switch(idx) {
    case 0:
        return m_record1.container_size();
    default:
        return 0;
    }
}

void Entry::dump(std::shared_ptr<spdlog::logger> logger)
{
    logger->info("Entry Key: {}", m_key);
    for (const auto &elt: m_record1)
    {
        auto v = m_record1.at(elt);
        logger->info("  {}: {}", elt.m_key, v.m_value);
    }
}

