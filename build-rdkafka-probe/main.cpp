#include <librdkafka/rdkafka.h>
#include <iostream>

int main() {
    std::cout << "librdkafka version: "
              << rd_kafka_version_str()
              << std::endl;

    std::cout << "RDKAFKA C API: PASS"
              << std::endl;

    return 0;
}