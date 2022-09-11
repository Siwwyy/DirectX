#include "WindowContainer.h"


LRESULT WindowContainer::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//OutputDebugStringA("BLABLA\n");
	switch (uMsg)
	{
	case WM_KEYDOWN:
	{
		auto keycode = static_cast<unsigned char>(wParam);
		if (keyboard.IsKeysAutoRepeat())
		{
			keyboard.OnKeyPressed(keycode);
		}
		else
		{
			const bool wasPressed = lParam & 0x40000000; //30th bit is responsible for 'is pressed'
			if (!wasPressed)
			{
				keyboard.OnKeyPressed(keycode);
			}
		}
		return 0;
	}
	case WM_KEYUP:
	{
		auto keycode = static_cast<unsigned char>(wParam);
		keyboard.OnKeyReleased(keycode);
		return 0;
	}
	case WM_CHAR:
	{
		auto ch = static_cast<unsigned char>(wParam);
		if (keyboard.IsCharsAutoRepeat())
		{
			keyboard.OnChar(ch);
		}
		else
		{
			const bool wasPressed = lParam & 0x40000000;
			if (!wasPressed)
			{
				keyboard.OnChar(ch);
			}
		}
		return 0;
	}
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}