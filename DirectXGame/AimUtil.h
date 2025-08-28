#pragma once
#include "KamataEngine.h" // Camera / Matrix4x4 / Vector3

namespace Util {

KamataEngine::Vector3 MakeShootDirFromMouse(float mouseX, float mouseY, const KamataEngine::Camera& cam, int viewportW, int viewportH, const KamataEngine::Vector3& muzzleWorld);

} // namespace Util
