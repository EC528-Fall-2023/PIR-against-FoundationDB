#include <iostream>
#include <chrono>
#include <cstring>
#include <array>
#include <sstream>
#include <iomanip>
#include <vector>

#include "multiclient.h"

void performPutBenchmark(MultiClient& client, const std::string& key, const std::string& data) {
    std::array<uint8_t, BYTES_PER_DATA> dataArray;
    dataArray.fill(0);

    std::memcpy(dataArray.data(), data.data(), std::min(data.size(), static_cast<size_t>(BYTES_PER_DATA)));    
    std::vector<double> putTimes;
    for (int i = 0; i < 10; ++i) {
        auto start = std::chrono::steady_clock::now();
        client.put(key, dataArray);
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> elapsedTime = end - start;
        putTimes.push_back(elapsedTime.count());
    }

    double averagePutTime = std::accumulate(putTimes.begin(), putTimes.end(), 0.0) / putTimes.size();

    std::cout << "PUT Operation - Key: " << std::setw(12) << key << " | Average Time Taken: "
              << averagePutTime << " seconds\n";
}

void performGetBenchmark(MultiClient& client, const std::string& key) {
    std::array<uint8_t, BYTES_PER_DATA> dataArray;
    dataArray.fill(0);
    std::vector<double> getTimes;
    for (int i = 0; i < 10; ++i) {
        auto start = std::chrono::steady_clock::now();
        client.get(key, dataArray);
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> elapsedTime = end - start;
        getTimes.push_back(elapsedTime.count());
    }

    double averageGetTime = std::accumulate(getTimes.begin(), getTimes.end(), 0.0) / getTimes.size();

    std::cout << "GET Operation - Key: " << std::setw(12) << key << " | Average Time Taken: "
              << averageGetTime << " seconds\n";
}

int main() {
    MultiClient client;
    client.initialize();

    std::vector<std::pair<std::string, std::string>> inputData = {
        {"small_data", "Dan is our mentor."},
        {"medium_data", "This is a moderate-sized text used for benchmarking purposes for private information retrieval project."},
        {"large_data", "In a world full of possibilities, each moment presents an opportunity for growth and discovery. Embracing challenges, individuals cultivate resilience and adaptability. Innovation thrives in environments that encourage experimentation and learning. Progress emerges from perseverance and collaboration. Together, we navigate complexities, shaping a future that reflects our aspirations and values."}
    };

    // Perform PUT operations for varying data sizes with unique keys
    for (const auto& data : inputData) {
        performPutBenchmark(client, data.first, data.second);
    }
// Perform GET operations for the keys after PUT operations
    for (const auto& data : inputData) {
        performGetBenchmark(client, data.first);
    }

    return 0;
}
