#include <cstdarg>
#include <cstdio>
#include <ctime>
#include "Utils.hxx"
#include "MIDI.hxx"
//#include <bass.h>
#include "extern/audio/bass.h"

#if defined(_WIN32) || defined(_WIN64) /* Setting font colors using the Windows API */

#include <Windows.h>

void NVi::error(const char *prefix, const char *str, ...)
{
    va_list args;
    static HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    SetConsoleTextAttribute(h, 0x0F); 
    fprintf(stderr, "[%s] ", prefix);
    SetConsoleTextAttribute(h, 0x0C); 
    fprintf(stderr, "ERROR: ");
    SetConsoleTextAttribute(h, 0x07);
    va_start(args, str); vfprintf(stderr, str, args); va_end(args);
}

void NVi::warn(const char *prefix, const char *str, ...)
{
    va_list args;
    static HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    SetConsoleTextAttribute(h, 0x0F); 
    fprintf(stderr, "[%s] ", prefix);
    SetConsoleTextAttribute(h, 0x0E); 
    fprintf(stderr, "WARNING: ");
    SetConsoleTextAttribute(h, 0x07);
    va_start(args, str); vfprintf(stderr, str, args); va_end(args);
}

void NVi::info(const char *prefix, const char *str, ...)
{
    va_list args;
    static HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    SetConsoleTextAttribute(h, 0x0F); 
    fprintf(stderr, "[%s] ", prefix);
    SetConsoleTextAttribute(h, 0x0A); 
    fprintf(stderr, "INFO: ");
    SetConsoleTextAttribute(h, 0x07);
    va_start(args, str); vfprintf(stderr, str, args); va_end(args);
}

#else /* Setting the terminal font color using the \033 control character */

void NVi::error(const char *prefix, const char *str, ...)
{
    va_list args;
    fprintf(stderr, "\033[1m[%s] \033[31mERROR: \033[m", prefix);
    va_start(args, str); 
    vfprintf(stderr, str, args); 
    va_end(args);
}

void NVi::warn(const char *prefix, const char *str, ...)
{
    va_list args;
    fprintf(stderr, "\033[1m[%s] \033[33mWARNING: \033[m", prefix);
    va_start(args, str); 
    vfprintf(stderr, str, args); 
    va_end(args);
}

void NVi::info(const char *prefix, const char *str, ...)
{
    va_list args;
    fprintf(stderr, "\033[1m[%s] \033[32mINFO: \033[m", prefix);
    va_start(args, str); 
    vfprintf(stderr, str, args); 
    va_end(args);
}

#endif


NVi::RGBAint NVi::Frgba2Irgba(ImVec4& col)
{
    RGBAint res;
    res.r = static_cast<int>(col.x * 255.0f);
    res.g = static_cast<int>(col.y * 255.0f);
    res.b = static_cast<int>(col.z * 255.0f);
    res.a = static_cast<int>(col.w * 255.0f);
    return res;
}

//using namespace NVi;

NVi::nv_ul64 NVi::operator"" _u64be(const char *str, size_t n)
{
    nv_ul64 ans = 0;
    u16_t   sft = 0; 

    while (nv_ul64 ch = str[sft])
    {
        ans |= ch << (sft++ << 3);
    }

    return ans;
}

void NVi::revU16(u16_t &x)
{
    x = x >> 8 | x << 8;
}

void NVi::revU32(u32_t &x)
{
    x = x >> 24 | (x & 0xFF0000) >> 8 | (x & 0xFF00) << 8 | x << 24;
}

void NVi::UpdateFileInfo(const std::string& file_path)
{
    if (!current_file_info) {
        current_file_info = new NVmidiFileInfo();
    }
    
    if (current_file_info->extract_info(file_path.c_str())) {
        current_midi_file = file_path;
        info("Utils", "File info updated for: %s\n", file_path.c_str());
        LogToFile("FILE_INFO", "Successfully extracted info for: %s\n", file_path.c_str());
    } else {
        error("Utils", "Failed to extract file info for: %s\n", file_path.c_str());
        LogToFile("FILE_INFO", "Failed to extract info for: %s\n", file_path.c_str());
    }
}

void NVi::LogToFile(const char* prefix, const char* str, ...)
{
    // Check if logging is enabled
    if (!parsed_config.enable_file_logging) {
        return;
    }
    
    static FILE* log_file = nullptr;
    static bool log_initialized = false;
    
    if (!log_initialized) {
#ifdef NON_ANDROID
        log_file = fopen("nvpfa.log", "a");
#else
        log_file = fopen("/data/data/com.qsp.nvpfa/files/nvpfa.log", "a");
#endif
        log_initialized = true;
    }
    
    if (log_file) {
        // Get current time
        time_t now = time(0);
        struct tm* timeinfo = localtime(&now);
        
        // Write timestamp and message
        fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ", 
                timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, prefix);
        
        va_list args;
        va_start(args, str);
        vfprintf(log_file, str, args);
        va_end(args);
        
        fflush(log_file);
    }
}

void NVi::ReloadMidiList()
{
    LogToFile("RELOAD", "Reloading MIDI list...\n");
    CreateMidiList();
    info("Utils", "MIDI list reloaded\n");
}

void NVi::ReloadSoundfontList()
{
    LogToFile("RELOAD", "Reloading soundfont list...\n");
    RefreshSFList();
    info("Utils", "Soundfont list reloaded\n");
}

void NVi::ReloadAllResources()
{
    LogToFile("RELOAD", "Reloading all resources...\n");
    ReloadMidiList();
    ReloadSoundfontList();
    info("Utils", "All resources reloaded\n");
}

void NVi::LoadDefaultGMSoundfont()
{
    if (parsed_config.use_default_gm_soundfont) {
        std::string gm_path = NVConf::GetDefaultGMSoundfontPath();
        LogToFile("SOUNDFONT", "Loading default GM soundfont: %s\n", gm_path.c_str());
        info("Utils", "Loading GM soundfont: %s\n", gm_path.c_str());
        
        // Here you would typically load the soundfont into your audio system
        // For now, we'll just log that it's being loaded
        // In a real implementation, you would call your audio library's soundfont loading function
    }
}

bool NVi::CheckGMSoundfontExists()
{
    std::string gm_path = NVConf::GetDefaultGMSoundfontPath();
    
    // Check if file exists
    FILE* file = fopen(gm_path.c_str(), "rb");
    if (file) {
        fclose(file);
        LogToFile("SOUNDFONT", "GM soundfont found: %s\n", gm_path.c_str());
        return true;
    } else {
        LogToFile("SOUNDFONT", "GM soundfont not found: %s\n", gm_path.c_str());
        return false;
    }
}
