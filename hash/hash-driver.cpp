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

/**
 * @brief POD representation of a key/value pair 
 */
struct Field {
    uint32_t key;
    uint32_t value;
};

/**
 * @brief POD representation of a message with identifier, field count
 *        and key/value pairs
 */
struct Message {
    uint32_t id;
    uint32_t fieldCount;
    Field fields[200];

    Message(const char* data)
    {
        const uint32_t *ptr = reinterpret_cast<const uint32_t*>(data);
        id = ptr[0];
        fieldCount = ptr[1];
        for (uint32_t i = 0; i < fieldCount; i++) {
            fields[i].key = ptr[2+(i*2)];
            fields[i].value = ptr[3+(i*2)];
        }
    }

    Message(uint32_t id_, uint32_t count):
        id(id_), fieldCount(count)
    {
        for (uint32_t i = 0; i < count; i++) {
            fields[i].key = 3000+i;
            fields[i].value = 6000+i;
        }
    }

    size_t size() const
    {
        return 64 + fieldCount * sizeof(Field);
    }

    void dump(std::shared_ptr<spdlog::logger> logger) const
    {
        logger->info("Msg id {} count {} size {}", id, fieldCount, size());
    }
};

#pragma pack(pop)

void usage() {
    std::cerr << "Usage\n"
              << "hash-driver [-h <redisHost> ][-p <redisPort>][-e <redisAuthEnvVar>][-l <logLevel>]\n";

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
    auto logger = spdlog::stdout_logger_mt("hash");
    logger->set_pattern("%Y-%m-%d %H:%M:%S.%e|hash-driver|%t|%L|%v");
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

        //auto redis = Redis(options);
        auto redis = RedisCluster(options);

        // Set hash:1 field to a single string value
        auto key1 = "hash:1";

        redis.hset(key1,"field1","field1Val");
        logger->info("Logged single-value hash {}", key1);

        // Set hash:2 field with a pair of values
        auto key2 = "hash:2";
        std::unordered_map<std::string, std::string> v2 = {
            { "field1", "val1" },
            { "field2", "val2" }
        };
        redis.hmset(key2,v2.begin(), v2.end());
        logger->info("Logged multi-value hash {}", key2);

        // Retrieve all keys for hash:2
        std::unordered_map<std::string, std::string> fields;
        redis.hgetall(key2, std::inserter(fields, fields.end()));
        logger->info("Retrieved multi-value hash values for {}", key2);
        for (const auto &field: fields) {
            logger->info("  {}: {}", field.first, field.second);
        }

        // 
        Message m1(3200, 15);
        m1.dump(logger);
        Message m2(3500, 20);
        m2.dump(logger);

        std::unordered_map<std::string, std::string_view> hashFields = {
            { "msg1", { reinterpret_cast<const char*>(&m1),m1.size() } },
            { "msg2", { reinterpret_cast<const char*>(&m2),m2.size() } }
        };
        logger->info("hashFields[msg1].size() {}", hashFields["msg1"].size());
        logger->info("hashFields[msg1].size() {}", hashFields["msg2"].size());
        auto key3 = "hash:3";
        redis.hmset(key3,hashFields.begin(), hashFields.end());
        logger->info("Stored multi-value hash values for {}", key3);

        std::unordered_map<std::string, std::string> fields2;
        redis.hgetall(key3, std::inserter(fields2, fields2.end()));
        logger->info("Retrieved multi-value hash values for {}", key3);
        logger->info("hashFields[msg1].size() {}", fields2["msg1"].size());
        logger->info("hashFields[msg1].size() {}", fields2["msg2"].size());

        for (const auto &field: fields2) {
            Message msg(field.second.data());
            msg.dump(logger);
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
