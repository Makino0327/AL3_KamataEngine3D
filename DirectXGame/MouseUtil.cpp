#include "MouseUtil.h"
#include "imgui.h"

namespace Util {

// --- MouseUtil 実装（そのまま） ---
bool MouseUtil::GetClientPos(float& x, float& y, HWND hwnd) {
	POINT p{};
	if (!GetCursorPos(&p))
		return false;
	if (!hwnd)
		hwnd = GetActiveWindow(); // 必要なら自分のHWNDに差し替え
	if (!hwnd)
		return false;
	if (!ScreenToClient(hwnd, &p))
		return false;
	x = static_cast<float>(p.x);
	y = static_cast<float>(p.y);
	return true;
}

bool MouseUtil::LeftClickTriggered() {
	static bool prevDown = false;
	bool nowDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	bool trig = (!prevDown && nowDown);
	prevDown = nowDown;
	return trig;
}

// --- 自由関数：Util::GetCursorPosition（呼び出し口を揃える） ---
// MouseUtil.cpp
bool GetCursorPosition(int& x, int& y, HWND hwnd) {
	// ★ ImGuiのコンテキストがあるときだけIOを読む
	if (ImGui::GetCurrentContext()) {
		ImVec2 p = ImGui::GetIO().MousePos;
		if (p.x >= 0.0f && p.y >= 0.0f) {
			x = static_cast<int>(p.x);
			y = static_cast<int>(p.y);
			return true;
		}
	}

	// フォールバック: Win32
	float fx = 0.0f, fy = 0.0f;
	if (MouseUtil::GetClientPos(fx, fy, hwnd)) {
		x = static_cast<int>(fx);
		y = static_cast<int>(fy);
		return true;
	}
	x = y = 0;
	return false;
}


} // namespace Util
