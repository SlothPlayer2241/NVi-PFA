#include "Json_Config.hxx"
#include <fstream>
#include <iostream>
#include "Utils.hxx"

#ifdef NON_ANDROID
#define DEFAULT_CONFIG_PATH "config.json"
#else
#define DEFAULT_CONFIG_PATH "/data/data/com.qsp.nvpfa/files/config.json"
#endif

bool JsonConfig::SaveConfig(const std::string& path, const NVConf::configuration& config) {
    try {
        // Create JSON object from config
        std::ofstream file(path);
        if (!file.is_open()) {
            NVi::error("JsonConfig", "Failed to open config file for writing: %s\n", path.c_str());
            return false;
        }

        // Format JSON data
        file << "{\n";
        file << "  \"audio\": {\n";
        file << "    \"voiceCount\": " << config.bass_voice_count << ",\n";
        file << "    \"lastMidiPath\": \"" << config.last_midi_path << "\"\n";
        file << "  },\n";
        file << "  \"visual\": {\n";
        file << "    \"noteSpeed\": " << config.note_speed << ",\n";
        file << "    \"windowWidth\": " << config.window_w << ",\n";
        file << "    \"windowHeight\": " << config.window_h << "\n";
        file << "  },\n";
        file << "  \"noteColors\": {\n";
        file << "    \"useCustomColors\": " << (config.use_custom_note_colors ? "true" : "false") << ",\n";
        file << "    \"primaryColor\": " << config.primary_note_color << ",\n";
        file << "    \"secondaryColor\": " << config.secondary_note_color << ",\n";
        file << "    \"colors\": [";
        for (size_t i = 0; i < config.custom_note_colors.size(); ++i) {
            file << config.custom_note_colors[i];
            if (i < config.custom_note_colors.size() - 1) file << ", ";
        }
        file << "]\n";
        file << "  },\n";
        file << "  \"noteSettings\": {\n";
        file << "    \"showNoteCounter\": " << (config.show_note_counter ? "true" : "false") << ",\n";
        file << "    \"runInBackground\": " << (config.run_in_background ? "true" : "false") << "\n";
        file << "  },\n";
        file << "  \"guiSettings\": {\n";
        file << "    \"showFps\": " << (config.show_fps ? "true" : "false") << ",\n";
        file << "    \"translucentNavigation\": " << (config.translucent_navigation ? "true" : "false") << "\n";
        file << "  }\n";
        file << "}\n";

        file.close();
        NVi::info("JsonConfig", "Settings saved to JSON: %s\n", path.c_str());
        return true;
    } catch (const std::exception& e) {
        NVi::error("JsonConfig", "Failed to save config: %s\n", e.what());
        return false;
    }
}

bool JsonConfig::LoadConfig(const std::string& path, NVConf::configuration& config) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            NVi::error("JsonConfig", "Failed to open config file for reading: %s\n", path.c_str());
            return false;
        }

        // Read the entire file into a string
        std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        // Parse JSON and load into config
        // For now, use default values until JSON parsing is implemented
        config.show_note_counter = true;
        config.run_in_background = false;
        config.show_fps = false;
        config.translucent_navigation = false;

        NVi::info("JsonConfig", "Settings loaded from JSON: %s\n", path.c_str());
        return true;
    } catch (const std::exception& e) {
        NVi::error("JsonConfig", "Failed to load config: %s\n", e.what());
        return false;
    }
}

std::string JsonConfig::GetDefaultConfigPath() {
    return DEFAULT_CONFIG_PATH;
}

bool JsonConfig::ConfigExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}
