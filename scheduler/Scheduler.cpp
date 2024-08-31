#include "Scheduler.h"


Scheduler::Scheduler(std::shared_ptr<sw::redis::Redis> redis, const std::string &keyPrefix, bool dispatcher, bool worker):
    m_redis(redis),
    m_logger(spdlog::get("scheduler")),
    m_schedulerKey(keyPrefix+"Zset"),
    m_queueKey(keyPrefix+"Queue"),
    m_running(false),
    m_dispatcher(nullptr),
    m_worker(nullptr)
{
    if (dispatcher) {
        m_dispatcher = std::make_unique<Dispatcher>(redis, keyPrefix);
    }
    if (worker) {
        m_worker = std::make_unique<Worker>(redis, keyPrefix);
    }
}

Scheduler::~Scheduler()
{
    m_running = false;
}

void Scheduler::scheduleEvent(const std::string &eventId, uint32_t , uint32_t interval)
{
    time_t now = time(nullptr);
    now += interval;
    double score = static_cast<double>(now + interval);

    try {
        m_logger->info("Now {} Scheduling event {} for time {}", now, eventId, score);
        m_redis->zadd(m_schedulerKey, eventId, score);
    }
    catch (std::exception &e) {
        m_logger->error("scheduleEvent() caught {}", e.what());
    }
}


