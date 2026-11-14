#pragma once
// Linux SDL2 ABI subset used by this program. See SDL2's public SDL_video.h,
// SDL_events.h, SDL_audio.h and SDL_keyboard.h. SPDX-License-Identifier: Zlib
#include <cstdint>
using U8 = uint8_t;
using U16 = uint16_t;
using U32 = uint32_t;
struct SDL_Window;
struct SDL_GameController;
struct SDL_Rect { int x,y,w,h; };
struct SDL_AudioSpec {
    int freq; U16 format; U8 channels,silence; U16 samples,padding; U32 size;
    void (*callback)(void*,U8*,int); void* userdata;
};
struct SDL_Keysym { int scancode,sym; U16 mod; U32 unused; };
struct SDL_KeyboardEvent { U32 type,timestamp,windowID; U8 state,repeat,padding2,padding3; SDL_Keysym keysym; };
struct SDL_TextInputEvent { U32 type,timestamp,windowID; char text[32]; };
struct SDL_WindowEvent { U32 type,timestamp,windowID; U8 event,p1,p2,p3; int data1,data2; };
struct SDL_MouseMotionEvent { U32 type,timestamp,windowID,which,state; int x,y,xrel,yrel; };
struct SDL_MouseButtonEvent { U32 type,timestamp,windowID,which; U8 button,state,clicks,padding; int x,y; };
struct SDL_MouseWheelEvent { U32 type,timestamp,windowID,which; int x,y; U32 direction; };
struct SDL_ControllerDeviceEvent { U32 type,timestamp; int which; };
struct SDL_ControllerButtonEvent { U32 type,timestamp; int which; U8 button,state,padding1,padding2; };
union SDL_Event {
    U32 type; SDL_KeyboardEvent key; SDL_WindowEvent window;
    SDL_TextInputEvent text;
    SDL_MouseMotionEvent motion; SDL_MouseButtonEvent button; SDL_MouseWheelEvent wheel;
    SDL_ControllerDeviceEvent cdevice; SDL_ControllerButtonEvent cbutton; void* alignment; U8 padding[56];
};
static_assert(sizeof(SDL_Event)==56);
extern "C" {
int SDL_Init(U32);
void SDL_Quit();
const char* SDL_GetError();
int SDL_GL_SetAttribute(int,int);
SDL_Window* SDL_CreateWindow(const char*,int,int,int,int,U32);
void SDL_DestroyWindow(SDL_Window*);
void SDL_SetWindowMinimumSize(SDL_Window*,int,int);
U32 SDL_GetWindowFlags(SDL_Window*);
int SDL_SetWindowFullscreen(SDL_Window*,U32);
void SDL_GetWindowSize(SDL_Window*,int*,int*);
void SDL_GL_GetDrawableSize(SDL_Window*,int*,int*);
void* SDL_GL_CreateContext(SDL_Window*);
void SDL_GL_DeleteContext(void*);
void* SDL_GL_GetProcAddress(const char*);
int SDL_GL_SetSwapInterval(int);
void SDL_GL_SwapWindow(SDL_Window*);
int SDL_PollEvent(SDL_Event*);
U32 SDL_GetMouseState(int*,int*);
const U8* SDL_GetKeyboardState(int*);
const char* SDL_GetScancodeName(int);
void SDL_StartTextInput();
void SDL_StopTextInput();
int SDL_SetClipboardText(const char*);
char* SDL_GetClipboardText();
void SDL_free(void*);
int SDL_NumJoysticks();
int SDL_IsGameController(int);
SDL_GameController* SDL_GameControllerOpen(int);
void SDL_GameControllerClose(SDL_GameController*);
int SDL_GameControllerGetAttached(SDL_GameController*);
U8 SDL_GameControllerGetButton(SDL_GameController*,int);
short SDL_GameControllerGetAxis(SDL_GameController*,int);
int SDL_ShowCursor(int);
void SDL_Delay(U32);
U32 SDL_OpenAudioDevice(const char*,int,const SDL_AudioSpec*,SDL_AudioSpec*,int);
void SDL_PauseAudioDevice(U32,int);
void SDL_CloseAudioDevice(U32);
void SDL_LockAudioDevice(U32);
void SDL_UnlockAudioDevice(U32);
}
namespace vl::sdl {
constexpr U32 video=0x20,audio=0x10,gameController=0x2000, openGL=2,hidden=8,resizable=0x20, highDPI=0x2000, fullscreen=0x1001;
constexpr int centered=0x2fff0000;
constexpr U32 quit=0x100,window=0x200,keyDown=0x300,keyUp=0x301,textInput=0x303,mouseMotion=0x400,mouseDown=0x401,mouseUp=0x402,
    mouseWheel=0x403,controllerButton=0x650,controllerAdded=0x653,controllerRemoved=0x654;
constexpr U8 focusLost=13;
constexpr U32 inputFocus=0x00000200;
// USB HID scancodes as defined by SDL_scancode.h.
namespace key {
constexpr int a=4,c=6,d=7,e=8,f=9,j=13,k=14,p=19,q=20,r=21,s=22,v=25,w=26,x=27,y=28,z=29,
    one=30,two=31,three=32,four=33,five=34,six=35,seven=36,enter=40,esc=41,backspace=42,tab=43,space=44,f3=60,f11=68,
    right=79,left=80,down=81,up=82,controlLeft=224,shiftLeft=225,shiftRight=229,controlRight=228;
}
constexpr int scancodes=512;
constexpr U32 buttonLeft=1,buttonMiddle=2,buttonRight=4;
namespace pad {
constexpr int a=0,b=1,x=2,y=3,start=6,stick=7,leftShoulder=9,rightShoulder=10,up=11,down=12,left=13,right=14;
constexpr int leftX=0,leftY=1,rightX=2,rightY=3,leftTrigger=4,rightTrigger=5;
}
}
