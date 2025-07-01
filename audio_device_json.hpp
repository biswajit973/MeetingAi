// nlohmann/json single-header library is required: https://github.com/nlohmann/json
#include "lohmann/json.hpp"
#include <fstream>
#include <vector>
#include <string>

struct AudioDevice {
    int index;
    std::string name;
};

std::vector<AudioDevice> LoadAudioDevicesFromJson(const std::string& filename = "audio_devices.json") {
    std::vector<AudioDevice> devices;
    std::ifstream in(filename);
    if (!in) {
        // Try one level up (project root)
        std::ifstream in2("../" + filename);
        if (!in2) {
            printf("Could not open %s or ../%s\n", filename.c_str(), filename.c_str());
            return devices;
        }
        nlohmann::json j;
        in2 >> j;
        for (const auto& item : j) {
            devices.push_back({item["index"].get<int>(), item["name"].get<std::string>()});
        }
        printf("Loaded %zu devices from ../%s\n", devices.size(), filename.c_str());
        for (const auto& d : devices) {
            printf("Device: %d %s\n", d.index, d.name.c_str());
        }
        return devices;
    }
    nlohmann::json j;
    in >> j;
    for (const auto& item : j) {
        devices.push_back({item["index"].get<int>(), item["name"].get<std::string>()});
    }
    // Debug print
    printf("Loaded %zu devices from %s\n", devices.size(), filename.c_str());
    for (const auto& d : devices) {
        printf("Device: %d %s\n", d.index, d.name.c_str());
    }
    return devices;
}
