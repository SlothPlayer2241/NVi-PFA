// Author: 0xsys (20.04.2025)
#pragma once


//#include "Gui.hxx"
#include <vector>
#include <string>
#include <stdint.h>

//#define NON_ANDROID

#ifdef NON_ANDROID
    #define CONFIG_PATH "config.toml"
    #define MIDI_LIST "midi_list.toml"
    #define SF_LIST "soundfonts.toml"
    #define DEFAULT_SOUNDFONT "piano_maganda.sf2"
    #define DEFAULT_GM_SOUNDFONT "gm.sf2"
    #define DEFAULT_MIDI "pfa_intro.mid"
#else
    #define CONFIG_PATH "/data/data/com.qsp.nvpfa/files/config.toml"
    #define MIDI_LIST "/data/data/com.qsp.nvpfa/files/midi_list.toml"
    #define SF_LIST "/data/data/com.qsp.nvpfa/files/soundfonts.toml"
    #define DEFAULT_SOUNDFONT "/data/data/com.qsp.nvpfa/files/piano_maganda.sf2"
    #define DEFAULT_GM_SOUNDFONT "/data/data/com.qsp.nvpfa/files/gm.sf2"
    #define DEFAULT_MIDI "/data/data/com.qsp.nvpfa/files/pfa_intro.mid"
#endif




struct SoundfontItem 
{
    std::string label;
    bool checked = false;
};



class NVConf
{
    public:
    typedef struct
    {
        int bass_voice_count;
        int note_speed;
        int window_w; // Custom width and height for the SDL window
        int window_h;
        int bg_R;
        int bg_G;
        int bg_B;
        int bg_A;
        std::string last_midi_path;
        std::vector<std::string> current_soundfonts;
        std::vector<int> custom_note_colors; // Custom colors for notes (RGB values)
        bool use_custom_note_colors; // Enable/disable custom note colors
        std::string background_image_path; // Path to background image
        bool use_background_image; // Enable/disable background image
        int key_range_start; // Starting MIDI note number
        int key_range_end; // Ending MIDI note number
        bool use_custom_key_range; // Enable/disable custom key range
        bool enable_file_logging; // Enable/disable logging to file
        std::vector<std::string> custom_midi_directories; // Custom directories to search for MIDI files
        std::vector<std::string> custom_soundfont_directories; // Custom directories to search for soundfont files
        bool use_default_gm_soundfont; // Enable/disable default GM soundfont
        std::string default_gm_soundfont_path; // Path to default GM soundfont
        int realtime_frame_rate; // Target frame rate for realtime processing
        float frame_fluctuation_threshold; // Threshold for frame fluctuation detection
        bool enable_audio_limiter; // Enable/disable audio limiter
        float limiter_threshold; // Audio limiter threshold
        float gui_scale; // Global GUI font scale
        int primary_note_color; // RGB integer
        int secondary_note_color; // RGB integer
            bool show_note_counter; // Enable/disable note counter
            bool run_in_background; // Enable/disable running in background
            bool show_fps; // Enable/disable FPS display
            bool translucent_navigation; // Enable/disable translucent navigation bar
    }configuration;
    
    static configuration ReadConfig();
    static void WriteConfig(configuration cfg);
    //static void ReadMidiLists();
    
    static void CreateSoundfontList(std::vector<SoundfontItem> lst);
    static std::vector<SoundfontItem> ReadSoundfontList();
    
    static std::string GetDefaultGMSoundfontPath();
    
    //static void CreateMidiFileList(std::vector<std::string> lst);
    //static std::vector<std::string> ReadMidiList();
    
};
