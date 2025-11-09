
#pragma once


#include <complex>
#include <SDL3/SDL.h>

extern const unsigned char KeyMap[128];

class Canvas
{
public:

    using color = std::complex<float>;

    struct RGBA_pix
    {
        unsigned char r, g, b, a;
    };

    SDL_Window   *Win;
    SDL_Renderer *Ren;
    //SDL_Texture  *GuiLayer;
	const SDL_DisplayMode *mod;
    int   TH, WinW, WinH;
    bool  KeyPress[128];
    unsigned int KeyColor[128];
    float KeyAlpha[128];
    bool KeyJustHit[128];

    Canvas(); 
    ~Canvas();

    void canvas_clear();

    void DrawKeyBoard();
    void DrawVerticalLines();
    void DrawBackgroundImage();

    void Note(int k, int yb, int ye, unsigned int c);
    unsigned int GetNoteColor(int note_number);
    bool LoadBackgroundImage(const std::string& image_path);
    bool IsNoteInRange(int note_number);
    void OptimizeRendering();
    float GetFrameRate();
    
    // Note counting methods
    bool isNoteCounted(int key) const { return NoteCounted[key]; }
    void setNoteCounted(int key, bool value) { NoteCounted[key] = value; }
    void incrementNoteCount() { note_hits_count++; }
    unsigned int getNoteHitsCount() const { return note_hits_count; }

private:

    SDL_Texture *Bk0, *Bk1, *Wk, *note, *background_texture;
    SDL_Surface *colors;

    int BkeyW, BkeyH, WkeyW, WkeyH;
    int TW, TX[11], KeyX[128];
    
    // Performance monitoring
    float frame_rate;
    float last_frame_time;
    int frame_count;
    float fps_update_timer;

    // Note counter: total notes hit since app start
    int note_hits_count;
    bool NoteCounted[128];
    unsigned int GetNoteCount();
    bool isNoteCounted(int key) const { return NoteCounted[key]; }
    void setNoteCounted(int key, bool counted) { NoteCounted[key] = counted; }
    void incrementNoteCount() { note_hits_count++; }

    int scale(int x) const;

    RGBA_pix getColor(color C) const;
};
