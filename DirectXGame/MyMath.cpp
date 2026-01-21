#include "MyMath.h"
#include "Vector.h"

using KamataEngine::Matrix4x4;
using KamataEngine::Vector3;

namespace MyMath {

Matrix4x4 Identity() {
	Matrix4x4 r{};
	r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.0f;
	return r;
}

Matrix4x4 Mul(const Matrix4x4& a, const Matrix4x4& b) {
	Matrix4x4 r{};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j] + a.m[i][3] * b.m[3][j];
		}
	}
	return r;
}

Matrix4x4 Inverse(const Matrix4x4& m) {
	Matrix4x4 a = m;
	Matrix4x4 inv = Identity();

	// ガウス・ジョルダン（部分ピボット付き）
	for (int i = 0; i < 4; ++i) {
		int pivot = i;
		float maxAbs = std::fabs(a.m[i][i]);
		for (int r = i + 1; r < 4; ++r) {
			float v = std::fabs(a.m[r][i]);
			if (v > maxAbs) {
				maxAbs = v;
				pivot = r;
			}
		}
		if (maxAbs < 1e-8f) {
			// 非可逆：保険で単位行列返し
			return Identity();
		}
		if (pivot != i) {
			for (int c = 0; c < 4; ++c) {
				std::swap(a.m[i][c], a.m[pivot][c]);
				std::swap(inv.m[i][c], inv.m[pivot][c]);
			}
		}
		float diag = a.m[i][i];
		float invDiag = 1.0f / diag;
		for (int c = 0; c < 4; ++c) {
			a.m[i][c] *= invDiag;
			inv.m[i][c] *= invDiag;
		}
		for (int r = 0; r < 4; ++r) {
			if (r == i)
				continue;
			float f = a.m[r][i];
			if (f == 0.0f)
				continue;
			for (int c = 0; c < 4; ++c) {
				a.m[r][c] -= f * a.m[i][c];
				inv.m[r][c] -= f * inv.m[i][c];
			}
		}
	}
	return inv;
}

Vector3 Normalize3(const Vector3& v) {
	float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
	if (len2 <= 1e-12f)
		return v;
	float invLen = 1.0f / std::sqrt(len2);
	return Vector3{v.x * invLen, v.y * invLen, v.z * invLen};
}

Vector4 Transform4(const KamataEngine::Vector4& v, const KamataEngine::Matrix4x4& m) {
	KamataEngine::Vector4 r{};
	r.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + v.w * m.m[3][0];
	r.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + v.w * m.m[3][1];
	r.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + v.w * m.m[3][2];
	r.w = v.x * m.m[0][3] + v.y * m.m[1][3] + v.z * m.m[2][3] + v.w * m.m[3][3];
	return r;
}


} // namespace MyMath

