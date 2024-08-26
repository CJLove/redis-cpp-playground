#include <sw/redis++/redis++.h>
#include <fmt/core.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <getopt.h>
#include <iostream>
#include <unordered_map>
#include <initializer_list>

using namespace sw::redis;

#pragma pack(push,1)


#pragma pack(pop)

void usage() {
    std::cerr << "Usage\n"
              << "zset-driver [-h <redisHost> ][-p <redisPort>][-e <redisAuthEnvVar>][-l <logLevel>]\n";

}

void dumpZset(const char *prefix, std::shared_ptr<spdlog::logger> logger, std::vector<std::pair<std::string, double>> &zset)
{
    logger->info("{}", prefix);
    for (const auto &elt: zset) {
        logger->info("  elt {} score {}", elt.first, elt.second);
    }
}

int main(int argc, char**argv)
{
    int logLevel = spdlog::level::trace;
    std::string redisHost ("127.0.0.1");
    uint16_t redisPort = 6379;
    std::string redisAuthEnvVar ("REDIS_PASSWORD");
    bool doAuth = false;
    int c;

    while ((c = getopt(argc,argv, "h:p:e:l:?")) != EOF) {
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
            case 'l':
                logLevel = std::stoi(optarg);
                break;
            default:
                usage();
                exit(1);
        }
    }
    auto logger = spdlog::stdout_logger_mt("sset");
    logger->set_pattern("%Y-%m-%d %H:%M:%S.%e|sset-driver|%t|%L|%v");
    // Set the log level for filtering
    spdlog::set_level(static_cast<spdlog::level::level_enum>(logLevel));

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

//        auto redis = Redis(options);
        auto redis = RedisCluster(options);

        std::map<std::string, double> s = {
            { "evt1", 1 },
            { "evt2", 3 },
            { "evt3", 5 },
            { "evt4", 5 },
            { "evt5", 9 },
        };

        redis.zadd("zset:1", "evt0", 0 );

        auto count = redis.zadd("zset:1", s.begin(), s.end());

        logger->info("zadd() added {} elements", count);

        auto rank = redis.zrank("zset:1", "evt2");
        if (rank) {
           logger->info("zrank(evt2) = {}",*rank);
        }
        auto score = redis.zscore("zset:1", "evt2");
        if (score) {
            logger->info("zscore(evt2) = {}", *score);
        }

        std::vector<std::pair<std::string, double>> zset_result;
        redis.zrangebyscore("zset:1", UnboundedInterval<double>{}, std::back_inserter(zset_result));
        dumpZset("Unbounded range", logger,zset_result);

        zset_result.clear();

        redis.zrangebyscore("zset:1", BoundedInterval<double>(5.0, 6, BoundType::RIGHT_OPEN), std::back_inserter(zset_result));
        dumpZset("Bounded range", logger, zset_result);




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
