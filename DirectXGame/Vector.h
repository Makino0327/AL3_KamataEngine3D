#pragma once
#include <cmath>
#include "KamataEngine.h"
using namespace KamataEngine;

struct Rect {
	float left = 5.0f;
	float right = 1000.0f;
	float bottom = 5.0f;
	float top = 1000.0f;
};

struct AABB {
	Vector3 min;
	Vector3 max;
};



Matrix4x4 IdentityMatrix();

Matrix4x4 MakeScaleMatrix(const Vector3& scale);
Matrix4x4 MakeRotateXMatrix(float rad);

Matrix4x4 MakeRotateYMatrix(float rad);

Matrix4x4 MakeRotateZMatrix(float rad);

Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotation, const Vector3& translation);

inline Vector3 Add(const Vector3& v1, const Vector3& v2) { return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }

bool IsCollisionAABB(const AABB& a, const AABB& b);

Vector3 Transform(const Vector3& v, const Matrix4x4& m);