#include <iostream>
#include <unistd.h>
#include <cstdint>
#include <wiringPiI2C.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#define HTU21D_ADDRESS 0x40
#define READ_TEMP_CMD 0xF3
#define READ_HUMIDITY_CMD 0xF5
#define SERIAL_NUMBER "78e7f5626b3905b939c1"

constexpr int updateInterval = 2000;
const std::string serverUrl = "https://airsense.yooud.org/api/sensor";

using json = nlohmann::json;

float readTemperature(int fd) {
    wiringPiI2CWrite(fd, READ_TEMP_CMD);
    usleep(50000);
    uint8_t data[3];
    if (const int bytesRead = read(fd, data, 3); bytesRead != 3) {
        std::cerr << "Помилка зчитування температури!" << std::endl;
        return -1.0;
    }
    const uint16_t rawTemp = (data[0] << 8) | data[1];
    return -46.85 + (175.72 * rawTemp / 65536.0);
}

float readHumidity(int fd) {
    wiringPiI2CWrite(fd, READ_HUMIDITY_CMD);
    usleep(50000);
    uint8_t data[3];
    if (const int bytesRead = read(fd, data, 3); bytesRead != 3) {
        std::cerr << "Помилка зчитування вологості!" << std::endl;
        return -1.0;
    }
    const uint16_t rawHumidity = (data[0] << 8) | data[1];
    float humidity = -6.0 + (125.0 * rawHumidity / 65536.0);
    if (humidity < 0) humidity = 0;
    if (humidity > 100) humidity = 100;
    return humidity;
}

void sendHttpData(const std::string& parameter, double value) {
    if (CURL* curl = curl_easy_init()) {
        json jsonData = {
            {"parameter", parameter},
            {"value", value}
        };

        const std::string jsonString = jsonData.dump();

        curl_easy_setopt(curl, CURLOPT_URL, serverUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        headers = curl_slist_append(headers, ("X-Serial-Number: " + std::string(SERIAL_NUMBER)).c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "HTTP request error: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Error initializing CURL" << std::endl;
    }
}


int main() {
    const int fd = wiringPiI2CSetup(HTU21D_ADDRESS);
    if (fd == -1) {
        std::cerr << "Не вдалося підключитися до I2C!" << std::endl;
        return 1;
    }

    while (true) {
        if (const float temperature = readTemperature(fd); temperature != -1.0) {
            sendHttpData("temperature", temperature);
            std::cout << temperature << "°C ";
        }

        if (const float humidity = readHumidity(fd); humidity != -1.0) {
            sendHttpData("humidity", humidity);
            std::cout << humidity << "% ";
        }

        std::cout << std::endl;
        usleep(updateInterval * 1000);
    }
}
