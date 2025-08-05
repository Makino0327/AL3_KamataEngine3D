#include "Vector.h"

using namespace KamataEngine;

Matrix4x4 IdentityMatrix() {
	Matrix4x4 result{};
	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;
	return result;
}

Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result = IdentityMatrix();
	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;
	return result;
}
Matrix4x4 MakeRotateXMatrix(float rad) {
	Matrix4x4 result = IdentityMatrix();
	result.m[1][1] = cosf(rad);
	result.m[1][2] = sinf(rad);
	result.m[2][1] = -sinf(rad);
	result.m[2][2] = cosf(rad);
	return result;
}

Matrix4x4 MakeRotateYMatrix(float rad) {
	Matrix4x4 result = IdentityMatrix();
	result.m[0][0] = cosf(rad);
	result.m[0][2] = -sinf(rad);
	result.m[2][0] = sinf(rad);
	result.m[2][2] = cosf(rad);
	return result;
}

Matrix4x4 MakeRotateZMatrix(float rad) {
	Matrix4x4 result = IdentityMatrix();
	result.m[0][0] = cosf(rad);
	result.m[0][1] = sinf(rad);
	result.m[1][0] = -sinf(rad);
	result.m[1][1] = cosf(rad);
	return result;
}

Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = IdentityMatrix();
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	return result;
}

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result{};
	for (int y = 0; y < 4; ++y) {
		for (int x = 0; x < 4; ++x) {
			result.m[y][x] = m1.m[y][0] * m2.m[0][x] + m1.m[y][1] * m2.m[1][x] + m1.m[y][2] * m2.m[2][x] + m1.m[y][3] * m2.m[3][x];
		}
	}
	return result;
}

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotation, const Vector3& translation) {
	Matrix4x4 scaleMat = MakeScaleMatrix(scale);
	Matrix4x4 rotZ = MakeRotateZMatrix(rotation.z);
	Matrix4x4 rotX = MakeRotateXMatrix(rotation.x);
	Matrix4x4 rotY = MakeRotateYMatrix(rotation.y);

	Matrix4x4 rot = Multiply(Multiply(rotZ, rotX), rotY); // Z→X→Y順（左手座標系でよく使われる）

	Matrix4x4 transMat = MakeTranslateMatrix(translation);

	// S * R * T の順で合成
	Matrix4x4 worldMatrix = Multiply(Multiply(scaleMat, rot), transMat);

	return worldMatrix;
}

bool IsCollisionAABB(const AABB& a, const AABB& b) { return a.min.x <= b.max.x && a.max.x >= b.min.x && a.min.y <= b.max.y && a.max.y >= b.min.y && a.min.z <= b.max.z && a.max.z >= b.min.z; }

Vector3 Transform(const Vector3& v, const Matrix4x4& m) {
	Vector3 result;
	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];
	return result;
}
