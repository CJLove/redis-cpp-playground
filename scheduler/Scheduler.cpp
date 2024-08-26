#include "Scheduler.h"


Scheduler::Scheduler(sw::redis::RedisCluster &redis, const std::string &keyPrefix, bool dispatcher, bool worker):
    m_redis(redis),
    m_logger(spdlog::get("scheduler")),
    m_schedulerKey(keyPrefix+"Zset{sched}"),
    m_queueKey(keyPrefix+"Queue{sched}"),
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

    m_logger->info("Now {} Scheduling event {} for time {}", now, eventId, score);
    m_redis.zadd(m_schedulerKey, eventId, score);
}


