#pragma once

#include <atomic>
#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>
#include <thread>


class Dispatcher {
public: 
    Dispatcher(std::shared_ptr<sw::redis::Redis> redis, const std::string &keyPrefix);

    virtual ~Dispatcher();

private:
    void run();
    
    std::shared_ptr<sw::redis::Redis> m_redis;

    std::shared_ptr<spdlog::logger> m_logger;

    std::string m_schedulerKey;

    std::string m_queueKey;

    std::atomic_bool m_running;

    std::thread m_dispatcher;
};