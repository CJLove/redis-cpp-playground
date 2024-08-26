#include "Dispatcher.h"


Dispatcher::Dispatcher(sw::redis::RedisCluster &redis, const std::string &keyPrefix):
    m_redis(redis),
    m_logger(spdlog::get("scheduler")),
    m_schedulerKey(keyPrefix+"Zset{sched}"),
    m_queueKey(keyPrefix+"Queue{sched}"),
    m_running(false),
    m_dispatcher(std::thread(&Dispatcher::run, this))
{

}

Dispatcher::~Dispatcher()
{
    m_running = false;
    if (m_dispatcher.joinable()) {
        m_dispatcher.join();
    }
}

void
Dispatcher::run()
{
    m_running = true;
    m_logger->info("Starting Dispatcher thread");

    auto tx = m_redis.transaction(m_schedulerKey);
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
            m_logger->error("Dispatcher exception {}", err.what());
        }
    }
    m_logger->info("Exiting Dispatcher thread");
}