#include <cstdio>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <sstream>
#include <fstream>
#include <vector>

#ifdef __linux__
#include <unistd.h>
#endif

#include "extern/audio/bassmidi.h"
#include "extern/imgui_sdl3/imgui_impl_sdl3.h"
#include "extern/cpptoml/cpptoml.h"

//#define NON_ANDROID
//#define DEV_TEST

#include "common.hxx"
#include "audio_effects.hxx"
#include "Utils.hxx"
#include "file_utils.hxx"
#include "Config_Utils.hxx"
#include "MIDI.hxx"
#include "Sequ.hxx"
#include "Nlist.hxx"
#include "Gui.hxx"
#include "canvas.hxx"

#ifdef DEV_TEST
#include "dev_test.h"
#endif

#if defined(_WIN32) || defined(_WIN64)
#define BASSMIDI_LIB "bassmidi.dll"
#else
#define BASSMIDI_LIB "libbassmidi.so"
#endif

std::vector<BASS_MIDI_FONT> font_list;
QWORD saved_position = 0;
const double SEEK_AMOUNT = 3.0;
bool is_paused = false;

unsigned char events[6144];
int eventCount = 0;
NVnoteList nv_list;

const int frameRate = 60;
const int frameDelay = 1000 / frameRate;

Uint32 frameStart;
int frameTime;
SDL_Event Evt;
NVmidiFile M;
NVsequencer S;
NVi::u32_t nowtick;

SDL_Mutex *bass_mutex = nullptr;

// ----------------- DEBUG LOG FUNCTION -----------------
void LogAudio(const char* msg)
{
    std::ostringstream os;
    os << "[Audio Debug] " << msg << "\n";
    SDL_Log("%s", os.str().c_str());
}
// ------------------------------------------------------

void LoadInitialSoundfonts()
{
    std::vector<BASS_MIDI_FONT> fontSet;

    if(checked_soundfonts.size() == 0)
    {
#ifndef NON_ANDROID
        std::string default_sf_path = NVFileUtils::GetFilePathA("piano_maganda.sf2", "rb");
        std::string default_gm_sf_path = NVFileUtils::GetFilePathA("gm_generic.sf2", "rb");

        HSOUNDFONT Sf1 = BASS_MIDI_FontInit(default_gm_sf_path.c_str(), 0);
        HSOUNDFONT Sf2 = BASS_MIDI_FontInit(default_sf_path.c_str(), 0);

        if(!Sf1) LogAudio(("Failed to load soundfont: " + default_gm_sf_path).c_str());
        if(!Sf2) LogAudio(("Failed to load soundfont: " + default_sf_path).c_str());

        fontSet.push_back({Sf2, -1, 0});
        fontSet.push_back({Sf1, -1, 0});

        BASS_MIDI_FontSetVolume(Sf1, 0.15);
        BASS_MIDI_FontSetVolume(Sf2, 0.15);
#else
        HSOUNDFONT Sf1 = BASS_MIDI_FontInit(DEFAULT_GM_SOUNDFONT, 0);
        HSOUNDFONT Sf2 = BASS_MIDI_FontInit(DEFAULT_SOUNDFONT, 0);

        if(!Sf1) LogAudio("Failed to load DEFAULT_GM_SOUNDFONT");
        if(!Sf2) LogAudio("Failed to load DEFAULT_SOUNDFONT");

        fontSet.push_back({Sf2, -1, 0});
        fontSet.push_back({Sf1, -1, 0});

        BASS_MIDI_FontSetVolume(Sf1, 0.15);
        BASS_MIDI_FontSetVolume(Sf2, 0.15);
#endif
    }
    else
    {
        fontSet.reserve(checked_soundfonts.size());
        for(int i = 0; i < checked_soundfonts.size(); i++)
        {
            HSOUNDFONT Sf = BASS_MIDI_FontInit(checked_soundfonts[i].c_str(), 0);
            if(!Sf) LogAudio(("Failed to load soundfont: " + checked_soundfonts[i]).c_str());
            else
            {
                BASS_MIDI_FontSetVolume(Sf, 0.15);
                fontSet.push_back({Sf, -1, 0});
            }

            std::ostringstream sf_file_info_text;
            NVFileUtils::FileInfo sf_file = NVFileUtils::GetFileInfo(checked_soundfonts[i]);
            sf_file_info_text_arr.clear();
            sf_file_info_text << "File Name: " << sf_file.file_name << "\n";
            sf_file_info_text << "Last Modified: " << sf_file.last_mod << "\n";
            sf_file_info_text << "Size: " << sf_file.size << "\n";
            sf_file_info_text_arr.push_back(sf_file_info_text.str());
        }
    }

    if(!fontSet.empty())
    {
        if(!Stm) LogAudio("Warning: Stream Stm is null when setting fonts.");
        else BASS_MIDI_StreamSetFonts(Stm, fontSet.data(), fontSet.size());
    }
}

void reloadSoundfonts()
{
    if (!Stm || !BASS_ChannelIsActive(Stm))
    {
        LogAudio("reloadSoundfonts called but stream is inactive");
        return;
    }

    QWORD position = BASS_ChannelGetPosition(Stm, BASS_POS_BYTE);
    bool was_playing = !is_paused;
    std::string current_midi = parsed_config.last_midi_path;

    BASS_ChannelStop(Stm);
    BASS_StreamFree(Stm);

    Stm = BASS_StreamCreateFile(0, current_midi.c_str(), 0, 0, BASS_SAMPLE_FLOAT);
    if(!Stm) LogAudio(("Failed to recreate MIDI stream: " + current_midi).c_str());
    else LogAudio(("Successfully recreated MIDI stream: " + current_midi).c_str());

    BASS_ChannelSetDSP(Stm, &dsp_limiter, 0, 0);

    LoadInitialSoundfonts();
    BASS_ChannelSetAttribute(Stm, BASS_ATTRIB_MIDI_VOICES, parsed_config.bass_voice_count);
    BASS_MIDI_StreamSetFilter(Stm, 0, reinterpret_cast<BOOL (*)(HSTREAM, int, BASS_MIDI_EVENT *, BOOL, void *)>(filter), nullptr);

    BASS_ChannelSetPosition(Stm, position, BASS_POS_BYTE);

    if(was_playing)
    {
        BASS_ChannelPlay(Stm, FALSE);
        is_paused = false;
    }
    else
    {
        is_paused = true;
    }

    Tplay = BASS_ChannelBytes2Seconds(Stm, position);

    NVConf::CreateSoundfontList(live_soundfont_list);
    LogAudio("Soundfonts reloaded");
}

void updateBassVoiceCount(int voiceCount)
{
    if(Stm && BASS_ChannelIsActive(Stm))
    {
        BASS_ChannelSetAttribute(Stm, BASS_ATTRIB_MIDI_VOICES, voiceCount);
        parsed_config.bass_voice_count = voiceCount;
        std::ostringstream log_msg;
        log_msg << "Voice count updated to " << voiceCount;
        LogAudio(log_msg.str().c_str());
    }
}

void loadMidiFile(const std::string& midi_path)
{
    SDL_LockMutex(bass_mutex);

    if(main_gui_window)
        main_gui_window = false;

    if(!MIDI.start_parse(midi_path.c_str()))
    {
        std::ostringstream temp_msg;
        temp_msg << "Failed to load '" << midi_path << "'";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "MIDI Loading Error", temp_msg.str().c_str(), nullptr);
        LogAudio(("Failed to parse MIDI file: " + midi_path).c_str());
        SDL_UnlockMutex(bass_mutex);
        return;
    }

    Stm = BASS_StreamCreateFile(0, midi_path.c_str(), 0, 0, BASS_SAMPLE_FLOAT);
    if(!Stm) LogAudio(("Failed to create MIDI stream: " + midi_path).c_str());
    else LogAudio(("Successfully created MIDI stream: " + midi_path).c_str());

    BASS_ChannelSetDSP(Stm, &dsp_limiter, 0, 0);
    LoadInitialSoundfonts();
    BASS_ChannelSetAttribute(Stm, BASS_ATTRIB_MIDI_VOICES, parsed_config.bass_voice_count);

    if(live_conf.vel_filter)
        BASS_MIDI_StreamSetFilter(Stm, 0, reinterpret_cast<BOOL (*)(HSTREAM, int, BASS_MIDI_EVENT *, BOOL, void *)>(filter), nullptr);

    if(!BASS_ChannelPlay(Stm, 1))
        LogAudio("Warning: Failed to start MIDI playback");

    Tplay = 0.0;
    is_paused = false;
    playback_ended = false;
    parsed_config.last_midi_path = midi_path;

    SDL_UnlockMutex(bass_mutex);
}

// ----------------- THE REST OF THE FILE REMAINS THE SAME -----------------
// All other functions like toggle_pause(), seek_playback(), NVi::CloseMIDI(),
// NVi::CreateMidiList(), NVi::RefreshSFList(), NVi::CreateImageList(), etc.
// are unchanged, but you can optionally add LogAudio(...) in them for debugging.

#ifdef NON_ANDROID
int main(int ac, char **av)
#else
int SDL_main(int ac, char **av)
#endif
{
    bass_mutex = SDL_CreateMutex();

    CvWin = new Canvas;

    LogAudio("Canvas created, initializing audio");

    BASS_PluginLoad(BASSMIDI_LIB, 0);
    BASS_SetConfig(BASS_CONFIG_BUFFER, 5000);
    BASS_SetConfig(BASS_CONFIG_MIDI_AUTOFONT, 0);

    if(!BASS_Init(live_conf.audio_device_index, 44100, 0, 0, nullptr))
    {
        int errorCode = BASS_ErrorGetCode();
        std::ostringstream msg;
        msg << "BASS_Init failed: " << errorCode;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Bass Init Error", msg.str().c_str(), NULL);
        LogAudio(msg.str().c_str());
    }
    else LogAudio("BASS successfully initialized");

    // Load last MIDI file
    loadMidiFile(parsed_config.last_midi_path);

    // Continue the main loop as in original code
    // ...
    return 0;
}
