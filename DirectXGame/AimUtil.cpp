#include "AimUtil.h"
#include <cmath>
using KamataEngine::Camera;
using KamataEngine::Matrix4x4;
using KamataEngine::Vector3;

// ヘルパは無名名前空間に置いて外部へ漏らさない
namespace {
inline Vector3 Normalize_(const Vector3& v) {
	float s = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (s <= 1e-8f)
		return {0, 0, 0};
	return {v.x / s, v.y / s, v.z / s};
}
inline Matrix4x4 Mul_(const Matrix4x4& a, const Matrix4x4& b) {
	Matrix4x4 r{};
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
	return r;
}
inline float Det3_(float a00, float a01, float a02, float a10, float a11, float a12, float a20, float a21, float a22) {
	return a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) + a02 * (a10 * a21 - a11 * a20);
}
Matrix4x4 Inverse_(const Matrix4x4& m) {
	Matrix4x4 r{};
	float c00 = Det3_(m.m[1][1], m.m[1][2], m.m[1][3], m.m[2][1], m.m[2][2], m.m[2][3], m.m[3][1], m.m[3][2], m.m[3][3]);
	float c01 = -Det3_(m.m[1][0], m.m[1][2], m.m[1][3], m.m[2][0], m.m[2][2], m.m[2][3], m.m[3][0], m.m[3][2], m.m[3][3]);
	float c02 = Det3_(m.m[1][0], m.m[1][1], m.m[1][3], m.m[2][0], m.m[2][1], m.m[2][3], m.m[3][0], m.m[3][1], m.m[3][3]);
	float c03 = -Det3_(m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2], m.m[3][0], m.m[3][1], m.m[3][2]);
	float det = m.m[0][0] * c00 + m.m[0][1] * c01 + m.m[0][2] * c02 + m.m[0][3] * c03;
	if (std::fabs(det) < 1e-8f)
		return r;
	float invDet = 1.0f / det;
	r.m[0][0] = c00 * invDet;
	r.m[0][1] = -Det3_(m.m[0][1], m.m[0][2], m.m[0][3], m.m[2][1], m.m[2][2], m.m[2][3], m.m[3][1], m.m[3][2], m.m[3][3]) * invDet;
	r.m[0][2] = Det3_(m.m[0][1], m.m[0][2], m.m[0][3], m.m[1][1], m.m[1][2], m.m[1][3], m.m[3][1], m.m[3][2], m.m[3][3]) * invDet;
	r.m[0][3] = -Det3_(m.m[0][1], m.m[0][2], m.m[0][3], m.m[1][1], m.m[1][2], m.m[1][3], m.m[2][1], m.m[2][2], m.m[2][3]) * invDet;
	r.m[1][0] = c01 * invDet;
	r.m[1][1] = Det3_(m.m[0][0], m.m[0][2], m.m[0][3], m.m[2][0], m.m[2][2], m.m[2][3], m.m[3][0], m.m[3][2], m.m[3][3]) * invDet;
	r.m[1][2] = -Det3_(m.m[0][0], m.m[0][2], m.m[0][3], m.m[1][0], m.m[1][2], m.m[1][3], m.m[3][0], m.m[3][2], m.m[3][3]) * invDet;
	r.m[1][3] = Det3_(m.m[0][0], m.m[0][2], m.m[0][3], m.m[1][0], m.m[1][2], m.m[1][3], m.m[2][0], m.m[2][2], m.m[2][3]) * invDet;
	r.m[2][0] = c02 * invDet;
	r.m[2][1] = -Det3_(m.m[0][0], m.m[0][1], m.m[0][3], m.m[2][0], m.m[2][1], m.m[2][3], m.m[3][0], m.m[3][1], m.m[3][3]) * invDet;
	r.m[2][2] = Det3_(m.m[0][0], m.m[0][1], m.m[0][3], m.m[1][0], m.m[1][1], m.m[1][3], m.m[3][0], m.m[3][1], m.m[3][3]) * invDet;
	r.m[2][3] = -Det3_(m.m[0][0], m.m[0][1], m.m[0][3], m.m[1][0], m.m[1][1], m.m[1][3], m.m[2][0], m.m[2][1], m.m[2][3]) * invDet;
	r.m[3][0] = c03 * invDet;
	r.m[3][1] = Det3_(m.m[0][0], m.m[0][1], m.m[0][2], m.m[2][0], m.m[2][1], m.m[2][2], m.m[3][0], m.m[3][1], m.m[3][2]) * invDet;
	r.m[3][2] = -Det3_(m.m[0][0], m.m[0][1], m.m[0][2], m.m[1][0], m.m[1][1], m.m[1][2], m.m[3][0], m.m[3][1], m.m[3][2]) * invDet;
	r.m[3][3] = Det3_(m.m[0][0], m.m[0][1], m.m[0][2], m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2]) * invDet;
	return r;
}
inline Vector3 TransformCoord_(const Vector3& v, const Matrix4x4& m) {
	float x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0];
	float y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1];
	float z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2];
	float w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + m.m[3][3];
	if (std::fabs(w) > 1e-8f) {
		x /= w;
		y /= w;
		z /= w;
	}
	return {x, y, z};
}
} // namespace

// ★定義は Util:: を付ける（ここが今回のエラー原因）
KamataEngine::Vector3 Util::MakeShootDirFromMouse(float mouseX, float mouseY, const KamataEngine::Camera& cam, int viewportW, int viewportH, const KamataEngine::Vector3& muzzleWorld) {
	using KamataEngine::Matrix4x4;
	using KamataEngine::Vector3;

	// 1) NDC
	float ndcX = (mouseX / static_cast<float>(viewportW)) * 2.0f - 1.0f;
	float ndcY = -(mouseY / static_cast<float>(viewportH)) * 2.0f + 1.0f;

	// 2) クリップ空間上の Near/Far
	Vector3 clipNear{ndcX, ndcY, 0.0f};
	Vector3 clipFar{ndcX, ndcY, 1.0f};

	// 3) ワールドへ（VP逆）
	// ※エンジンの行列順序により、ここの順番が逆の可能性あり。
	//   弾が妙な方向なら Mul_ の順を入れ替えて試してください。
	Matrix4x4 vp = Mul_(cam.matView, cam.matProjection);
	// Matrix4x4 vp = Mul_(cam.matProjection, cam.matView); // ←こちらが正しい場合もある
	Matrix4x4 invVP = Inverse_(vp);

	Vector3 worldNear = TransformCoord_(clipNear, invVP);
	Vector3 worldFar = TransformCoord_(clipFar, invVP);

	// 4) レイ = worldNear + t*(worldFar - worldNear)
	Vector3 rayDir{worldFar.x - worldNear.x, worldFar.y - worldNear.y, worldFar.z - worldNear.z};

	// 5) 平面 z = muzzleWorld.z との交点
	float dz = rayDir.z;
	if (std::fabs(dz) < 1e-6f) {
		// ほぼ平行：z を固定してスクリーン上の点から作る fallback
		Vector3 to = worldNear;
		to.z = muzzleWorld.z;
		Vector3 dir{to.x - muzzleWorld.x, to.y - muzzleWorld.y, 0.0f};
		return Normalize_(dir);
	}
	float t = (muzzleWorld.z - worldNear.z) / dz;

	// 6) 交点（マウスが指す “銃口と同じ z 面” 上の点）
	Vector3 hit{worldNear.x + rayDir.x * t, worldNear.y + rayDir.y * t, muzzleWorld.z};

	// 7) そこへの正規化方向（z 成分は 0 なので平面内に収まる）
	Vector3 dir{hit.x - muzzleWorld.x, hit.y - muzzleWorld.y, 0.0f};
	return Normalize_(dir);
}
