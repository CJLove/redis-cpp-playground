#include "Scheduler.h"


Scheduler::Scheduler(sw::redis::Redis &redis, const std::string &keyPrefix):
    m_redis(redis),
    m_logger(spdlog::get("scheduler")),
    m_schedulerKey(keyPrefix+"Zset"),
    m_queueKey(keyPrefix+"Queue"),
    m_running(false),
    m_dispatcher(std::thread(&Scheduler::Dispatcher, this)),
    m_worker(std::thread(&Scheduler::Worker, this))
{

}

Scheduler::~Scheduler()
{
    m_running = false;
    if (m_dispatcher.joinable()) {
        m_dispatcher.join();
    }
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

void Scheduler::scheduleEvent(const std::string &eventId, uint32_t , uint32_t interval)
{
    time_t now = time(nullptr);
    now += interval;
    double score = static_cast<double>(now + interval);

    m_logger->info("Now {} Scheduling event {} for time {}", now, eventId, score);
    m_redis.zadd(m_schedulerKey, eventId, score);
}

void
Scheduler::Dispatcher()
{
    m_running = true;
    m_logger->info("Starting Scheduler thread");

    auto tx = m_redis.transaction();
    auto txRedis = tx.redis();

    while (m_running.load())
    {
        try {
            txRedis.watch(m_schedulerKey);

            double minScore = 0;
            double maxScore = static_cast<double>(time(nullptr));
            std::vector<std::pair<std::string, double>> zset_result;
            txRedis.zrangebyscore(m_schedulerKey, sw::redis::BoundedInterval<double>(minScore, maxScore, sw::redis::BoundType::CLOSED), std::back_inserter(zset_result));

            if (!zset_result.empty()) {
                auto replies = tx.rpush(m_queueKey, zset_result[0].first)
                                 .zrem(m_schedulerKey, zset_result[0].first)
                                 .exec();
                //m_logger->info("Dispatcher() moving elt {} to queue", zset_result[0].first);
                // Expect both steps of transaction to return 1
                //m_logger->info("Transaction {} {}", replies.get<long long>(0), replies.get<long long>(1));
            }
        } catch (const sw::redis::WatchError &err) {
            continue;
        } catch (const sw::redis::Error &err) {
            m_logger->error("Exception {}", err.what());
        }
    }
    m_logger->info("Exiting Scheduler thread");
}

void
Scheduler::Worker()
{
    m_logger->info("Starting Worker thread");

    while (m_running.load()) {
        auto event = m_redis.blpop(m_queueKey, std::chrono::seconds(2));
        if (event) {
            time_t now = time(nullptr);
            m_logger->info("Now {} worker thread handling event {}", now, event->second);
        }

    }
    m_logger->info("Exiting Worker thread");

}

