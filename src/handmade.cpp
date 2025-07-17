#include <cstdint>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef uint8_t u8;
typedef uint32_t u32;

#define GetMessageAll(msg) GetMessage(msg, 0, 0, 0)

static BITMAPINFO BitmapInfo = {
	.bmiHeader = {
		.biSize = sizeof(BITMAPINFOHEADER),
		.biPlanes = 1,
		.biBitCount = 32,
		.biCompression = BI_RGB,
	}
};
static void* BitmapMemory;
static HBITMAP BitmapHandle; 
static HDC BitmapDeviceContext;
static int BitmapWidth;
static int BitmapHeight;
static bool Running = true;
// DIB - Device Independent Bitmap
void Win32ResizeDIBSection(int Width, int Height)
{
	if (BitmapMemory)
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}
	// bitmap memory size: width * height * bytes_per_pixel
	int BytesPerPixel = 4;
	BitmapMemory = VirtualAlloc(0, (size_t)(Width * Height * BytesPerPixel), MEM_COMMIT, PAGE_READWRITE);

	// NOTE: Negative height makes it so that (0, 0) is in upper left not bottom left
	BitmapInfo.bmiHeader.biHeight = -Height;
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapHeight = Height;
	BitmapWidth = Width;

	u8* Row = (u8*)BitmapMemory;
	int Pitch = Width * BytesPerPixel;
	for (int Y = 0;Y < BitmapHeight;++Y)
	{
		u8* Pixel = (u8*)Row;
		for (int X = 0;X < BitmapWidth;++X)
		{
			// blue
			*Pixel = (u8)X;
			Pixel += 1;
			// green
			*Pixel = (u8)Y;
			Pixel += 1;
			//red
			*Pixel = 0;
			Pixel += 1;
			// padding
			Pixel += 1;
		}
		Row += Pitch;
	}
	
}

void Win32WindowUpdate(HDC DeviceContext, RECT* WindowRect)
{
	/*StretchDIBits(
		DeviceContext,
		X, Y, W, H,
		X, Y, W, H,
		&BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
	*/	
	int WindowWidth = WindowRect->right - WindowRect->left;
	int WindowHeight = WindowRect->bottom - WindowRect->top;
	StretchDIBits(DeviceContext,
			   0, 0, BitmapWidth, BitmapHeight,
			   0, 0, WindowWidth, WindowHeight,
			   BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			Win32ResizeDIBSection(Width, Height);
		}
			break;
		case WM_DESTROY:
			Running = false;
			break;
		case WM_CLOSE:
			Running = false;
			break;
		case WM_ACTIVATEAPP:
			OutputDebugStringA("WM_ACTIVATEAPP\n");
			break;
		case WM_PAINT: {
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			PatBlt(DeviceContext, X, Y, Width, Height, BLACKNESS);
			Win32WindowUpdate(DeviceContext, &ClientRect);
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

	HWND WindowHandle = CreateWindowExA(0, 
									WindowClass.lpszClassName, 
									"Handmade Hero", 
									WS_OVERLAPPEDWINDOW | WS_VISIBLE,
									CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
									0, 0, Instance, 0);
	if (!WindowHandle)
	{
		// TODO: im to lazy to report this it's not gonna fail
		return 1;
	}

	while (Running)
	{
		MSG Message;
		BOOL MessageResult = GetMessageAll(&Message);
		if (MessageResult <= 0) break;

		// NOTE: These functions can fail but if windows decides not to handle the messeges
		// there is not much to be done so yeah
		TranslateMessage(&Message);
		DispatchMessageA(&Message);	

	}
	return 0;
}
