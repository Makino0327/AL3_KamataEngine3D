#pragma once
#include <cmath>
#include "KamataEngine.h"
using namespace KamataEngine;

Matrix4x4 IdentityMatrix();

Matrix4x4 MakeScaleMatrix(const Vector3& scale);
Matrix4x4 MakeRotateXMatrix(float rad);

Matrix4x4 MakeRotateYMatrix(float rad);

Matrix4x4 MakeRotateZMatrix(float rad);

Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotation, const Vector3& translation);