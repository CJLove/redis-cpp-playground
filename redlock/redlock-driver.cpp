#include <sw/redis++/redis++.h>
#include <sw/redis++/patterns/redlock.h>
#include <fmt/core.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <getopt.h>
#include <iostream>
#include <unordered_map>
#include <initializer_list>

using namespace sw::redis;

void usage() {
    std::cerr << "Usage\n"
              << "redlock-driver [-h <redisHost> ][-p <redisPort>][-e <redisAuthEnvVar>][-l <logLevel>]\n";

}

void autoExtendCallback(std::exception_ptr ptr) {
    try {
        std::rethrow_exception(ptr);
    } catch (const Error &e) {
        std::cout << "Caught redis-plus-plus exception " << e.what() << "\n";
    } catch (std::exception &e) {
        std::cout << "Caught std::exception " << e.what() << "\n";
    }
}

int main(int argc, char**argv)
{
    int logLevel = spdlog::level::trace;
    std::string redisHost ("127.0.0.1");
    uint16_t redisPort = 6379;
    bool doAuth = false;
    std::string redisAuthEnvVar ("REDIS_PASSWORD");
    std::vector<uint16_t> sentinelPorts;
    bool doLock = false;
    bool doTry = false;
    bool doGuard = false;
    int conSize = 5;
    int c;

    while ((c = getopt(argc,argv, "h:p:e:ltgc:s:?")) != EOF) {
        switch (c) {
            case 'h':
                redisHost = optarg;
                break;
            case 'p':
                redisPort = static_cast<uint16_t>(std::stoi(optarg));
                break;
            case 'e':
                redisAuthEnvVar = optarg;
                doAuth = true;
                break;
            case 's':
                sentinelPorts.push_back(static_cast<uint16_t>(std::stoi(optarg)));
                break;
            case 'c':
                conSize = static_cast<int>(std::stoi(optarg));
                break;
            case 'l':
                doLock = true;
                break;
            case 't':
                doTry = true;
                break;
            case 'g':
                doGuard = true;
                break;
            default:
                usage();
                exit(1);
        }
    }
    auto logger = spdlog::stdout_logger_mt("hash");
    logger->set_pattern("%Y-%m-%d %H:%M:%S.%e|hash-driver|%t|%L|%v");
    // Set the log level for filtering
    spdlog::set_level(static_cast<spdlog::level::level_enum>(logLevel));

    try {
        SentinelOptions sentinelOptions;
        ConnectionOptions options;
        std::shared_ptr<Sentinel> sentinel;
        std::shared_ptr<Redis> redis;
        ConnectionPoolOptions poolOptions;
        poolOptions.size = conSize;
        if (doAuth) {
            auto passwd = getenv(redisAuthEnvVar.c_str());
            if (!passwd) {
                logger->error("Unable to get auth password");
                exit(1);
            }
            options.password = passwd;
        }

        if (sentinelPorts.size()) {
            // Connect to Sentinel
            for (const auto &port: sentinelPorts) {
                sentinelOptions.nodes.push_back({redisHost, port});
            }
            sentinelOptions.connect_timeout = std::chrono::milliseconds(500);
            sentinelOptions.socket_timeout = std::chrono::milliseconds(500);

            sentinel = std::make_shared<Sentinel>(sentinelOptions);


            options.connect_timeout = std::chrono::milliseconds(500);
            options.socket_timeout = std::chrono::milliseconds(500);

            redis = std::make_shared<Redis>(sentinel, "mymaster", Role::MASTER, options, poolOptions);
        } else {
            // Connect to Redis master
            options.host = redisHost;
            options.port = redisPort;

            redis = std::make_shared<Redis>(options, poolOptions);
        }

        RedMutexOptions opts;
        opts.ttl = std::chrono::seconds(5);

        auto watcher = std::make_shared<LockWatcher>();

        RedMutex mtx({ redis }, "mutex", autoExtendCallback, opts, watcher);

        if (doLock) {
            logger->info("Locking mutex");
            mtx.lock();
            logger->info("Locked");
            std::this_thread::sleep_for(std::chrono::seconds(30));
            logger->info("Unlocking mutex");
            mtx.unlock();
            logger->info("Unlocked");    
        }

        if (doTry) {
            logger->info("Trying to lock mutex");
            auto locked = mtx.try_lock();
            if (locked) {
                logger->info("Locked");
                std::this_thread::sleep_for(std::chrono::seconds(30));
                logger->info("Unlocking mutex");
                mtx.unlock();
                logger->info("Unlocked");                  
            } else {
                logger->info("Unable to lock mutex");
            }
        }

        if (doGuard) {
            logger->info("Locking mutex via guard");
            {
                std::lock_guard<RedMutex> guard(mtx);
                logger->info("Locked");
                std::this_thread::sleep_for(std::chrono::seconds(30));
                logger->info("Unlocking mutex");
            }
            logger->info("Unlocked");
        }   

    }
    catch (Error &e)
    {
        logger->error("Caught redis-plus-plus error {}", e.what());
    }
    catch (std::exception &e)
    {
        logger->error("Caught std::exception {}", e.what());
    }

}
