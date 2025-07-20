#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

void RenderWeirdGradient(win32_buffer Buffer, int XOffset, int YOffset)
{
	// WARNING: The function kinda arbitrarly assumes BytesPerPixel to be 4
	u8* Row = (u8*)Buffer.Memory;
	int Width = Buffer.Width;
	int Height = Buffer.Height;
	int Pitch = Buffer.Pitch;
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
			OutputDebugStringA("WM_ACTIVATEAPP\n");
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
	WNDCLASS WindowClass = {
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


		RenderWeirdGradient(GBuffer, XOffset, YOffset);
		HDC DeviceContext = GetDC(Window);
		win32_win_dimension Dim = Win32GetWindowDimensions(Window);
		Win32WindowCopyBuffer(GBuffer, DeviceContext, Dim.Width, Dim.Height);
		XOffset += 1;
		ReleaseDC(Window, DeviceContext);

	}
	return 0;
}
