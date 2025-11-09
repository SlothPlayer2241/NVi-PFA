#include <iostream>
#include <string>

//#include <android/asset_manager.h>
//#include <android/asset_manager_jni.h>
//#include <SDL_android.h>   // Add this for SDL_AndroidGetAssetManager

#include "Gui.hxx"
#include "Config_Utils.hxx"
#include "Utils.hxx"
#include "file_utils.hxx"
#include "Json_Config.hxx"


#include "extern/imgui/imgui.h"
#include "extern/imgui_sdl3/imgui_impl_sdl3.h"
#include "extern/imgui_sdl3/imgui_impl_sdlrenderer3.h"





bool show_demo_window = true;
//bool main_gui_window = false;

int selIndex = 0;
static char midi_search[128];
static char sf_search[128];
static std::string midi_search_text;
static std::string sf_search_text;

// Global fonts (loaded in Setup)
static ImFont* g_font_small = nullptr;
static ImFont* g_font_large = nullptr;


// Define thresholds for double-tap detection
#define DOUBLE_TAP_TIME 0.3f  // Maximum time (in seconds) between taps
#define DOUBLE_TAP_DISTANCE 10.0f  // Maximum distance (in pixels) between taps

// Variables to track double-tap state
static float lastTapTime = 0.0f;
static ImVec2 lastTapPos = ImVec2(0.0f, 0.0f);

//ImGuiIO& io = ImGui::GetIO();

// Add these to your other extern declarations
extern bool is_paused;
extern void seek_playback(double seconds);
extern void toggle_pause();
extern Canvas* Win; // allow HUD access to frame rate and sizes

void NVGui::SetDefaultTheme()
{
    /*
    Theme: Moonlight
    Author: deathsu/madam-herta
    Original source: https://github.com/Madam-Herta/Moonlight
    */
    ImGuiStyle& style = ImGui::GetStyle();
	
	style.Alpha                    = 1.0f;
	style.DisabledAlpha            = 1.0f;
    style.WindowPadding            = ImVec2(8.0f, 8.0f);
    style.WindowRounding           = 8.0f;
	style.WindowBorderSize         = 0.0f;
	style.WindowMinSize            = ImVec2(20.0f, 20.0f);
	style.WindowTitleAlign         = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Right;
	style.ChildRounding            = 0.0f;
	style.ChildBorderSize          = 1.0f;
	style.PopupRounding            = 0.0f;
	style.PopupBorderSize          = 1.0f;
    style.FramePadding             = ImVec2(8.0f, 3.0f);
    style.FrameRounding            = 6.0f;
	style.FrameBorderSize          = 0.0f;
    style.ItemSpacing              = ImVec2(4.0f, 4.0f);
    style.ItemInnerSpacing         = ImVec2(4.0f, 1.5f);
    style.CellPadding              = ImVec2(6.0f, 6.0f);
	style.IndentSpacing            = 0.0f;
	style.ColumnsMinSpacing        = 4.900000095367432f;
	
	// Different size for the scrollbar so user can actually grab it lol
#ifdef NON_ANDROID
    style.ScrollbarSize            = 12.0f;
    io.Fonts->AddFontFromFileTTF("ui_font.ttf", 22.0f);
    style.ScrollbarSize            = 18.0f;
#endif

    style.ScrollbarRounding        = 8.0f;
#ifdef NON_ANDROID
    style.GrabMinSize              = 8.0f;
#else
    style.GrabMinSize              = 10.0f;
#endif

    style.GrabRounding             = 6.0f;
	style.TabRounding              = 8.89999961853027f;
	style.TabBorderSize            = 0.0f;
	// style.TabMinWidthForCloseButton = 0.0f; // This seems to be deprecated in Imgui v1.91.9b
	style.ColorButtonPosition      = ImGuiDir_Right;
	style.ButtonTextAlign          = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign      = ImVec2(0.0f, 0.0f);
	
	style.Colors[ImGuiCol_Text]                  = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
	style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.09250493347644806f, 0.100297249853611f, 0.1158798336982727f, 1.0f);
	style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_Border]                = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.1120669096708298f, 0.1262156516313553f, 0.1545064449310303f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
	style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.971993625164032f, 1.0f, 0.4980392456054688f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.0f, 0.7953379154205322f, 0.4980392456054688f, 1.0f);
	style.Colors[ImGuiCol_Button]                = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.1821731775999069f, 0.1897992044687271f, 0.1974248886108398f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.1545050293207169f, 0.1545048952102661f, 0.1545064449310303f, 1.0f);
	style.Colors[ImGuiCol_Header]                = ImVec4(0.1414651423692703f, 0.1629818230867386f, 0.2060086131095886f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.1072951927781105f, 0.107295036315918f, 0.1072961091995239f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_Separator]             = ImVec4(0.1293079704046249f, 0.1479243338108063f, 0.1931330561637878f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.1459212601184845f, 0.1459220051765442f, 0.1459227204322815f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.9725490212440491f, 1.0f, 0.4980392158031464f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.999999463558197f, 1.0f, 0.9999899864196777f, 1.0f);
	style.Colors[ImGuiCol_Tab]                   = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_TabActive]             = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused]          = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.1249424293637276f, 0.2735691666603088f, 0.5708154439926147f, 1.0f);
	style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.8841201663017273f, 0.7941429018974304f, 0.5615870356559753f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.9570815563201904f, 0.9570719599723816f, 0.9570761322975159f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
	style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.9356134533882141f, 0.9356129765510559f, 0.9356223344802856f, 1.0f);
	style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavHighlight]          = ImVec4(0.266094446182251f, 0.2890366911888123f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
	style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
}

void NVGui::Setup(SDL_Window *w, SDL_Renderer *r)
{
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
#ifndef NON_ANDROID
    io.IniFilename = "/data/data/com.qsp.nvpfa/files/imgui.ini"; // Custom path for imgui ini
#endif
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    // Setup ImGui style
    ImGui::StyleColorsDark();
    SetDefaultTheme(); // Setting a nice looking GUI :3
    
    
        //ImFont* font = io.Fonts->AddFontFromMemoryTTF(fontBuffer, fontSize, 19.0f);
      
    
    //Font suggested by Nerdly
    //Metrophobic-Regular.ttf
#ifndef NON_ANDROID
    std::string ui_font_file = NVFileUtils::GetFilePathA("ui_font.ttf", "rb");
    // Desktop base font
    g_font_small = io.Fonts->AddFontFromFileTTF(ui_font_file.c_str(), 16.0f);
#else
    // Android: load a small control font and a large UI font for window chrome
    g_font_small = io.Fonts->AddFontFromFileTTF("ui_font.ttf", 20.0f);
    g_font_large = io.Fonts->AddFontFromFileTTF("ui_font.ttf", 48.0f);
    // Use the large font as the default for window titles/headers, but we'll push the small font for most controls
    io.FontDefault = g_font_large;
#endif

    // Global scale tweak for Android to make UI very large on high-DPI devices
    // Keep desktop slightly reduced but Android much larger
#ifdef NON_ANDROID
    io.FontGlobalScale = 0.95f;
#else
    // Increase global scale on Android so overall UI chrome is very large on high-DPI screens
    io.FontGlobalScale = 2.5f;
    // Make scrollbars and grab handles larger for touch
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScrollbarSize = 28.0f;
    style.GrabMinSize = 18.0f;
#endif
    
    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(w, r);
    ImGui_ImplSDLRenderer3_Init(r);
}

/*
Functions for internal use only
*/

void HandleDoubleTap()
{
    // Get the current time
    float currentTime = ImGui::GetTime();

    // Check if the left mouse button was clicked
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        // Get the current mouse position
        ImVec2 currentPos = ImGui::GetMousePos();

        // Check if this is a double-tap
        if ((currentTime - lastTapTime) <= DOUBLE_TAP_TIME) {
            float dx = currentPos.x - lastTapPos.x;
            float dy = currentPos.y - lastTapPos.y;
            float distanceSquared = dx * dx + dy * dy;

            if (distanceSquared <= DOUBLE_TAP_DISTANCE * DOUBLE_TAP_DISTANCE) {
                // Double-tap detected
                main_gui_window = true;  // Open the ImGui window
                printf("Double-tap detected at (%f, %f)\n", currentPos.x, currentPos.y);
            }
        }

        // Update the last tap information
        lastTapTime = currentTime;
        lastTapPos = currentPos;
    }
}

// So many of you wanted this
// And I can relate to such a problem

// Filter only the filenames before showing them on the midi and soundfont lists or it will cause a godamn stupid chaos
std::string FilenameOnly(const std::string& path) 
{
    size_t slash = path.find_last_of("/\\");
    return (slash == std::string::npos) ? path : path.substr(slash + 1);
}

void RenderMidiList(const std::vector<std::string>& items, int& selectedIndex, std::string find_item)
{
    ImGui::BeginChild("ListRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
        for (int i = 0; i < items.size(); ++i) 
        {
            std::string filename = FilenameOnly(items[i]);
    
            // Filter check (case-insensitive optional)
            if (!find_item.empty() && filename.find(find_item) == std::string::npos)
                continue;
    
            bool isSelected = (i == selectedIndex);
    
            if (ImGui::Selectable((filename + "##" + std::to_string(i)).c_str(), isSelected)) 
            {
                selectedIndex = i;
            }
    
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
    
        ImGui::EndChild();
}

void RenderSoundfontList(std::vector<SoundfontItem>& items, std::string find_item)
{
    ImGui::BeginChild("ListRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    for (int i = 0; i < items.size(); ++i) 
    {
        std::string filename = FilenameOnly(items[i].label);
    
        // Filter check
        if (!find_item.empty() && filename.find(find_item) == std::string::npos)
            continue;
    
        // Unique ID to avoid conflicts
        ImGui::Checkbox((filename + "##" + std::to_string(i)).c_str(), &items[i].checked);
    }
    
    ImGui::EndChild();
}

std::vector<std::string> NVGui::GetCheckedSoundfonts(const std::vector<SoundfontItem>& items)
{
    std::vector<std::string> checkedItems;
    
    for (const auto& item : items) 
    {
        if (item.checked) 
        {
            checkedItems.push_back(item.label);
        }
    }
    
    return checkedItems;
}

void NVGui::Run(SDL_Renderer *r)
{
    NVGui nvg;
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    // Apply runtime GUI scale from config so Resize GUI slider takes effect immediately
    ImGuiIO& io = ImGui::GetIO();

    // Show save confirmation message
    static bool show_save_message = false;
    static float save_message_timer = 0.0f;
    if (show_save_message) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Config saved successfully!");
        save_message_timer -= ImGui::GetIO().DeltaTime;
        if (save_message_timer <= 0.0f) {
            show_save_message = false;
        }
    }

    // Show load confirmation message
    static bool show_load_message = false;
    static float load_message_timer = 0.0f;
    if (show_load_message) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Config loaded successfully!");
        load_message_timer -= ImGui::GetIO().DeltaTime;
        if (load_message_timer <= 0.0f) {
            show_load_message = false;
        }
    }

    // Note Counter: small left-side overlay showing total notes hit (renders even when GUI closed)
    if (live_conf.show_note_counter) {
        ImGuiIO& io = ImGui::GetIO();
        float pad = 10.0f * io.FontGlobalScale;
        ImVec2 size = ImVec2(200.0f * io.FontGlobalScale, 40.0f * io.FontGlobalScale);
        // Position counter on the left side, below the HUD
        ImVec2 pos = ImVec2(pad, pad + 80.0f * io.FontGlobalScale);
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::Begin("NoteCounter", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);

        int noteCount = 0;
        if (Win) noteCount = (int)Win->GetNoteCount();
        char countbuf[64];
        snprintf(countbuf, sizeof(countbuf), "Notes: %d", noteCount);
        ImGui::TextUnformatted(countbuf);
        ImGui::End();
    }

    // Show save confirmation message
    static bool show_save_message = false;
    static float save_message_timer = 0.0f;
    if (show_save_message) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Config saved successfully!");
        save_message_timer -= ImGui::GetIO().DeltaTime;
        if (save_message_timer <= 0.0f) {
            show_save_message = false;
        }
    }

    // Show load confirmation message
    static bool show_load_message = false;
    static float load_message_timer = 0.0f;
    if (show_load_message) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Config loaded successfully!");
        load_message_timer -= ImGui::GetIO().DeltaTime;
        if (load_message_timer <= 0.0f) {
            show_load_message = false;
        }
    }
#ifndef NON_ANDROID
    io.FontGlobalScale = live_conf.gui_scale > 0.0f ? live_conf.gui_scale * 0.95f : 0.95f;
#else
    io.FontGlobalScale = live_conf.gui_scale > 0.0f ? live_conf.gui_scale * 2.5f : 2.5f;
#endif
    //HandleDoubleTap();
    
    // When main GUI is closed, handle raw touch/mouse input ourselves for a reliable full-screen 3-zone overlay.
    if (!main_gui_window) {
        // Touch timing constants
        const float HOLD_THRESHOLD = 0.5f;      // seconds to consider a hold
        const float REPEAT_INTERVAL = 0.25f;    // seconds between repeated seeks while holding

        static double zone_start_time[3] = {0.0, 0.0, 0.0};
        static double zone_last_repeat[3] = {0.0, 0.0, 0.0};
        static bool zone_active[3] = {false, false, false};
        static bool zone_hold_triggered[3] = {false, false, false};

        float screen_w = io.DisplaySize.x;
        float screen_h = io.DisplaySize.y;
        float zone_w = screen_w / 3.0f;

        double now = ImGui::GetTime();

        bool down = ImGui::GetIO().MouseDown[0];
        ImVec2 mp = ImGui::GetIO().MousePos;

        for (int zi = 0; zi < 3; ++zi) {
            bool inside = (mp.x >= zone_w * zi && mp.x < zone_w * (zi + 1) && mp.y >= 0 && mp.y <= screen_h);

            // Activation start
            if (down && inside && !zone_active[zi]) {
                // Start this zone
                zone_active[zi] = true;
                zone_start_time[zi] = now;
                zone_last_repeat[zi] = now - REPEAT_INTERVAL; // allow immediate repeat
                zone_hold_triggered[zi] = false;
            }

            if (zone_active[zi]) {
                if (down) {
                    double held = now - zone_start_time[zi];
                    if ((zi == 0 || zi == 2) && held >= HOLD_THRESHOLD) {
                        if (now - zone_last_repeat[zi] >= REPEAT_INTERVAL) {
                            seek_playback((zi == 0) ? -SEEK_AMOUNT : SEEK_AMOUNT);
                            zone_last_repeat[zi] = now;
                            zone_hold_triggered[zi] = true;
                        }
                    }
                    if (zi == 1 && !zone_hold_triggered[1] && held >= HOLD_THRESHOLD) {
                        main_gui_window = true;
                        zone_hold_triggered[1] = true;
                    }
                } else {
                    // Released
                    if (!zone_hold_triggered[zi]) {
                        if (zi == 0) seek_playback(-SEEK_AMOUNT);
                        else if (zi == 2) seek_playback(SEEK_AMOUNT);
                        else if (zi == 1) toggle_pause();
                    }
                    zone_active[zi] = false;
                    zone_hold_triggered[zi] = false;
                    zone_start_time[zi] = 0.0;
                    zone_last_repeat[zi] = 0.0;
                }
            }
            // If user moves the pointer outside while pressed, we keep the zone active until release (intentional)
        }
    }
    
    // HUD: small top-left overlay showing time, FPS and score
    {
        ImGuiIO& io = ImGui::GetIO();
        float pad = 10.0f * io.FontGlobalScale;
        ImVec2 size = ImVec2(200.0f * io.FontGlobalScale, 60.0f * io.FontGlobalScale);
        // Position HUD on the left side for better visibility on Android
        ImVec2 pos = ImVec2(pad, pad);
        ImGui::SetNextWindowBgAlpha(0.35f);
        ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
        ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        // Time: use Player.cxx Tplay if exposed via extern; fallback to 0:00
        double seconds = 0.0;
        // Try to access playback time from global variable Tplay if available
        extern double Tplay; // declared in Player.cxx
        seconds = Tplay;
        int minutes = (int)(seconds / 60.0);
        int secs = (int)(seconds) % 60;
        char timebuf[32];
        snprintf(timebuf, sizeof(timebuf), "Time : %02d:%02d", minutes, secs);
        ImGui::TextUnformatted(timebuf);

        // FPS: obtain from Win if available and enabled
        if (live_conf.show_fps) {
            float fps = 0.0f;
            if (Win) fps = Win->GetFrameRate();
            char fpsbuf[32];
            snprintf(fpsbuf, sizeof(fpsbuf), "FPS: %.1f", fps);
            ImGui::TextUnformatted(fpsbuf);
        }

        // Score placeholder
        ImGui::TextUnformatted("Score: N/A");
        ImGui::End();
    }
    
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    //if (show_demo_window)
    //    ImGui::ShowDemoWindow(&show_demo_window);
    //
            // Reserved for future use
            //ImGui::Text("%.1f FPS", nvg.io.Framerate);
    
    
            // Show the main GUI window
            if (main_gui_window)
            {
                // Use smaller font for controls so buttons/checks render at a comfortable touch size
                if (g_font_small) ImGui::PushFont(g_font_small);
                ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
                ImGui::SetNextWindowPos(center, ImGuiCond_Once, ImVec2(0.5f, 0.5f)); // Pivot 0.5 = center
#ifdef NON_ANDROID
                // Desktop: slightly smaller default window
                ImGui::SetNextWindowSizeConstraints(ImVec2(600, 320), ImVec2(FLT_MAX, FLT_MAX));
                ImGui::Begin("NVi PFA", &main_gui_window);
#else           // Setting up a different ui layout for mobile users
                // Mobile: reduced window size to fit comfortably on smaller screens
                // Larger default window for modern phones/tablets
                ImGui::SetNextWindowSize(ImVec2(900.0f, 600.0f));
                ImGui::Begin("NVi PFA", &main_gui_window, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
#endif
                
                //ImGui::SameLine();
                
                if (ImGui::Button("Quit"))
                    NVi::Quit();
                    
                if (ImGui::BeginItemTooltip())
                {
                    ImGui::Text("Quit NVi PFA");
                    ImGui::EndTooltip();
                }
                //ImGui::SameLine();
                
                if(ImGui::BeginTabBar("tabs", ImGuiTabBarFlags_None))
                {
                    if (ImGui::BeginTabItem("Play MIDI Files"))
                    {
                        ImGui::SetNextItemWidth(300);
                        ImGui::InputText("##A", midi_search, 128);
                        
                        midi_search_text = midi_search;
                        
                        // Draw placeholder text if empty and not focused
                        if (strlen(midi_search) == 0 && !ImGui::IsItemActive()) 
                        {
                            ImVec2 pos = ImGui::GetItemRectMin();
                            ImVec2 text_pos = ImVec2(pos.x + ImGui::GetStyle().FramePadding.x, pos.y + ImGui::GetStyle().FramePadding.y);
                            ImGui::GetWindowDrawList()->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), "Search midis");
                        }
                        
                        ImGui::SameLine();
                        
                        if(ImGui::Button("Refresh List"))
                        {
                            NVi::ReloadMidiList(); // Reload MIDI list without restart
                        }
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Synchronize the midi file list with the new files created");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::SameLine();
                        
                        if(ImGui::Button("Reload All"))
                        {
                            NVi::ReloadAllResources(); // Reload all resources
                        }
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Reload all MIDI and soundfont resources");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::SameLine();
                        
                        if (ImGui::Button("Select MIDI File"))
                        {
                            // Immediately request the player to switch to the selected MIDI
                            if (selIndex < live_midi_list.size()) {
                                live_conf.last_midi_path = live_midi_list[selIndex];
                                // Also set the runtime note speed for user-selected MIDIs
                                live_note_speed = 2000;
                                NVi::RequestMidiChange(live_midi_list[selIndex]);
                                // Close the GUI after selecting a MIDI file
                                main_gui_window = false;
                            }
                        }
                            
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Plays the selected MIDI immediately");
                            ImGui::EndTooltip();
                        }
                        
                        //ImGui::SameLine();
                        
                        // Not used yet...
                        //if (ImGui::Button("Close"))
                        //    //NVi::CloseMidiPlayback();
                        //    std::cout << "Close midi playback\n";
                        
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Close and reset midi playback");
                            ImGui::EndTooltip();
                        }
                        
                        RenderMidiList(live_midi_list, selIndex, midi_search_text);
                        
                        // Update file info when selection changes
                        if (selIndex < live_midi_list.size()) {
                            static int last_selection = -1;
                            if (selIndex != last_selection) {
                                NVi::UpdateFileInfo(live_midi_list[selIndex]);
                                last_selection = selIndex;
                            }
                        }
                        
                        //std::cout << "Selection index: " << live_midi_list[selIndex] << "\n";
                        ImGui::EndTabItem();
                    }
                    
                    if (ImGui::BeginTabItem("Soundfonts"))
                    {
                        ImGui::Text("Default GM Soundfont");
                        ImGui::Checkbox("Use Default GM Soundfont", &live_conf.use_default_gm_soundfont);
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Use built-in General MIDI soundfont (gm.sf2)");
                            ImGui::EndTooltip();
                        }
                        
                        if (live_conf.use_default_gm_soundfont) {
                            ImGui::Text("GM Soundfont Path: %s", live_conf.default_gm_soundfont_path.c_str());
                            
                            // Check if file exists
                            bool gm_exists = NVi::CheckGMSoundfontExists();
                            if (gm_exists) {
                                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "✓ GM Soundfont file found");
                            } else {
                                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "✗ GM Soundfont file not found");
                            }
                            
                            if (ImGui::Button("Load GM Soundfont")) {
                                // Load the GM soundfont
                                std::string gm_path = NVConf::GetDefaultGMSoundfontPath();
                                live_conf.default_gm_soundfont_path = gm_path;
                                NVi::LoadDefaultGMSoundfont();
                            }
                            if (ImGui::BeginItemTooltip())
                            {
                                ImGui::Text("Load the default GM soundfont (gm.sf2)");
                                ImGui::EndTooltip();
                            }
                        }
                        
                        ImGui::Text("\n");
                        ImGui::Text("Custom Soundfonts:");
                        ImGui::SetNextItemWidth(300);
                        ImGui::InputText("##E", sf_search, 128);
                        
                        sf_search_text = sf_search;
                        
                        // Draw placeholder text if empty and not focused
                        if (strlen(midi_search) == 0 && !ImGui::IsItemActive()) 
                        {
                            ImVec2 pos = ImGui::GetItemRectMin();
                            ImVec2 text_pos = ImVec2(pos.x + ImGui::GetStyle().FramePadding.x, pos.y + ImGui::GetStyle().FramePadding.y);
                            ImGui::GetWindowDrawList()->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_TextDisabled), "Search soundfonts");
                        }
                        
                        ImGui::SameLine();
                        
                        if(ImGui::Button("Refresh List"))
                        {
                            NVi::ReloadSoundfontList(); // Reload soundfont list without restart
                        }
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Synchronize the soundfont file list with the new files created");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::SameLine();
                        
                        if(ImGui::Button("Select SF"))
                        {
                            // Apply the selected soundfont immediately (no restart)
                            // Find the first checked soundfont in the runtime list
                            for (const auto &item : live_soundfont_list) {
                                if (item.checked) {
                                    // Request runtime change
                                    NVi::RequestSoundfontChange(item.label);
                                    break;
                                }
                            }
                            // Persist the soundfont list so selection survives restart
                            NVConf::CreateSoundfontList(live_soundfont_list);
                        }
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Save the modified soundfont list");
                            ImGui::EndTooltip();
                        }
                        RenderSoundfontList(live_soundfont_list, sf_search_text);
                            
                        ImGui::EndTabItem();
                    }
                    
                    if (ImGui::BeginTabItem("Settings"))
                    {
                        ImGui::Text("Settings marked with * require restart of NV PFA\n\n");
                        ImGui::SliderInt("Note Speed", &live_note_speed, 50, 50000);
                        ImGui::Text("Background Color");
                        clear_color = ImVec4(live_conf.bg_R / 255.0f, live_conf.bg_G / 255.0f, live_conf.bg_B / 255.0f, live_conf.bg_A / 255.0f);
                        ImGui::ColorEdit3("##H", (float*)&clear_color);
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Change the background color of the main scene");
                            ImGui::EndTooltip();
                        }
                        liveColor = NVi::Frgba2Irgba(clear_color);
                        ImGui::Text("\n");
                        ImGui::Text("Voice Count *");
                        static int i0 = 123;
                        ImGui::InputInt("##LOL", &live_conf.bass_voice_count);
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Set how many notes can be played on specific instruments");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::Text("\n");
                        ImGui::Text("Custom Note Colors");
                        ImGui::Checkbox("Use Custom Note Colors", &live_conf.use_custom_note_colors);
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Enable custom colors for notes instead of default white");
                            ImGui::EndTooltip();
                        }
                        
                        if (live_conf.use_custom_note_colors) {
                            ImGui::Text("Piano Key Colors:");
                            ImGui::Text("Choose colors for the 7 piano keys (C, D, E, F, G, A, B):");

                            // Define 7 piano key colors
                            static float piano_colors[7][3] = {
                                {1.0f, 0.0f, 0.0f},    // C - Red
                                {1.0f, 0.5f, 0.0f},    // D - Orange
                                {1.0f, 1.0f, 0.0f},    // E - Yellow
                                {0.0f, 1.0f, 0.0f},    // F - Green
                                {0.0f, 0.0f, 1.0f},    // G - Blue
                                {0.5f, 0.0f, 1.0f},    // A - Indigo
                                {1.0f, 0.0f, 1.0f}     // B - Violet
                            };

                            const char* key_names[7] = {"C", "D", "E", "F", "G", "A", "B"};

                            // Initialize from config if available
                            if (live_conf.custom_note_colors.size() >= 7) {
                                for (int i = 0; i < 7; ++i) {
                                    int color = live_conf.custom_note_colors[i];
                                    piano_colors[i][0] = ((color >> 16) & 0xFF) / 255.0f;
                                    piano_colors[i][1] = ((color >> 8) & 0xFF) / 255.0f;
                                    piano_colors[i][2] = (color & 0xFF) / 255.0f;
                                }
                            }

                            for (int i = 0; i < 7; ++i) {
                                ImGui::ColorEdit3((std::string(key_names[i]) + " Key Color").c_str(), piano_colors[i]);
                                ImGui::SameLine();
                                if (ImGui::Button((std::string("Apply ") + key_names[i]).c_str())) {
                                    int color = ((int)(piano_colors[i][0]*255.0f) << 16) |
                                               ((int)(piano_colors[i][1]*255.0f) << 8) |
                                               (int)(piano_colors[i][2]*255.0f);
                                    if (live_conf.custom_note_colors.size() <= (size_t)i) {
                                        live_conf.custom_note_colors.resize(i + 1);
                                    }
                                    live_conf.custom_note_colors[i] = color;
                                    NVConf::WriteConfig(live_conf);
                                }
                            }

                            ImGui::Separator();
                            ImGui::Text("Quick Color Presets:");
                            if (ImGui::Button("Rainbow Colors")) {
                                int rainbow_colors[7] = {
                                    0xFF0000,   // Red
                                    0xFF8000,   // Orange
                                    0xFFFF00,   // Yellow
                                    0x00FF00,   // Green
                                    0x0000FF,   // Blue
                                    0x8000FF,   // Indigo
                                    0xFF00FF    // Violet
                                };
                                live_conf.custom_note_colors.assign(rainbow_colors, rainbow_colors + 7);
                                NVConf::WriteConfig(live_conf);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Pastel Colors")) {
                                int pastel_colors[7] = {
                                    0xFFB3BA,   // Pastel Red
                                    0xFFDFBA,   // Pastel Orange
                                    0xFFFFBA,   // Pastel Yellow
                                    0xBAFFBA,   // Pastel Green
                                    0xBAE1FF,   // Pastel Blue
                                    0xD7BAFF,   // Pastel Indigo
                                    0xFFB3F0    // Pastel Violet
                                };
                                live_conf.custom_note_colors.assign(pastel_colors, pastel_colors + 7);
                                NVConf::WriteConfig(live_conf);
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Monochrome")) {
                                int mono_colors[7] = {
                                    0xFFFFFF, 0xF0F0F0, 0xE0E0E0, 0xD0D0D0,
                                    0xC0C0C0, 0xB0B0B0, 0xA0A0A0
                                };
                                live_conf.custom_note_colors.assign(mono_colors, mono_colors + 7);
                                NVConf::WriteConfig(live_conf);
                            }

                            if (ImGui::Button("Clear All Piano Colors")) {
                                live_conf.custom_note_colors.clear();
                                NVConf::WriteConfig(live_conf);
                            }

                            ImGui::Separator();
                            static bool show_save_message = false;
                            static float save_message_timer = 0.0f;

                            if (ImGui::Button("Save Colors")) {
                                // Save the currently selected piano key colors
                                // This ensures the colors are persisted even if user doesn't use individual "Apply" buttons
                                for (int i = 0; i < 7; ++i) {
                                    int color = ((int)(piano_colors[i][0]*255.0f) << 16) |
                                               ((int)(piano_colors[i][1]*255.0f) << 8) |
                                               (int)(piano_colors[i][2]*255.0f);
                                    if (live_conf.custom_note_colors.size() <= (size_t)i) {
                                        live_conf.custom_note_colors.resize(i + 1);
                                    }
                                    live_conf.custom_note_colors[i] = color;
                                }
                                NVConf::WriteConfig(live_conf);
                                show_save_message = true;
                                save_message_timer = 2.0f; // Show message for 2 seconds
                            }
                            if (ImGui::BeginItemTooltip())
                            {
                                ImGui::Text("Save the currently selected piano key colors to config");
                                ImGui::EndTooltip();
                            }

                            // Show save confirmation message with timer
                            if (show_save_message) {
                                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Colors saved successfully!");
                                save_message_timer -= ImGui::GetIO().DeltaTime;
                                if (save_message_timer <= 0.0f) {
                                    show_save_message = false;
                                }
                            }
                        }
                        
                        ImGui::Text("\n");
                        ImGui::Text("Background Image");
                        ImGui::Checkbox("Use Background Image", &live_conf.use_background_image);
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Enable background image instead of solid color");
                            ImGui::EndTooltip();
                        }
                        
                        if (live_conf.use_background_image) {
                            static char image_path[256] = "";
                            strncpy(image_path, live_conf.background_image_path.c_str(), sizeof(image_path) - 1);
                            image_path[sizeof(image_path) - 1] = '\0';
                            
                            ImGui::InputText("Image Path", image_path, sizeof(image_path));
                            live_conf.background_image_path = image_path;
                            
                            if (ImGui::Button("Load Image")) {
                                // Load the background image
                                extern Canvas* Win;
                                if (Win) {
                                    Win->LoadBackgroundImage(live_conf.background_image_path);
                                }
                            }
                            if (ImGui::BeginItemTooltip())
                            {
                                ImGui::Text("Load the specified background image");
                                ImGui::EndTooltip();
                            }
                        }
                        
                        ImGui::Text("\n");
                        ImGui::Text("Custom Key Range");
                        ImGui::Checkbox("Use Custom Key Range", &live_conf.use_custom_key_range);
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Limit the keyboard display to a specific range of MIDI notes");
                            ImGui::EndTooltip();
                        }
                        
                        if (live_conf.use_custom_key_range) {
                            ImGui::SliderInt("Start Note", &live_conf.key_range_start, 0, 127);
                            ImGui::SliderInt("End Note", &live_conf.key_range_end, 0, 127);
                            
                            if (live_conf.key_range_start > live_conf.key_range_end) {
                                live_conf.key_range_end = live_conf.key_range_start;
                            }
                            
                            ImGui::Text("Range: %d - %d (%d notes)", 
                                live_conf.key_range_start, 
                                live_conf.key_range_end, 
                                live_conf.key_range_end - live_conf.key_range_start + 1);
                        }
                        
                        ImGui::Text("\n");
                        ImGui::Text("Logging");
                        ImGui::Checkbox("Enable File Logging", &live_conf.enable_file_logging);
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Enable logging to file for development purposes");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::Text("\n");
                        ImGui::Text("Custom Search Directories");
                        
                        ImGui::Text("MIDI Directories:");
                        static char midi_dir_input[256] = "";
                        ImGui::InputText("Add MIDI Directory", midi_dir_input, sizeof(midi_dir_input));
                        ImGui::SameLine();
                        if (ImGui::Button("Add MIDI Dir")) {
                            if (strlen(midi_dir_input) > 0) {
                                live_conf.custom_midi_directories.push_back(std::string(midi_dir_input));
                                midi_dir_input[0] = '\0';
                            }
                        }
                        
                        for (size_t i = 0; i < live_conf.custom_midi_directories.size(); ++i) {
                            ImGui::Text("  %s", live_conf.custom_midi_directories[i].c_str());
                            ImGui::SameLine();
                            if (ImGui::Button(("Remove##midi" + std::to_string(i)).c_str())) {
                                live_conf.custom_midi_directories.erase(live_conf.custom_midi_directories.begin() + i);
                                break;
                            }
                        }
                        
                        ImGui::Text("Soundfont Directories:");
                        static char sf_dir_input[256] = "";
                        ImGui::InputText("Add Soundfont Directory", sf_dir_input, sizeof(sf_dir_input));
                        ImGui::SameLine();
                        if (ImGui::Button("Add SF Dir")) {
                            if (strlen(sf_dir_input) > 0) {
                                live_conf.custom_soundfont_directories.push_back(std::string(sf_dir_input));
                                sf_dir_input[0] = '\0';
                            }
                        }
                        
                        for (size_t i = 0; i < live_conf.custom_soundfont_directories.size(); ++i) {
                            ImGui::Text("  %s", live_conf.custom_soundfont_directories[i].c_str());
                            ImGui::SameLine();
                            if (ImGui::Button(("Remove##sf" + std::to_string(i)).c_str())) {
                                live_conf.custom_soundfont_directories.erase(live_conf.custom_soundfont_directories.begin() + i);
                                break;
                            }
                        }
                        
                        ImGui::Text("\n");
                        ImGui::Text("Audio Settings");
                        ImGui::SliderInt("Realtime Frame Rate", &live_conf.realtime_frame_rate, 30, 120);
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Target frame rate for realtime audio processing");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::SliderFloat("Frame Fluctuation Threshold", &live_conf.frame_fluctuation_threshold, 0.01f, 1.0f);
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Threshold for detecting frame rate fluctuations");
                            ImGui::EndTooltip();
                        }
                        
                        ImGui::Checkbox("Enable Audio Limiter", &live_conf.enable_audio_limiter);
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Enable audio limiter to prevent clipping");
                            ImGui::EndTooltip();
                        }
                        
                        if (live_conf.enable_audio_limiter) {
                            ImGui::SliderFloat("Limiter Threshold", &live_conf.limiter_threshold, 0.1f, 1.0f);
                            if (ImGui::BeginItemTooltip())
                            {
                                ImGui::Text("Audio limiter threshold (0.1 = 10%%, 1.0 = 100%%)");
                                ImGui::EndTooltip();
                            }
                        }
                        
                        ImGui::Text("\n");
                        ImGui::Text("Performance Settings");
                        if (ImGui::Button("Optimize Rendering")) {
                            extern Canvas* Win;
                            if (Win) {
                                Win->OptimizeRendering();
                            }
                        }
                        if (ImGui::BeginItemTooltip())
                        {
                            ImGui::Text("Apply rendering optimizations for better performance");
                            ImGui::EndTooltip();
                        }
                        
                        // Keeping the background color updated
                        live_conf.bg_R = liveColor.r;
                        live_conf.bg_G = liveColor.g;
                        live_conf.bg_B = liveColor.b;
                        live_conf.bg_A = liveColor.a;
                        live_conf.note_speed = live_note_speed;
                        
                        // Persist settings automatically
                        NVConf::WriteConfig(live_conf);
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("GUI"))
                    {
                        ImGui::Text("GUI Settings (resize applies immediately)");
                        ImGui::SliderFloat("Resize GUI", &live_conf.gui_scale, 0.5f, 4.0f);
                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text("Adjust global GUI scale (fonts and UI chrome). Applies immediately.");
                            ImGui::EndTooltip();
                        }
                        ImGui::Checkbox("Show FPS", &live_conf.show_fps);
                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text("Show/hide FPS counter in the HUD");
                            ImGui::EndTooltip();
                        }
                        ImGui::Checkbox("Show Note Counter", &live_conf.show_note_counter);
                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text("Show/hide note counter on the left side of the screen");
                            ImGui::EndTooltip();
                        }
                        ImGui::Checkbox("Run in Background", &live_conf.run_in_background);
                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text("Allow the app to continue running when in background");
                            ImGui::EndTooltip();
                        }
                        ImGui::Checkbox("Translucent Navigation", &live_conf.translucent_navigation);
                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text("Enable translucent navigation bar (may cause issues on Android 11/12)");
                            ImGui::EndTooltip();
                        }
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Save & Load"))
                    {
                        ImGui::Text("Save & Load Settings\n\n");

                        static char save_path[256] = "";
                        ImGui::InputText("Save Directory", save_path, sizeof(save_path));
                        ImGui::SameLine();
                        if (ImGui::Button("Save Config")) {
                            if (strlen(save_path) > 0) {
                                std::string full_path = std::string(save_path) + "/nvpfa_config.json";
                                if (JsonConfig::SaveConfig(full_path, live_conf)) {
                                    show_save_message = true;
                                    save_message_timer = 2.0f;
                                }
                            }
                        }
                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text("Save current settings to a JSON file");
                            ImGui::EndTooltip();
                        }

                        ImGui::Text("\n");

                        static char load_path[256] = "";
                        ImGui::InputText("Load File Path", load_path, sizeof(load_path));
                        ImGui::SameLine();
                        if (ImGui::Button("Load Config")) {
                            if (strlen(load_path) > 0) {
                                NVConf::configuration temp_config;
                                if (JsonConfig::LoadConfig(load_path, temp_config)) {
                                    live_conf = temp_config;
                                    show_load_message = true;
                                    load_message_timer = 2.0f;
                                }
                            }
                        }
                        if (ImGui::BeginItemTooltip()) {
                            ImGui::Text("Load settings from a JSON file");
                            ImGui::EndTooltip();
                        }

                        ImGui::EndTabItem();
                    }
                    
                       // Note Counter: small left-side overlay showing total notes hit
                       if (live_conf.show_note_counter) {
                           ImGuiIO& io = ImGui::GetIO();
                           float pad = 10.0f * io.FontGlobalScale;
                           ImVec2 size = ImVec2(200.0f * io.FontGlobalScale, 40.0f * io.FontGlobalScale);
                           // Position counter on the left side, below the HUD
                           ImVec2 pos = ImVec2(pad, pad + 80.0f * io.FontGlobalScale);
                           ImGui::SetNextWindowBgAlpha(0.35f);
                           ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
                           ImGui::Begin("NoteCounter", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
                           char countbuf[32];
                           snprintf(countbuf, sizeof(countbuf), "Notes: %d", 0); // Placeholder for now
                           ImGui::TextUnformatted(countbuf);
                           ImGui::End();
                       }
                    if (ImGui::BeginTabItem("File Info"))
                    {
                        ImGui::BeginChild("FileInfoRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        
                        if (current_file_info && !current_file_info->title.empty()) {
                            ImGui::Text("Title: %s", current_file_info->title.c_str());
                        }
                        if (current_file_info && !current_file_info->artist.empty()) {
                            ImGui::Text("Artist: %s", current_file_info->artist.c_str());
                        }
                        if (current_file_info && !current_file_info->copyright.empty()) {
                            ImGui::Text("Copyright: %s", current_file_info->copyright.c_str());
                        }
                        if (current_file_info && !current_file_info->comment.empty()) {
                            ImGui::Text("Comment: %s", current_file_info->comment.c_str());
                        }
                        
                        if (current_file_info) {
                            ImGui::Text("MIDI Type: %d", current_file_info->type);
                            ImGui::Text("Number of Tracks: %d", current_file_info->tracks);
                            ImGui::Text("PPQN (Pulses Per Quarter Note): %d", current_file_info->ppnq);
                            ImGui::Text("Duration: %.2f seconds", current_file_info->duration_seconds);
                            
                            if (current_file_info->duration_seconds > 0) {
                                int minutes = (int)(current_file_info->duration_seconds / 60);
                                int seconds = (int)(current_file_info->duration_seconds) % 60;
                                ImGui::Text("Duration: %d:%02d", minutes, seconds);
                            }
                        }
                        
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    
                    if (ImGui::BeginTabItem("About"))
                    {
                        ImGui::BeginChild("ScrollRegion", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        ImGui::Text("A clone of the original Piano From Above for mobile based on Qishipai's midi processing library.");
                        ImGui::Text("Authors:");
                        ImGui::Text("NVirsual: Qishipai");
                        ImGui::Text("Piano From Above imitation: Tweak1600");
                        ImGui::Text("Improved by:");
                        ImGui::Text("0xsys");
                        ImGui::Text("Hexagon-Midis\n\n");
                        ImGui::Text("Icon Made by Zeal");
                        ImGui::Text("Powered by: SDL3, Imgui, bass and bass plugins");
                        ImGui::Text("Special Thanks to CraftyKid for adding all the features and fixing bugs!");
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                if (g_font_small) ImGui::PopFont();
                ImGui::End();
            }
    
            // Rendering
            ImGui::Render();
            //SDL_RenderClear(r); // Don't clear the render bc no midi scene will show up
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), r);
}
