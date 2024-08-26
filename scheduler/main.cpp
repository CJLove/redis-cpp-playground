#include <sw/redis++/redis++.h>
#include <fmt/core.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <getopt.h>
#include <iostream>
#include <unordered_map>
#include <initializer_list>
#include "Scheduler.h"
#include <signal.h>
#include <atomic>

using namespace sw::redis;

std::atomic_bool running = true;

void usage() {
    std::cerr << "Usage\n"
              << "scheduler [-h <redisHost> ][-p <redisPort>][-e <redisAuthEnvVar>][-n <name> ][-l <logLevel>][-d][-w]\n";

}

static void sig_int(int )
{
    std::cout << "Got SIGINT\n";
    running = false;
}



int main(int argc, char**argv)
{
    int logLevel = spdlog::level::trace;
    std::string redisHost ("127.0.0.1");
    uint16_t redisPort = 6379;
    std::string redisAuthEnvVar ("REDIS_PASSWORD");
    bool doAuth = false;
    std::string name = "client1";
    bool dispatcher = false;
    bool worker = false;
    int c;

    while ((c = getopt(argc,argv, "h:p:e:l:n:dw?")) != EOF) {
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
            case 'n':
                name = optarg;
                break;
            case 'd':
                dispatcher = true;
                break;
            case 'w':
                worker = true;
                break;
            case 'l':
                logLevel = std::stoi(optarg);
                break;
            default:
                usage();
                exit(1);
        }
    }
    auto logger = spdlog::stdout_logger_mt("scheduler");
    // Update logging pattern to reflect the client name
    auto pattern = fmt::format("%Y-%m-%d %H:%M:%S.%e|{}|%t|%L|%v", name);
    logger->set_pattern(pattern);

    // Set the log level for filtering
    spdlog::set_level(static_cast<spdlog::level::level_enum>(logLevel));

    if (::signal(SIGINT, sig_int) == SIG_ERR) {
        logger->error("Unable to register signal handler");
        return 1;
    }

    try {
        ConnectionOptions options;
        options.host = redisHost;
        options.port = redisPort;
        if (doAuth) {
            auto passwd = getenv(redisAuthEnvVar.c_str());
            if (!passwd) {
                logger->error("Unable to get auth password");
                exit(1);
            }
            options.password = passwd;
        }
        ConnectionPoolOptions poolOptions;
        poolOptions.size = 3;

        //auto redis = Redis(options, poolOptions);
        auto redis = RedisCluster(options, poolOptions);

        Scheduler scheduler(redis,"scheduler",dispatcher,worker);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        uint32_t eventCount = 0; 

        for (eventCount = 0; eventCount < 10; eventCount++) {

            auto eventId = fmt::format("{}-event-{}",name,eventCount);
            scheduler.scheduleEvent(eventId,0,10);

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        logger->info("Waiting for SIGINT");

        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        logger->info("Exiting main() scope");


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
