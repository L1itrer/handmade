#include <stdint.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


// NOTE: in the original handmade hero casey was loading xinput dynamically from a dll, there was that
// entire macro system, it was cool
// but i think nowadays it might beis simply overkill, his reasoning was that requirments listed in MSDN are sketchy
// and might require a very new windows 8 that no one has, and you want the program to run even
// when someone does not have this library
// so now everyone has windows 10 at the least, it seems that it is unnecessery. nevertheless,
// the idea is so cool that i will follow along anyway
#pragma warning(push, 0)
#include <Xinput.h>
#pragma warning(pop)

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef size_t usize;
#define GetMessageAll(msg) GetMessage(msg, 0, 0, 0)


#define global static
#define local_persist static

typedef struct win32_buffer{
	void* Memory;
	int Pitch;
	int Width;
	int Height;
	int BytesPerPixel;
	BITMAPINFO Info; 
	int padding; // NOTE: this exists so that msvc shuts up about padding
}win32_buffer;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return 0;
}
global x_input_get_state* XInputGetState_ = XInputGetStateStub;


#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return 0;
}
global x_input_set_state* XInputSetState_ = XInputSetStateStub;

#define LOAD_XINPUT_DLL
#ifdef LOAD_XINPUT_DLL

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_
#endif

static void Win32LoadInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary) XInputLibrary = LoadLibraryA("xinput1_3.dll");
	if (XInputLibrary)
	{
		XInputGetState_ = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState_ = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

global bool GRunning = true;
global win32_buffer GBuffer;

typedef struct win32_win_dimension {
	int Width;
	int Height;
}win32_win_dimension;

win32_win_dimension Win32GetWindowDimensions(HWND Window)
{
	RECT Rect;
	GetClientRect(Window, &Rect);
	win32_win_dimension Result = {
		.Width = Rect.right - Rect.left,
		.Height = Rect.bottom - Rect.top
	};
	return Result;
}

void RenderWeirdGradient(win32_buffer* Buffer, int XOffset, int YOffset)
{
	// WARNING: The function kinda arbitrarly assumes BytesPerPixel to be 4
	u8* Row = (u8*)Buffer->Memory;
	int Width = Buffer->Width;
	int Height = Buffer->Height;
	int Pitch = Buffer->Pitch;
	for (int Y = 0;Y < Height;++Y)
	{
		u8* Pixel = (u8*)Row;
		for (int X = 0;X < Width;++X)
		{
			// blue
			*Pixel = (u8)(X + XOffset);
			Pixel += 1;
			// green
			*Pixel = (u8)(Y + YOffset);
			Pixel += 1;
			// red
			*Pixel = 0;
			Pixel += 1;
			// padding
			Pixel += 1;
		}
		Row += Pitch;
	}
}


// DIB - Device Independent Bitmap
void Win32ResizeDIBSection(win32_buffer* Buffer, int Width, int Height)
{
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}
	// bitmap memory size: width * height * bytes_per_pixel
	Buffer->BytesPerPixel = 4;
	int BytesPerPixel = Buffer->BytesPerPixel;	
	Buffer->Memory = VirtualAlloc(0, (size_t)(Width * Height * BytesPerPixel), MEM_COMMIT, PAGE_READWRITE);

	// NOTE: Negative height makes it so that (0, 0) is in top left not bottom left
	Buffer->Info = {
		.bmiHeader = {
			.biSize = sizeof(BITMAPINFOHEADER),
			.biWidth = Width,
			.biHeight = -Height,
			.biPlanes = 1,
			.biBitCount = 32, .biCompression = BI_RGB,
		}
	};

	Buffer->Height = Height;
	Buffer->Width = Width;


	Buffer->Pitch = Width * BytesPerPixel;
}

void Win32WindowCopyBuffer(win32_buffer Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	StretchDIBits(DeviceContext,
			   0, 0, WindowWidth, WindowHeight,
			   0, 0, Buffer.Width, Buffer.Height,
			   Buffer.Memory, &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(
  HWND Window,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam)
{
	LRESULT Result = 0;
	switch (Message)
	{
		case WM_SIZE: {
		}
			break;
		case WM_DESTROY:
			GRunning = false;
			break;
		case WM_CLOSE:
			GRunning = false;
			break;
		case WM_ACTIVATEAPP:
			break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_KEYDOWN:
			{
				u32 VKCode = WParam;
				#define KEY_MESSAGE_WAS_DOWN_BIT (1 << 30)
				#define KEY_MESSAGE_IS_DOWN_BIT (1 << 31)
				bool WasDown = ((LParam & KEY_MESSAGE_WAS_DOWN_BIT) != 0);
				bool IsDown = ((LParam & KEY_MESSAGE_IS_DOWN_BIT) == 0);
				if (IsDown == WasDown) break;
				if (VKCode == VK_UP)
				{
					if (IsDown) OutputDebugStringA("UP: IsDown\n");
					if (WasDown) OutputDebugStringA("UP: WasDown\n");
				}
			}
			break;
		case WM_PAINT: {
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			win32_win_dimension Dim = Win32GetWindowDimensions(Window);
			Win32WindowCopyBuffer(GBuffer, DeviceContext, Dim.Width, Dim.Height);
			EndPaint(Window, &Paint);
		}
			break;
		default:
			Result = DefWindowProc(Window, Message, WParam, LParam);
			break;
	}
	return Result;
}

int CALLBACK WinMain(
	HINSTANCE Instance, 
	HINSTANCE PrevInstance, 
	LPSTR CommandLine, 
	int ShowCode)
{
	// NOTE: OWNDC style is overkill and unnececery, but
	// i'm still leaving it because it does not hurt
	// HREDRAW and VREDRAW forces windows to redraw entire
	// window when resizing
	WNDCLASSA WindowClass = {
		.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = Win32MainWindowCallback,
		.hInstance = Instance,
		.lpszClassName = "HandmadeHeroWindowClass"
	};

	if (!RegisterClassA(&WindowClass))
	{
		MessageBoxA(0, "Registering the class failed", "There is no God", MB_OK | MB_ICONSTOP);
		return 1;
	}

	HWND Window = CreateWindowExA(0, 
									WindowClass.lpszClassName, 
									"Handmade Hero", 
									WS_OVERLAPPEDWINDOW | WS_VISIBLE,
									CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
									0, 0, Instance, 0);
	if (!Window)
	{
		// TODO: im to lazy to report this it's not gonna fail
		return 1;
	}
	Win32ResizeDIBSection(&GBuffer, 1280, 720);
	#ifdef LOAD_XINPUT_DLL
	Win32LoadInput();
	#endif

	int XOffset = 0, YOffset = 0;
	while (GRunning)
	{

		MSG Message;
		while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
		{
			if (Message.message == WM_QUIT) GRunning = false;
			// NOTE: These functions can fail but if windows decides not to handle the messeges
			// there is not much to be done so yeah
			TranslateMessage(&Message);
			DispatchMessageA(&Message);	
		}

		for (DWORD ControllerIdx = 0; ControllerIdx < XUSER_MAX_COUNT; ++ControllerIdx)
		{
			XINPUT_STATE ControllerState;
			if (XInputGetState(ControllerIdx, &ControllerState) != ERROR_SUCCESS)
			{
				// NOTE: controller is unavailable
				continue;
			}
			// NOTE: The controller is plugged in
			// NOTE: ControllerState.dwPackerNumber says how many missed inputs there were
			XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;
			bool DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
			bool DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
			bool DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
			bool DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
			bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
			bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);	
			bool LShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
			bool RShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
			bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
			bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
			bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
			bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);


			i16 StickX = Pad->sThumbLX;
			i16 StickY = Pad->sThumbLY;

			XOffset += StickX >> 12;
			// NOTE: why do i have to subtract not add tho?
			YOffset -= StickY >> 12;
		}


		RenderWeirdGradient(&GBuffer, XOffset, YOffset);
		HDC DeviceContext = GetDC(Window);
		win32_win_dimension Dim = Win32GetWindowDimensions(Window);
		Win32WindowCopyBuffer(GBuffer, DeviceContext, Dim.Width, Dim.Height);
		ReleaseDC(Window, DeviceContext);

	}
	return 0;
}
