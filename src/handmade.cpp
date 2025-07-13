#include <windows.h>



#define GetAllMessages(msg) GetMessage(msg, 0, 0, 0)

LRESULT CALLBACK MainWindowCallback(
  HWND Window,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam 
)
{
	LRESULT Result = 0;
	switch (Message)
	{
		case WM_SIZE:
			OutputDebugStringA("WM_SIZE\n");
			break;
		case WM_DESTROY:
			OutputDebugStringA("WM_DESTROY\n");
			break;
		case WM_CLOSE:
			OutputDebugStringA("WM_CLOSE\n");
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
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.bottom;
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
		.lpfnWndProc = MainWindowCallback,
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
	MSG Message;

	for (;;)
	{
		BOOL MessageResult = GetAllMessages(&Message);
		if (MessageResult > 0)
		{
			// NOTE: These functions can fail but if windows decides not to handle the messeges
			// there is not much to be done so yeah
			TranslateMessage(&Message);
			DispatchMessage(&Message);	
		}
		else 
		{
			break;
		}

	}
	return 0;
}
