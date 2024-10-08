#include "Worker.h"

Worker::Worker(std::shared_ptr<sw::redis::Redis> redis, const std::string &keyPrefix):
    m_redis(redis),
    m_logger(spdlog::get("scheduler")),
    m_schedulerKey(keyPrefix+"Zset"),
    m_queueKey(keyPrefix+"Queue"),
    m_running(false),
    m_worker(std::thread(&Worker::run, this))
{

}

Worker::~Worker()
{
    m_running = false;
    if (m_worker.joinable()) {
        m_worker.join();
    }   
}

void Worker::run()
{
    m_logger->info("Starting Worker thread");
    m_running = true;

    while (m_running.load()) {
        try {
            auto event = m_redis->blpop(m_queueKey, std::chrono::seconds(2));
            if (event) {
                time_t now = time(nullptr);
                m_logger->info("Now {} worker thread handling event {}", now, event->second);
            }
        }
        catch (sw::redis::TimeoutError &e) {
            continue;
        }
        catch (std::exception &e) {
            m_logger->error("Worker::run() caught exeception {}", e.what());
        }
    }
    m_logger->info("Exiting Worker thread");
}