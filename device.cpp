#include <iostream>
#include <unistd.h>
#include <cstdint>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <wiringPi.h>
#include <softPwm.h>

#define SERIAL_NUMBER "0a61df1888991de8a0be"
#define PWM_PIN 23

constexpr int updateInterval = 2000;
const std::string serverUrl = "https://airsense.yooud.org/api/device";

using json = nlohmann::json;

size_t writeCallback(void* contents, const size_t size, const size_t nmemb, void* userp) {
    const auto responseJson = static_cast<json*>(userp);
    const size_t totalSize = size * nmemb;
    std::string response(static_cast<char*>(contents), totalSize);

    try {
        *responseJson = json::parse(response);
    } catch (const json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }

    return totalSize;
}

void getPwmInstruction() {
    if (CURL* curl = curl_easy_init()) {
        json responseJson;

        curl_easy_setopt(curl, CURLOPT_URL, serverUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseJson);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, ("X-Serial-Number: " + std::string(SERIAL_NUMBER)).c_str());

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
            std::cerr << "HTTP request error: " << curl_easy_strerror(res) << std::endl;
        } else {
            try {
                if (responseJson.is_null() || !responseJson.contains("fan_speed")) {
                    std::cerr << "Response is empty or missing 'fan_speed' field." << std::endl;
                } else {
                    if (const int fanSpeed = responseJson["fan_speed"].get<int>(); fanSpeed >= 0 && fanSpeed <= 100) {
                        softPwmWrite(PWM_PIN, fanSpeed);
                        std::cout << "Fan speed is set: " << fanSpeed << std::endl;
                    } else {
                        std::cerr << "Incorrect fan_speed value!" << std::endl;
                    }
                }
            } catch (const json::exception& e) {
                std::cerr << "JSON processing error: " << e.what() << std::endl;
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Error initializing CURL" << std::endl;
    }
}

int main() {
    if (wiringPiSetup() == -1) {
        std::cerr << "Error initializing WiringPi!" << std::endl;
        return 1;
    }

    if (softPwmCreate(PWM_PIN, 0, 100) != 0) {
        std::cerr << "Software PWM creation error!" << std::endl;
        return 1;
    }

    while (true) {
        getPwmInstruction();
        usleep(updateInterval * 1000);
    }
}
