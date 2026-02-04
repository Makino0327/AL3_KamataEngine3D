#pragma once
#include "KamataEngine.h" // Matrix4x4, Vector3
#include <cmath>

namespace MyMath {

// 4x4 単位行列
KamataEngine::Matrix4x4 Identity();

// 4x4 行列積 r = a * b
KamataEngine::Matrix4x4 Mul(const KamataEngine::Matrix4x4& a, const KamataEngine::Matrix4x4& b);

// 4x4 逆行列（一般行列用：ガウス・ジョルダン法）
KamataEngine::Matrix4x4 Inverse(const KamataEngine::Matrix4x4& m);

// 3Dベクトル正規化（長さ0のときはそのまま返す）
KamataEngine::Vector3 Normalize3(const KamataEngine::Vector3& v);

 KamataEngine::Vector4 Transform4(const KamataEngine::Vector4& v, const KamataEngine::Matrix4x4& m);

} // namespace MyMath
