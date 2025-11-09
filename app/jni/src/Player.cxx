#include <cstdio>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
//#include <android/log.h>
//#include <bassmidi.h>
//#include <bass_fx.h>

#include "extern/audio/bassmidi.h"
//#include "extern/audio/bass_fx.h"

#include <iostream>


//#define NON_ANDROID // Temporary defined in here
//#define DEV_TEST // Use this for development purposes only




#include "Utils.hxx"
#include "file_utils.hxx"
#include "Config_Utils.hxx"
#include "MIDI.hxx"
#include "Sequ.hxx"
#include "Nlist.hxx"
#include "Gui.hxx"
#include "canvas.hxx"

#include "extern/imgui_sdl3/imgui_impl_sdl3.h"
#include "extern/cpptoml/cpptoml.h"






NVmidiFile M;
NVsequencer S;
NVi::u32_t nowtick;
double Tread;
Canvas cv;
unsigned char events[6144];
int eventCount = 0;

// Runtime request to change the currently playing MIDI file
static std::string requested_midi_path = "";
// Runtime request to change soundfont
static std::string requested_soundfont_path = "";

void NVi::RequestMidiChange(const std::string& path)
{
    requested_midi_path = path;
}

void NVi::RequestSoundfontChange(const std::string& path)
{
    requested_soundfont_path = path;
}


//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);


// Canvas expects colors in low-to-high byte order: RR in lowest byte, GG next, BB next, AA top
// (i.e. 0xAABBGGRR). Convert our human-friendly AARRGGBB constants into that ordering here.
static constexpr unsigned int Col[]= {
    0xFFFF0000, // blue  (was 0xFF0000FF in AARRGGBB)
    0xFF337EFF, // orange(was 0xFFFF7E33)
    0xFFFF0000,
    0xFF337EFF,
    0xFFFF0000,
    0xFF337EFF,
    0xFFFF0000,
    0xFF337EFF,
    0xFFFF0000,
    0xFF337EFF,
    0xFFFF0000,
    0xFF337EFF,
    0xFFFF0000,
    0xFF337EFF,
    0xFFFF0000,
    0xFF337EFF
};


SDL_Event Evt;
HSTREAM   Stm;



Uint64 lastTapTime = 0;
float lastTapX = 0, lastTapY = 0;


float limiter_threshold = 0.8f;
float limiter_knee = 0.05f;
float limiter_attack = 0.1f;   // attack speed (0 = instant, 1 = never)
// Missing globals used by the limiter DSP: current gain state and release smoothing
float current_gain = 1.0f;     // start with unity gain
float limiter_release = 0.01f; // release smoothing factor (small => slow release)
// Master gain to reduce perceived loudness on mobile devices (0.0 - 1.0)
float master_gain = 0.60f; // default to 60% volume
    // (removed duplicate palette) palette is defined above as blue/orange alternating
void bassevt(DWORD type, DWORD param, DWORD chan, DWORD tick, DWORD time)
{
    BASS_MIDI_EVENT evt = {type,param,chan,tick,time};
    BASS_MIDI_StreamEvents(Stm,BASS_MIDI_EVENTS_STRUCT|BASS_MIDI_EVENTS_NORSTATUS|BASS_MIDI_EVENTS_CANCEL,&evt,1);
}


void CALLBACK dsp_limiter(HDSP handle, DWORD channel, void *buffer, DWORD length, void *user)
{
    float *samples = (float*)buffer;
    DWORD count = length / sizeof(float);
   
    for (DWORD i = 0; i < count; ++i)
    {
        float input = samples[i];
        float abs_input = fabs(input);
   
        float desired_gain = 1.0f;
   
        if (abs_input > limiter_threshold)
        {
            float exceed = abs_input - limiter_threshold;
            float compressed = exceed / (exceed + limiter_knee);
            desired_gain = limiter_threshold / (limiter_threshold + compressed);
        }
   
        // Smoothly approach desired gain
        if (desired_gain < current_gain)
            current_gain += (desired_gain - current_gain) * limiter_attack; // attack smoothing
        else
               current_gain += (desired_gain - current_gain) * limiter_release; // release smoothing
   
           samples[i] *= current_gain;
    }
}


int    _WinH;
//static int ns = live_note_speed;
// 20000 maximum
// 100 minimum
double Tplay = 0.0, Tscr;

// Flag to indicate we're playing the intro default midi on startup
bool is_intro_mode = false;

static void DrawNote(NVi::u16_t k, const NVnote &n, int pps)
{
    unsigned int c = Col[(n.track%16+n.chn)%16];
    int key = KeyMap[k], y_1;
    int y_0 = floor(_WinH - (n.Tstart - Tplay) * pps+0.5f);

    if (y_0 < 0)
    {
        y_0 = 0;
    }
    else
    {
        if (y_0 > _WinH)
        {
            y_0 = _WinH;
        }
    }

    if (n.Tend < Tplay + Tscr)
    {
        y_1 = floor(_WinH - (n.Tend - Tplay) * pps+0.5f);

        if (y_1 < 0)
        {
            y_1 = 0;
        }
        else
        {
            if (y_1 > _WinH)
            {
                y_1 = _WinH;
            }
        }
    }
    else
    {
        y_1 = 0;
    }

    if (n.Tstart <= Tplay && Tplay < n.Tend)
    {
        // Mark key as active and set alpha to full so it will fade out smoothly
        Win->KeyAlpha[key] = 1.0f;

        // Count this note only once when it becomes active
        if (!Win->isNoteCounted(key)) {
            Win->incrementNoteCount();
            Win->setNoteCounted(key, true);
        }

        // If intro mode is active, override color based on left/right side
        if (is_intro_mode) {
            // Choose left side blue, right side orange. Use k (note index) to determine side.
            // Using AABBGGRR ordering to match canvas expectations
            auto convert_rgb_to_renderer = [](unsigned int rgb) -> unsigned int {
                rgb &= 0xFFFFFFu;
                unsigned int r = (rgb >> 16) & 0xFF;
                unsigned int g = (rgb >> 8) & 0xFF;
                unsigned int b = rgb & 0xFF;
                return 0xFF000000u | (b << 16) | (g << 8) | (r << 0); // AABBGGRR
            };

            unsigned int left = convert_rgb_to_renderer(static_cast<unsigned int>(parsed_config.primary_note_color));
            unsigned int right = convert_rgb_to_renderer(static_cast<unsigned int>(parsed_config.secondary_note_color));
            if (k < 60) {
                c = left;
            } else {
                c = right;
            }
        }
        Win->KeyColor[key] = c;
    }

    // If this note has ended (y_1 > 0 and n.Tend <= Tplay), ensure the key alpha is cleared immediately
    // so the visual disappears when the note finishes rather than fading out on the keyboard contact.
    if (n.Tend <= Tplay) {
        Win->KeyAlpha[key] = 0.0f;
        // Mark a one-frame hit so canvas can draw an immediate key flash right when the note reaches the keyboard
        Win->KeyJustHit[key] = true;
        // When the note ends, clear the counted flag so the same key can be counted again later
        if (Win->isNoteCounted(key)) {
            Win->setNoteCounted(key, false);
        }
    }

    Win->Note(k, y_0, y_1, c);
}

BOOL CALLBACK filter(HSTREAM S, DWORD trk, BASS_MIDI_EVENT *E, BOOL sk, void *u)
{
    if (E->event == MIDI_EVENT_NOTE)
    {
        int vel = HIBYTE(E->param);
        return vel == 0 || vel > 9;
    }

    return TRUE;
}

// Add these with your other global variables
bool is_paused = false;
QWORD saved_position = 0; // For storing position when paused
const double SEEK_AMOUNT = 3.0; // 3 seconds for seeking

// Function to handle seeking
void seek_playback(double seconds) {
    QWORD current_byte_pos = BASS_ChannelGetPosition(Stm, BASS_POS_BYTE);
    double current_time = BASS_ChannelBytes2Seconds(Stm, current_byte_pos);
    double new_time = current_time + seconds;
    
    // Ensure we don't seek before the beginning
    if (new_time < 0) new_time = 0;
    
    // Convert back to bytes and set position
    QWORD new_pos = BASS_ChannelSeconds2Bytes(Stm, new_time);
    BASS_ChannelSetPosition(Stm, new_pos, BASS_POS_BYTE);
    
    // Update our playback time
    Tplay = BASS_ChannelBytes2Seconds(Stm, new_pos);
    
    // After any seek we must resync the sequencer's note lists so visuals match audio.
    // Clear the note lists then reposition and update them for the new Tplay.
    for (int i = 0; i < 128; ++i) {
        MIDI.L[i].clear();
    }
    MIDI.list_seek(Tplay);
    MIDI.update_to(Tplay + Tscr);
}

// Function to toggle pause/play
void toggle_pause() {
    if (is_paused) {
        // Resume playback
        BASS_ChannelPlay(Stm, false); // false means don't restart from beginning
        is_paused = false;
    } else {
        // Pause playback
        saved_position = BASS_ChannelGetPosition(Stm, BASS_POS_BYTE);
        BASS_ChannelPause(Stm);
        is_paused = true;
    }
}

// Native functions called from Java for pause/resume on app lifecycle
#ifndef NON_ANDROID
// Android version with JNI
#include <jni.h>

extern "C" {
    JNIEXPORT void JNICALL Java_com_qsp_nvpfa_NvpfaActivity_pauseMidiPlayback(JNIEnv* env, jobject obj) {
        if (!is_paused && Stm) {
            saved_position = BASS_ChannelGetPosition(Stm, BASS_POS_BYTE);
            BASS_ChannelPause(Stm);
            is_paused = true;
            NVi::info("Player", "MIDI playback paused (app backgrounded)");
        }
    }

    JNIEXPORT void JNICALL Java_com_qsp_nvpfa_NvpfaActivity_resumeMidiPlayback(JNIEnv* env, jobject obj) {
        if (is_paused && Stm) {
            BASS_ChannelPlay(Stm, false); // false means don't restart from beginning
            is_paused = false;
            NVi::info("Player", "MIDI playback resumed (app foregrounded)");
        }
    }
}
#endif

void NVi::Quit()
{
    // Causes runtime errors
    //ImGui_ImplSDLRenderer3_Shutdown();
    //ImGui_ImplSDL3_Shutdown();
    //ImGui::DestroyContext();
    delete Win;
    BASS_Free();
    BASS_PluginFree(0);
    MIDI.destroy_all();
    // On Android, don't call exit() as it crashes the app. Instead, let the function return gracefully.
    // The main loop will exit naturally when the stream ends.
}





void NVi::CreateMidiList()
{
    // Insert all midi and soundfont file paths
    //live_midi_list = NVFileUtils::GetFilesByExtension(BASE_DIRECTORY,".mid");
    
    std::vector<std::string> all_midi_files;
    
    //All possibilities
    auto mid_files       = NVFileUtils::GetFilesByExtension(BASE_DIRECTORY, ".mid");
    auto midi_files      = NVFileUtils::GetFilesByExtension(BASE_DIRECTORY, ".midi");
    auto smf_files       = NVFileUtils::GetFilesByExtension(BASE_DIRECTORY, ".smf");
    auto mid_caps_files  = NVFileUtils::GetFilesByExtension(BASE_DIRECTORY, ".MID");
    auto midi_caps_files = NVFileUtils::GetFilesByExtension(BASE_DIRECTORY, ".MIDI");
    auto smf_caps_files  = NVFileUtils::GetFilesByExtension(BASE_DIRECTORY, ".SMF");
    
    all_midi_files.insert(all_midi_files.end(), mid_files.begin(), mid_files.end());
    all_midi_files.insert(all_midi_files.end(), midi_files.begin(), midi_files.end());
    all_midi_files.insert(all_midi_files.end(), smf_files.begin(), smf_files.end());
    all_midi_files.insert(all_midi_files.end(), mid_caps_files.begin(), mid_caps_files.end());
    all_midi_files.insert(all_midi_files.end(), midi_caps_files.begin(), midi_caps_files.end());
    all_midi_files.insert(all_midi_files.end(), smf_caps_files.begin(), smf_caps_files.end());
    
    live_midi_list.resize(all_midi_files.size());
    
    for(int i = 0; i < live_midi_list.size(); i++)
    {
        live_midi_list[i] = all_midi_files[i];
        //std::cout << "Midi List: " << live_midi_list[i] << "\n";
    }
    
    
    auto root = cpptoml::make_table();
    
    auto midi_files_out = cpptoml::make_array();
    
    for (const auto& path : live_midi_list) 
    {
        midi_files_out->push_back(path);
    }
    
    root->insert("midi_files", midi_files_out);
    
    // Save list
    std::ofstream out(MIDI_LIST);
    if (out.is_open()) 
    {
        out << (*root);
        std::cout << "TOML written to output.toml\n";
    } 
    else 
    {
        Canvas C;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error!!!!!", "Failed to save midi list", nullptr);
    }
}

void NVi::ReadMidiList()
{
    try
    {
        auto config = cpptoml::parse_file(MIDI_LIST);

        if (auto array = config->get_array_of<std::string>("midi_files"))
        {
            live_midi_list = *array;
        }
        else
        {
            NVi::error("Player", "'midi_files' array not found or invalid");
            // On Android, don't crash - log error and continue with empty list
            live_midi_list.clear();
        }
    } // Try to catch parsing errros
    catch (const cpptoml::parse_exception& e)
    {
        std::ostringstream msg;
        msg << "TOML Parse Error: " << e.what();
        NVi::error("Player", "TOML Parse error: %s", e.what());
        // On Android, don't crash - log error and continue with empty list
        live_midi_list.clear();
    }
}


void NVi::RefreshSFList()
{
    live_soundfont_files = NVFileUtils::GetFilesByExtension(BASE_DIRECTORY, ".sf2");
    auto sfz_files = NVFileUtils::GetFilesByExtension(BASE_DIRECTORY, ".sfz");
    live_soundfont_files.insert(live_soundfont_files.end(), sfz_files.begin(), sfz_files.end());
    live_soundfont_list.resize(live_soundfont_files.size());
    
    for(size_t i = 0; i < live_soundfont_files.size(); i++)
    {
        live_soundfont_list[i] = {live_soundfont_files[i], false};
    }
    NVConf::CreateSoundfontList(live_soundfont_list);
}

//Maybe this will be eventually used sometime
/*
void NVi::StartBassPlayback(std::string midi_path, std::vector<std::string> soundfonts)
{
    //std::cout << "playback\n";
    Stm = BASS_StreamCreateFile(0, midi_path.c_str(), 0, 0, 0);
    HSOUNDFONT Sf;
    //std::cout << "array size: " << live_soundfont_list.size() << "\n";
    for(int i = 0; i < checked_soundfonts.size(); i++)
    {
        //std::cout << "sdfdsfdsfsdfdsfdsertertyertr\n";
        Sf = BASS_MIDI_FontInit(checked_soundfonts[i].c_str(), 0);
        //std::cout << checked_soundfonts[i] << "dfgfdgfdgf\n";
    }
    BASS_MIDI_FONT FontSet{Sf, -1, 0};
    BASS_MIDI_FontSetVolume(Sf, 0.15);
    BASS_MIDI_StreamSetFonts(Stm, &FontSet, 1);
    
    BASS_ChannelSetAttribute(Stm, BASS_ATTRIB_MIDI_VOICES, parsed_config.bass_voice_count);
    
    if (Stm) {
        BASS_MIDI_StreamSetFilter(Stm, 0, reinterpret_cast<BOOL (*)(HSTREAM, int, BASS_MIDI_EVENT *, BOOL, void *)>(filter), nullptr);
        if (!BASS_ChannelPlay(Stm, 1)) {
            NVi::error("Player", "Failed to start BASS channel playback");
        }
    }
}
*/

#if defined(_WIN32) || defined(_WIN64)
    #define BASSMIDI_LIB "bassmidi.dll"
#else
    #define BASSMIDI_LIB "libbassmidi.so"
#endif

#ifdef NON_ANDROID
int main(int ac, char **av)
#else
int SDL_main(int ac, char **av)
#endif
{
#ifndef DEV_TEST
    const char * midi_path = av[1];
    const char * sf_path = av[2];
    
#ifndef NON_ANDROID
    std::string default_sf_path = NVFileUtils::GetFilePathA("piano_maganda.sf2", "rb");
    std::string default_midi_path = NVFileUtils::GetFilePathA("pfa_intro.mid", "rb");
    std::string ui_font = NVFileUtils::GetFilePathA("ui_font.ttf", "rb");
#endif
    if(NVFileUtils::FileExists(CONFIG_PATH) == true)
    {
        // Read Config and assign settings structure
        parsed_config = NVConf::ReadConfig();
        live_conf = parsed_config;
        liveColor.r = parsed_config.bg_R;
        liveColor.g = parsed_config.bg_G;
        liveColor.b = parsed_config.bg_B;
        liveColor.a = parsed_config.bg_A;
        live_note_speed = parsed_config.note_speed;
    }
    else
    {
        // If not assign default configuration
        default_config.bass_voice_count = 1000;
#ifndef NON_ANDROID
        default_config.current_soundfonts = {default_sf_path};
#endif
        default_config.bg_R = 43;
        default_config.bg_G = 43;
        default_config.bg_B = 43;
        default_config.bg_A = 255;
        liveColor.r = default_config.bg_R;
        liveColor.g = default_config.bg_G;
        liveColor.b = default_config.bg_B;
        liveColor.a = default_config.bg_A;
        default_config.gui_scale = 1.0f;
        live_conf = default_config;
    }
    
    if(NVFileUtils::FileExists(MIDI_LIST) == true)
    {
        NVi::ReadMidiList();
    }
    else
    {
        try {
            NVi::CreateMidiList();
        } catch (const std::exception& e) {
            NVi::error("Player", "Failed to create MIDI list: %s", e.what());
            // Continue without MIDI list
        }
    }
    
    if(NVFileUtils::FileExists(SF_LIST) == true)
    {
        live_soundfont_list = NVConf::ReadSoundfontList();
        checked_soundfonts = NVGui::GetCheckedSoundfonts(live_soundfont_list);
    }
    else
    {
        try {
            NVi::RefreshSFList();
        } catch (const std::exception& e) {
            NVi::error("Player", "Failed to refresh soundfont list: %s", e.what());
            // Continue without soundfont list
        }
    }
    
    if(parsed_config.last_midi_path.length() == 0)
    {
        // If there's no last MIDI path in config, default to the intro MIDI.
        parsed_config.last_midi_path = DEFAULT_MIDI;
        //std::cout << "No default midi\n";
    }

    // On app restart, always start with intro mode regardless of saved MIDI
    // This provides a consistent experience - intro plays first, then user can select other MIDIs
    is_intro_mode = true;
    parsed_config.last_midi_path = DEFAULT_MIDI; // Force intro MIDI on restart

    // Apply intro mode visuals and settings
    liveColor.r = 0;   // black background
    liveColor.g = 0;
    liveColor.b = 0;
    liveColor.a = 255;
    live_note_speed = 400; // intro note speed
    
    if(live_note_speed == 0)
    {
        NVi::error("Player", "Note speed cannot be zero");
        // On Android, don't crash - set a default speed and continue
        live_note_speed = 100; // Default fallback
    }

    if(!MIDI.start_parse(parsed_config.last_midi_path.c_str()))
    {
        std::ostringstream temp_msg;
        temp_msg << "Failed to load '" << parsed_config.last_midi_path << "'";
        NVi::error("Player", temp_msg.str().c_str());
        // On Android, don't crash - try to load the intro MIDI as fallback
        if (!MIDI.start_parse(DEFAULT_MIDI)) {
            NVi::error("Player", "Failed to load default MIDI file as well");
            // Continue anyway - the app should still run even without MIDI
        }
    }
    else
    {
        NVi::info("Player", "Midi file loaded successfully");
    }
    

    Win = new Canvas;
    
    SDL_SetRenderDrawColor(Win->Ren, liveColor.r, liveColor.g, liveColor.b, liveColor.a);

    //_WinH = Win->WinH - Win->WinW * 80 / 1000; Tscr = (double)_WinH / live_note_speed;


    if (!BASS_PluginLoad(BASSMIDI_LIB, 0)) {
        NVi::error("Player", "Failed to load BASS MIDI plugin");
    }
    BASS_SetConfig(BASS_CONFIG_MIDI_AUTOFONT, 0);
    if (!BASS_Init(-1, 44100, 0, 0, nullptr)) {
        NVi::error("Player", "Failed to initialize BASS audio system");
    }
    
    Stm = BASS_StreamCreateFile(0, parsed_config.last_midi_path.c_str(), 0, 0, BASS_SAMPLE_FLOAT);
    if (!Stm) {
        NVi::error("Player", "Failed to create BASS stream for MIDI file: %s", parsed_config.last_midi_path.c_str());
    } else {
        BASS_ChannelSetDSP(Stm, &dsp_limiter, 0, 0);
        // Apply master gain to channel to reduce perceived loudness
        BASS_ChannelSetAttribute(Stm, BASS_ATTRIB_VOL, master_gain);
    }
    HSOUNDFONT Sf;
    
    //Load all enabled soundfonts
    //std::cout << "Size: " << live_soundfont_list.size() << "\n";
    if(live_soundfont_list.size() == 0)
    {
        std::cout << "Default sf\n";
#ifndef NON_ANDROID
        if (!default_sf_path.empty()) {
            Sf = BASS_MIDI_FontInit(default_sf_path.c_str(), 0);
            if (!Sf) {
                NVi::error("Player", "Failed to load default soundfont: %s", default_sf_path.c_str());
            }
        } else {
            NVi::error("Player", "Default soundfont path is empty");
        }
#else
        Sf = BASS_MIDI_FontInit(DEFAULT_SOUNDFONT, 0);
        if (!Sf) {
            NVi::error("Player", "Failed to load default soundfont: %s", DEFAULT_SOUNDFONT);
        }
#endif
    }
    else
    {
        for(int i = 0; i < checked_soundfonts.size(); i++)
        {
            if (!checked_soundfonts[i].empty()) {
                Sf = BASS_MIDI_FontInit(checked_soundfonts[i].c_str(), 0);
                if (!Sf) {
                    NVi::error("Player", "Failed to load soundfont: %s", checked_soundfonts[i].c_str());
                }
            }
        }
    }
    BASS_MIDI_FONT FontSet{Sf, -1, 0};
    BASS_MIDI_FontSetVolume(Sf, 0.15);
    BASS_MIDI_StreamSetFonts(Stm, &FontSet, 1);
    
    // No more errape
    BASS_ChannelSetAttribute(Stm, BASS_ATTRIB_MIDI_VOICES, parsed_config.bass_voice_count);


    
    BASS_MIDI_StreamSetFilter(Stm, 0, reinterpret_cast<BOOL (*)(HSTREAM, int, BASS_MIDI_EVENT *, BOOL, void *)>(filter), nullptr);
    BASS_ChannelPlay(Stm, 1);
    
    const unsigned int FPS=1000000/Win->mod->refresh_rate;// 20 may be replaced with a limited frame rate
    //unsigned long long _FPS_Timer; // No ideea what this is for

    
    Uint64 lastTapTime = 0;
    const Uint32 doubleTapThreshold = 400; // in milliseconds
    
    
   	SDL_SetRenderDrawColor(Win->Ren, liveColor.r, liveColor.g, liveColor.b, liveColor.a);
    
    // 0 note speed results in invisible notes and extreme lag
    
    // updated tplay method
    // Run the main render loop while stream is playing or paused. Do not call Quit() when the stream ends;
    // instead exit the loop and allow the function to return gracefully (prevents abrupt crashes on normal end-of-playback).
    while (true)
    {
    // Frame start timestamp for simple frame rate limiting (reduce CPU/GPU usage)
    Uint64 frameStart = (Uint64)SDL_GetTicks();
	    _WinH = Win->WinH - Win->WinW * 80 / 1000; Tscr = (double)_WinH / live_note_speed; // On desktop it seems to cause frame shifting or icomplete frame rendering
		MIDI.update_to(Tplay + Tscr);
		MIDI.remove_to(Tplay);
		Win->canvas_clear();
		while (SDL_PollEvent(&Evt)) 
		{
			ImGui_ImplSDL3_ProcessEvent(&Evt);
			if (Evt.type == SDL_EVENT_QUIT)
				break;
				
			// Add keyboard controls for playback
			if (Evt.type == SDL_EVENT_KEY_DOWN) 
			{
				switch (Evt.key.key) {
					case SDLK_SPACE:
						toggle_pause();
						break;
					case SDLK_LEFT:
						seek_playback(-SEEK_AMOUNT);
						break;
					case SDLK_RIGHT:
						seek_playback(SEEK_AMOUNT);
						break;
					case SDLK_Q:
						NVi::Quit();
						break;
				}
			}
		}
		
		//pps = live_note_speed;
		
		SDL_SetRenderDrawColor(Win->Ren, liveColor.r, liveColor.g, liveColor.b, liveColor.a);
		
		for(int i=0;i!=128;++i)
		{
	        for (const NVnote &n : MIDI.L[KeyMap[i]])
	        {
				DrawNote(i, n, live_note_speed);
	        }
		}
		
		Win->DrawKeyBoard();
		NVGui::Run(Win->Ren);
        SDL_RenderPresent(Win->Ren);

        // Only update Tplay if not paused and stream is valid
        if (!is_paused && Stm) {
            QWORD pos = BASS_ChannelGetPosition(Stm, BASS_POS_BYTE);
            if (pos != -1) { // Check for valid position
                Tplay = BASS_ChannelBytes2Seconds(Stm, pos);
            }
        }

        // If the GUI requested a MIDI change, perform it safely here on the main thread
        if (!requested_midi_path.empty()) {
            std::string newpath = requested_midi_path;
            requested_midi_path.clear();

            // Stop and free old stream
            if (Stm) {
                BASS_ChannelStop(Stm);
                BASS_StreamFree(Stm);
                Stm = 0;
            }

            // Update config and reparse MIDI for visuals
            parsed_config.last_midi_path = newpath;
            live_conf.last_midi_path = newpath;
            // Update intro mode based on the new MIDI file
            is_intro_mode = (newpath.find("pfa_intro.mid") != std::string::npos);
            // Update file info for the new MIDI file
            NVi::UpdateFileInfo(newpath);
            // Reparse MIDI data for the new file
            MIDI.destroy_all();
            if (!MIDI.start_parse(parsed_config.last_midi_path.c_str())) {
                NVi::error("Player", "Failed to load requested midi: %s", parsed_config.last_midi_path.c_str());
            } else {
                // recreate BASS stream
                Stm = BASS_StreamCreateFile(0, parsed_config.last_midi_path.c_str(), 0, 0, BASS_SAMPLE_FLOAT);
                if (Stm) {
                    HSOUNDFONT Sf = 0;
                    if(live_soundfont_list.size() == 0)
                    {
#ifndef NON_ANDROID
                        std::string sf_path = NVFileUtils::GetFilePathA("piano_maganda.sf2","rb");
                        if (!sf_path.empty()) {
                            Sf = BASS_MIDI_FontInit(sf_path.c_str(), 0);
                        }
#else
                        Sf = BASS_MIDI_FontInit(DEFAULT_SOUNDFONT, 0);
#endif
                    }
                    else {
                        for(int i = 0; i < checked_soundfonts.size(); i++) {
                            if (!checked_soundfonts[i].empty()) {
                                Sf = BASS_MIDI_FontInit(checked_soundfonts[i].c_str(), 0);
                            }
                        }
                    }
                    if (Sf) {
                        BASS_MIDI_FONT FontSet{Sf, -1, 0};
                        BASS_MIDI_StreamSetFonts(Stm, &FontSet, 1);
                    }
                    BASS_ChannelSetAttribute(Stm, BASS_ATTRIB_MIDI_VOICES, parsed_config.bass_voice_count);
                    BASS_MIDI_StreamSetFilter(Stm, 0, reinterpret_cast<BOOL (*)(HSTREAM, int, BASS_MIDI_EVENT *, BOOL, void *)>(filter), nullptr);
                    BASS_ChannelPlay(Stm, 1);
                    // Ensure master gain is applied to newly created stream
                    BASS_ChannelSetAttribute(Stm, BASS_ATTRIB_VOL, master_gain);
                    // Reset playback time and resync visuals
                    QWORD pos = BASS_ChannelGetPosition(Stm, BASS_POS_BYTE);
                    if (pos != -1) {
                        Tplay = BASS_ChannelBytes2Seconds(Stm, pos);
                    }
                    for (int i = 0; i < 128; ++i) MIDI.L[i].clear();
                    MIDI.list_seek(Tplay);
                    MIDI.update_to(Tplay + Tscr);
                } else {
                    NVi::error("Player", "Failed to create BASS stream for new MIDI file");
                }
            }
        }

        // If the GUI requested a soundfont change, apply it to the running stream
        if (!requested_soundfont_path.empty()) {
            std::string sfpath = requested_soundfont_path;
            requested_soundfont_path.clear();
            // Initialize the new soundfont
            if (!sfpath.empty()) {
                HSOUNDFONT newSf = BASS_MIDI_FontInit(sfpath.c_str(), 0);
                if (newSf) {
                    BASS_MIDI_FONT FontSet{newSf, -1, 0};
                    if (Stm) {
                        // Don't stop the stream, just change the soundfont
                        BASS_MIDI_StreamSetFonts(Stm, &FontSet, 1);
                        NVi::info("Player", "Applied new soundfont at runtime: %s", sfpath.c_str());
                        // Update the checked soundfonts list to persist the selection
                        checked_soundfonts.clear();
                        checked_soundfonts.push_back(sfpath);
                    }
                    // Also keep the checked_soundfonts vector to reflect selection
                    // (we don't clear other soundfonts here because the GUI sends the exact path)
                } else {
                    NVi::error("Player", "Failed to initialize soundfont: %s", sfpath.c_str());
                }
            } else {
                NVi::error("Player", "Requested soundfont path is empty");
            }
        }

        // Simple frame limiting based on realtime_frame_rate (default 60). This reduces CPU usage on high-refresh devices.
        {
            Uint32 targetFps = (live_conf.realtime_frame_rate > 0) ? (Uint32)live_conf.realtime_frame_rate : 60u;
            Uint32 frameDelay = 1000u / (targetFps > 0 ? targetFps : 60u);
            Uint64 frameEnd = (Uint64)SDL_GetTicks();
            Uint32 frameTime = (Uint32)(frameEnd - frameStart);
            if (frameTime < frameDelay) {
                SDL_Delay(frameDelay - frameTime);
            }
        }
		
		const char * sdl_err = SDL_GetError();
		if(sdl_err && strlen(sdl_err) > 0)
		{
		    SDL_Log("SDL Error: %s", sdl_err);
		}
		// Continue running even when MIDI playback finishes - don't break the loop
		// This allows the app to stay responsive and the user can select new MIDI files
		// Only break on SDL_QUIT event or other critical errors
    }

    // Cleanup the stream but do not exit the process. This allows the app to remain responsive after playback finishes.
    if (Stm) {
        BASS_ChannelStop(Stm);
        BASS_StreamFree(Stm);
        Stm = 0;
    }
    // Ensure MIDI resources are cleaned up
    MIDI.destroy_all();
#else

/*
- - - - - - - - - - - - - - - - - - - - - - Testing new backend components - - - - - - - - - - - - - - - - - - - - - -
*/
    //std::vector<std::string> midi_files;
    //midi_files = NVFileUtils::GetFilesByExtension("/home/0xsys/Desktop",".mid");
    
    //for(int i = 0; i < midi_files.size(); i++)
    //{
    //    std::cout << "From Vector: " << midi_files[i] << "\n";
    //}

    // NVConf::configuration test;
    // NVConf::configuration test2;
    
/*
    test.bass_voice_count = 700;
    test.bg_B = 48;
    test.bg_G = 32;
    test.bg_R = 56;
    test.current_soundfont = "/home/someone/black/midi/Soundfont.sf2";
    test.window_w = 580;
    test.window_h = 430;
    NVConf::WriteConfig(test);
*/
/*
    test2 = NVConf::ReadConfig();
    std::cout << test2.bass_voice_count 
              << "\n" << test2.bg_B 
              << "\n" << test2.bg_G 
              << "\n" << test2.bg_R 
              << "\n" << test2.current_soundfont 
              << "\n" << test2.window_h 
              << "\n" << test2.window_w 
              << "\n";
*/

// NVi::info("Some Prefix", "Log idk\n");
// NVi::error("Error", "Something failed lol\n");
// NVi::warn("Warn prefix", "There might be a problem\n");
/*
Todo:
Finish midi and soundfont file scanning + implementing both midi and soudfont list files
Add support for bass_fx to reduce the loudness
Adding about info
Trying to make an apk build of this + practical tests
Releasing the source code
*/

//NVi::CreateMediaLists();
//    
//NVConf::CreateSoundfontList(live_soundfont_list);

/*
std::vector<SoundfontItem> reading = NVConf::ReadSoundfontList();
for(int i = 0; i < reading.size(); i++)
{
    std::cout << "Reading: " << reading[i].label << " | Enabled: " << reading[i].checked << "\n";
}
*/
    
#endif
    return 0;
}
