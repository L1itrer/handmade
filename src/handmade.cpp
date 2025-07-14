#include <windows.h>

static bool Running = true;

#define GetMessageAll(msg) GetMessage(msg, 0, 0, 0)

BITMAPINFO BitmapInfo = {
	.bmiHeader = {
		.biSize = sizeof(BITMAPINFOHEADER),
		.biPlanes = 1,
		.biBitCount = 32,
		.biCompression = BI_RGB,
		.biSizeImage = 0,
		.biXPelsPerMeter = 0,
		.biYPelsPerMeter = 0,
		.biClrUsed = 0,
		.biClrImportant = 0
	}
};
static void* BitmapMemory;
static HBITMAP BitmapHandle; 
static HDC BitmapDeviceContext;
// DIB - Device Independent Bitmap
void Win32ResizeDIBSection(int Width, int Height)
{
	if (BitmapHandle)
	{
		DeleteObject(BitmapHandle);
	}
	else
	{
		BitmapDeviceContext = CreateCompatibleDC(0);
	}

	
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biWidth = Width;
	HDC DeviceContext = CreateCompatibleDC(0);
	BitmapHandle = CreateDIBSection(BitmapDeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}

void Win32WindowUpdate(HDC DeviceContext, int X, int Y, int W, int H)
{
	StretchDIBits(
		DeviceContext,
		X, Y, W, H,
		X, Y, W, H,
		&BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
		
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
			Win32WindowUpdate(DeviceContext, X, Y, Width, Height);
			PatBlt(DeviceContext, X, Y, Width, Height, BLACKNESS);
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
