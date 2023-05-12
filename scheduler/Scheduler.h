#pragma once

#include <atomic>
#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>
#include <thread>

class Scheduler {
public:
    Scheduler(sw::redis::Redis &redis, const std::string &schedulerKey);

    virtual ~Scheduler();

    void scheduleEvent(const std::string &eventId, uint32_t type, uint32_t interval);

private:
    void Dispatcher();

    void Worker();

    sw::redis::Redis &m_redis;

    std::shared_ptr<spdlog::logger> m_logger;

    std::string m_schedulerKey;

    std::string m_queueKey;

    std::atomic_bool m_running;

    std::thread m_dispatcher;

    std::thread m_worker;
};