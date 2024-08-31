#pragma once

#include "Dispatcher.h"
#include "Worker.h"
#include <atomic>
#include <memory>
#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>
#include <thread>

class Scheduler {
public:
    Scheduler(std::shared_ptr<sw::redis::Redis> redis, const std::string &keyPrefix, bool dispatcher = true, bool worker = true);

    virtual ~Scheduler();

    void scheduleEvent(const std::string &eventId, uint32_t type, uint32_t interval);

private:
    std::shared_ptr<sw::redis::Redis> m_redis;

    std::shared_ptr<spdlog::logger> m_logger;

    std::string m_schedulerKey;

    std::string m_queueKey;

    std::atomic_bool m_running;

    std::unique_ptr<Dispatcher> m_dispatcher;

    std::unique_ptr<Worker> m_worker;
};