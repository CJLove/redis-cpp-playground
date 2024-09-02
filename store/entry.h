#pragma once
#include <memory>
#include <unordered_map>
#include "flatdict.h"

namespace spdlog {
    class logger;
}

struct Value
{
    uint32_t m_value;
    bool m_flag1;
    bool m_flag2;

    Value() : m_value(0), m_flag1(false), m_flag2(false)
    {
    }

    Value(uint32_t value) : m_value(value), m_flag1(false), m_flag2(false)
    {
    }
};

using Fields = dict::Dict<Value,20>;

class Entry {
public:
    explicit Entry(const std::string &key);

    Entry(const std::string &key, std::unordered_map<std::string, std::string> &data);

    ~Entry() = default;

    // Copy construction and assignment
    Entry(const Entry &rhs) = default;
    Entry& operator=(const Entry &rhs) = default;

    void refresh(std::unordered_map<std::string, std::string> &data);

    Fields &getRecord1() {
        return m_record1;
    }

    const char *data(int idx);

    size_t size(int idx) const;

    void dump(std::shared_ptr<spdlog::logger> logger);

private:
    std::string m_key;
    Fields m_record1;
};