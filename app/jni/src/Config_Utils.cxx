#include <limits> // Required by cpptoml
#include <cstdint>
#include <unistd.h>
#include "Utils.hxx"
#include "extern/cpptoml/cpptoml.h"


//#define NON_ANDROID

//#include "Gui.hxx"
#include "Config_Utils.hxx"

//#include <iostream>




/*
Changed my mind bree
I won't use json anymore >:D
*/

void NVConf::WriteConfig(configuration cfg)
{
    auto out_cfg = cpptoml::make_table();
    
    auto audio = cpptoml::make_table();
    audio->insert("VoiceCount", cfg.bass_voice_count);
    audio->insert("LastMIDIpath", cfg.last_midi_path);
    out_cfg->insert("Audio", audio);
    
    auto vis = cpptoml::make_table();
    vis->insert("NoteSpeed", cfg.note_speed);
    vis->insert("Window_w", cfg.window_w);
    vis->insert("Window_h", cfg.window_h);
    out_cfg->insert("Visual", vis);
    
    auto bg_col = cpptoml::make_table();
    bg_col->insert("R", cfg.bg_R);
    bg_col->insert("G", cfg.bg_G);
    bg_col->insert("B", cfg.bg_B);
    bg_col->insert("A", cfg.bg_A);
    out_cfg->insert("BackgroundColor", bg_col);
    
    auto note_colors = cpptoml::make_table();
    note_colors->insert("UseCustomColors", cfg.use_custom_note_colors);
    auto color_array = cpptoml::make_array();
    for (const auto& color : cfg.custom_note_colors) {
        color_array->push_back(color);
    }
    note_colors->insert("Colors", color_array);
    note_colors->insert("PrimaryColor", cfg.primary_note_color);
    note_colors->insert("SecondaryColor", cfg.secondary_note_color);
    out_cfg->insert("NoteColors", note_colors);
    
    auto bg_image = cpptoml::make_table();
    bg_image->insert("UseBackgroundImage", cfg.use_background_image);
    bg_image->insert("ImagePath", cfg.background_image_path);
    out_cfg->insert("BackgroundImage", bg_image);
    
    auto key_range = cpptoml::make_table();
    key_range->insert("UseCustomKeyRange", cfg.use_custom_key_range);
    key_range->insert("KeyRangeStart", cfg.key_range_start);
    key_range->insert("KeyRangeEnd", cfg.key_range_end);
    out_cfg->insert("KeyRange", key_range);
    
    auto logging = cpptoml::make_table();
    logging->insert("EnableFileLogging", cfg.enable_file_logging);
    out_cfg->insert("Logging", logging);
    
    auto midi_dirs = cpptoml::make_array();
    for (const auto& dir : cfg.custom_midi_directories) {
        midi_dirs->push_back(dir);
    }
    out_cfg->insert("CustomMidiDirectories", midi_dirs);
    
    auto sf_dirs = cpptoml::make_array();
    for (const auto& dir : cfg.custom_soundfont_directories) {
        sf_dirs->push_back(dir);
    }
    out_cfg->insert("CustomSoundfontDirectories", sf_dirs);
    
    auto gm_soundfont = cpptoml::make_table();
    gm_soundfont->insert("UseDefaultGM", cfg.use_default_gm_soundfont);
    gm_soundfont->insert("GMSoundfontPath", cfg.default_gm_soundfont_path);
    out_cfg->insert("DefaultGMSoundfont", gm_soundfont);
    
    auto audio_settings = cpptoml::make_table();
    audio_settings->insert("RealtimeFrameRate", cfg.realtime_frame_rate);
    audio_settings->insert("FrameFluctuationThreshold", cfg.frame_fluctuation_threshold);
    audio_settings->insert("EnableAudioLimiter", cfg.enable_audio_limiter);
    audio_settings->insert("LimiterThreshold", cfg.limiter_threshold);
    audio_settings->insert("GuiScale", cfg.gui_scale);
    out_cfg->insert("AudioSettings", audio_settings);

    auto note_settings = cpptoml::make_table();
    note_settings->insert("ShowNoteCounter", cfg.show_note_counter);
    note_settings->insert("RunInBackground", cfg.run_in_background);
    out_cfg->insert("NoteSettings", note_settings);
    
    std::ofstream out(CONFIG_PATH);
    out << (*out_cfg); // cpptoml overloads the << operator
    out.close();
    NVi::info("Config_Utils", "Settings saved\n");
}

NVConf::configuration NVConf::ReadConfig()
{
    NVConf::configuration in_cfg;
    auto cfg = cpptoml::parse_file(CONFIG_PATH);
    
    auto audio = cfg->get_table("Audio");
    in_cfg.bass_voice_count = *audio->get_as<int>("VoiceCount");
    in_cfg.last_midi_path = *audio->get_as<std::string>("LastMIDIpath");
    
    auto vis = cfg->get_table("Visual");
    in_cfg.window_w = *vis->get_as<int>("Window_w");
    in_cfg.window_h = *vis->get_as<int>("Window_h");
    in_cfg.note_speed = *vis->get_as<int>("NoteSpeed");
    
    auto bg_col = cfg->get_table("BackgroundColor");
    in_cfg.bg_R = *bg_col->get_as<int>("R");
    in_cfg.bg_G = *bg_col->get_as<int>("G");
    in_cfg.bg_B = *bg_col->get_as<int>("B");
    in_cfg.bg_A = *bg_col->get_as<int>("A");
    
    auto note_colors = cfg->get_table("NoteColors");
    if (note_colors) {
        in_cfg.use_custom_note_colors = *note_colors->get_as<bool>("UseCustomColors");
        // cpptoml may represent integer arrays as 64-bit; read as int64_t and convert
        auto colors = note_colors->get_array_of<int64_t>("Colors");
        if (colors) {
            in_cfg.custom_note_colors.clear();
            for (const auto &v : *colors) {
                in_cfg.custom_note_colors.push_back(static_cast<int>(v));
            }
        }
        // Primary and secondary color fallbacks
        if (note_colors->get_as<int>("PrimaryColor"))
            in_cfg.primary_note_color = *note_colors->get_as<int>("PrimaryColor");
        else
            in_cfg.primary_note_color = in_cfg.custom_note_colors.empty() ? 0xFFFFFF : (in_cfg.custom_note_colors[0] & 0xFFFFFF);

        if (note_colors->get_as<int>("SecondaryColor"))
            in_cfg.secondary_note_color = *note_colors->get_as<int>("SecondaryColor");
        else
            in_cfg.secondary_note_color = (in_cfg.custom_note_colors.size() > 1) ? in_cfg.custom_note_colors[1] : 0xFF7E33;
    } else {
        in_cfg.use_custom_note_colors = false;
        in_cfg.custom_note_colors.clear();
        in_cfg.primary_note_color = 0xFFFFFF;
        in_cfg.secondary_note_color = 0xFF7E33;
    }
    
    auto bg_image = cfg->get_table("BackgroundImage");
    if (bg_image) {
        in_cfg.use_background_image = *bg_image->get_as<bool>("UseBackgroundImage");
        in_cfg.background_image_path = *bg_image->get_as<std::string>("ImagePath");
    } else {
        in_cfg.use_background_image = false;
        in_cfg.background_image_path = "";
    }
    
    auto key_range = cfg->get_table("KeyRange");
    if (key_range) {
        in_cfg.use_custom_key_range = *key_range->get_as<bool>("UseCustomKeyRange");
        in_cfg.key_range_start = *key_range->get_as<int>("KeyRangeStart");
        in_cfg.key_range_end = *key_range->get_as<int>("KeyRangeEnd");
    } else {
        in_cfg.use_custom_key_range = false;
        in_cfg.key_range_start = 0;
        in_cfg.key_range_end = 127;
    }
    
    auto logging = cfg->get_table("Logging");
    if (logging) {
        in_cfg.enable_file_logging = *logging->get_as<bool>("EnableFileLogging");
    } else {
        in_cfg.enable_file_logging = false;
    }
    
    auto midi_dirs = cfg->get_array_of<std::string>("CustomMidiDirectories");
    if (midi_dirs) {
        in_cfg.custom_midi_directories = *midi_dirs;
    } else {
        in_cfg.custom_midi_directories.clear();
    }
    
    auto sf_dirs = cfg->get_array_of<std::string>("CustomSoundfontDirectories");
    if (sf_dirs) {
        in_cfg.custom_soundfont_directories = *sf_dirs;
    } else {
        in_cfg.custom_soundfont_directories.clear();
    }
    
    auto gm_soundfont = cfg->get_table("DefaultGMSoundfont");
    if (gm_soundfont) {
        in_cfg.use_default_gm_soundfont = *gm_soundfont->get_as<bool>("UseDefaultGM");
        in_cfg.default_gm_soundfont_path = *gm_soundfont->get_as<std::string>("GMSoundfontPath");
    } else {
        in_cfg.use_default_gm_soundfont = false;
        in_cfg.default_gm_soundfont_path = DEFAULT_GM_SOUNDFONT;
    }
    
    auto audio_settings = cfg->get_table("AudioSettings");
    if (audio_settings) {
        in_cfg.realtime_frame_rate = *audio_settings->get_as<int>("RealtimeFrameRate");
        in_cfg.frame_fluctuation_threshold = *audio_settings->get_as<double>("FrameFluctuationThreshold");
        in_cfg.enable_audio_limiter = *audio_settings->get_as<bool>("EnableAudioLimiter");
        in_cfg.limiter_threshold = *audio_settings->get_as<double>("LimiterThreshold");
        if (audio_settings->get_as<double>("GuiScale"))
            in_cfg.gui_scale = *audio_settings->get_as<double>("GuiScale");
        else
            in_cfg.gui_scale = 1.0f;
    } else {
        in_cfg.realtime_frame_rate = 60;
        in_cfg.frame_fluctuation_threshold = 0.1f;
        in_cfg.enable_audio_limiter = true;
        in_cfg.limiter_threshold = 0.95f;
        in_cfg.gui_scale = 1.0f;
    }

    // Initialize GUI settings that aren't in config yet
    in_cfg.show_fps = false;
    in_cfg.translucent_navigation = false;

    return in_cfg;
}


void NVConf::CreateSoundfontList(std::vector<SoundfontItem> lst)
{
    auto out_sf_list= cpptoml::make_table();
    
    auto sf_path = cpptoml::make_array();
    for (const auto& path : lst) 
    {
        sf_path->push_back(path.label);
    }
    
    auto enabled = cpptoml::make_array();
    for(const auto& en : lst)
    {
        enabled->push_back(en.checked);
    }

    out_sf_list->insert("paths", sf_path);
    out_sf_list->insert("enabled_disabled", enabled);
    
    std::ofstream out(SF_LIST);
    if (out.is_open()) 
    {
        out << (*out_sf_list);
    } 
    else 
    {
        NVi::error("Config_Utils", "Failed to write 'soundfonts.toml'\n");
    }
}

std::vector<SoundfontItem> NVConf::ReadSoundfontList()
{
    std::vector<SoundfontItem> lst;
    auto in_sf_list = cpptoml::parse_file(SF_LIST);
    
    auto paths = in_sf_list->get_array_of<std::string>("paths");
    auto enabled = in_sf_list->get_array_of<bool>("enabled_disabled");
    
    if (!paths || !enabled || paths->size() != enabled->size()) 
    {
        NVi::error("Config_Utils", "Invalid or mismatched arrays in 'soundfonts.toml'\n");
        return lst;
    }
    
    for (size_t i = 0; i < paths->size(); ++i) 
    {
        lst.push_back(SoundfontItem{(*paths)[i], (*enabled)[i]});
    }
    return lst;
}

std::string NVConf::GetDefaultGMSoundfontPath()
{
    return DEFAULT_GM_SOUNDFONT;
}
