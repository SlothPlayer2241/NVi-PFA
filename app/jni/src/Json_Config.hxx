// app/jni/src/Json_Config.hxx
#pragma once

#include <string>
#include "Config_Utils.hxx"

/*
  Simple header declaring the JsonConfig helper implemented in Json_Config.cxx.
  This file provides declarations so other .cxx files (e.g. Gui.cxx) can call
  JsonConfig::SaveConfig/LoadConfig without causing undefined symbols at link time.
*/

class JsonConfig
{
public:
    // Save config to the given path. Returns true on success.
    static bool SaveConfig(const std::string& path, const NVConf::configuration& config);

    // Load config from the given path into 'config'. Returns true on success.
    static bool LoadConfig(const std::string& path, NVConf::configuration& config);

    // Return the default path to the config file on the current platform.
    static std::string GetDefaultConfigPath();

    // Test whether a config file exists at the given path.
    static bool ConfigExists(const std::string& path);
};
