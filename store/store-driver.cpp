#include <sw/redis++/redis++.h>
#include <fmt/core.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <getopt.h>
#include <iostream>
#include <unordered_map>
#include <initializer_list>
#include "store.h"

using namespace sw::redis;

std::vector<std::string> split(const std::string &str, const char delim)
{
    std::vector<std::string> strings;
    std::istringstream stream(str);
    std::string s;
    while (std::getline(stream, s, delim)) {
        strings.push_back(s);
    }
    return strings;
}


void usage() {
    std::cerr << "Usage\n"
              << "hash-driver [-h <redisHost> ][-p <redisPort>][-e <redisAuthEnvVar>][-l <logLevel>]\n";

}

int main(int argc, char**argv)
{
    int logLevel = spdlog::level::trace;
    std::string redisHost ("127.0.0.1");
    uint16_t redisPort = 6379;
    bool doAuth = false;
    std::string redisAuthEnvVar ("REDIS_PASSWORD");
    std::vector<uint16_t> sentinelPorts;
    int conSize = 5;
    int c;

    while ((c = getopt(argc,argv, "h:p:e:l:c:s:?")) != EOF) {
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
                logLevel = std::stoi(optarg);
                break;
            default:
                usage();
                exit(1);
        }
    }
    auto logger = spdlog::stdout_logger_mt("store");
    logger->set_pattern("%Y-%m-%d %H:%M:%S.%e|store-driver|%t|%L|%v");
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

        Store store(redis,logger);

        while (true) {
            std::string line;
            std::cout << "Command >";
            std::getline(std::cin, line);

            auto words = split(line, ' ');
            if (words[0] == "quit") {
                break;
            } else if (words[0] == "entry" ) {
                std::string key = words[1];
                Entry entry(key);
                for (int x = 1; x < 10; x++) {
                    entry.getRecord1().insert(x*100, x);
                }
                entry.dump(logger);

                store.storeEntry(key, entry);
            } else if (words[0] == "get") {
                std::string key = words[1];
                Entry *entryPtr = nullptr;
                if (store.queryEntry(words[1],entryPtr)) {
                    entryPtr->dump(logger);
                }
            } else if (words[0] == "del") {
                std::string key = words[1];
                store.deleteEntry(key);
            } else if (words[0] == "mod") {
                std::string key = words[1];
                Entry *entryPtr = nullptr;
                if (store.queryEntry(words[1],entryPtr)) {
                    uint32_t mult = 0;
                    auto &record = entryPtr->getRecord1();
                    if (!record.contains(999)) {
                        record.insert(999,2);
                    }
                    mult = record.at(999).m_value + 1;
                    record.set(999,mult);
                    for (int x = 1; x < 10; x++) {
                        entryPtr->getRecord1().set(x*100,x*mult);
                    }
                    store.storeEntry(words[1],*entryPtr);
                }
            }

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
