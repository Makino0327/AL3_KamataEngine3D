#pragma once
#include <Windows.h>

namespace Util {

struct MouseUtil {
	// クライアント座標を取得（float）
	static bool GetClientPos(float& x, float& y, HWND hwnd = nullptr);

	// 左クリックの「押した瞬間」
	static bool LeftClickTriggered();
};

// ★あなたが使うインターフェース（自由関数）
bool GetCursorPosition(int& x, int& y, HWND hwnd = nullptr);

} // namespace Util
