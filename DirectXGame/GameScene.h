#pragma once
#include "KamataEngine.h"
#include <vector>
#include <cmath>


using KamataEngine::Matrix4x4;
using KamataEngine::Vector3;

class GameScene {
public:
	// 3Dモデルデータ
	KamataEngine::Model* model_ = nullptr;
	// カメラ
	KamataEngine::Camera camera_;
	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;
	// ワールドトランスフォーム
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	// テクスチャハンドル
	uint32_t block_ = 0;
	// デバックカメラ有効
	bool isDebugCameraActive_ = false;
	// デバックカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	public:

	void Initialize();

	void Update();

	void Draw();

	~GameScene();

	// スケール行列を作る
	Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
		Matrix4x4 result = {};

		result.m[0][0] = scale.x;
		result.m[1][1] = scale.y;
		result.m[2][2] = scale.z;
		result.m[3][3] = 1.0f;

		return result;
	}

	// 回転行列（Z軸回転）
	Matrix4x4 MakeRotateZMatrix(float angle) {
		Matrix4x4 result = {};

		float cosA = cosf(angle);
		float sinA = sinf(angle);

		result.m[0][0] = cosA;
		result.m[0][1] = sinA;
		result.m[1][0] = -sinA;
		result.m[1][1] = cosA;
		result.m[2][2] = 1.0f;
		result.m[3][3] = 1.0f;

		return result;
	}

	// 回転行列（X軸回転）
	Matrix4x4 MakeRotateXMatrix(float angle) {
		Matrix4x4 result = {};

		float cosA = cosf(angle);
		float sinA = sinf(angle);

		result.m[0][0] = 1.0f;
		result.m[1][1] = cosA;
		result.m[1][2] = sinA;
		result.m[2][1] = -sinA;
		result.m[2][2] = cosA;
		result.m[3][3] = 1.0f;

		return result;
	}

	// 回転行列（Y軸回転）
	Matrix4x4 MakeRotateYMatrix(float angle) {
		Matrix4x4 result = {};

		float cosA = cosf(angle);
		float sinA = sinf(angle);

		result.m[0][0] = cosA;
		result.m[0][2] = -sinA;
		result.m[1][1] = 1.0f;
		result.m[2][0] = sinA;
		result.m[2][2] = cosA;
		result.m[3][3] = 1.0f;

		return result;
	}

	// 平行移動行列を作る
	Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
		Matrix4x4 result = {};

		result.m[0][0] = 1.0f;
		result.m[1][1] = 1.0f;
		result.m[2][2] = 1.0f;
		result.m[3][3] = 1.0f;

		result.m[3][0] = translate.x;
		result.m[3][1] = translate.y;
		result.m[3][2] = translate.z;

		return result;
	}

	// 4x4行列同士の掛け算
	Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
		Matrix4x4 result = {};

		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				result.m[row][col] = m1.m[row][0] * m2.m[0][col] + m1.m[row][1] * m2.m[1][col] + m1.m[row][2] * m2.m[2][col] + m1.m[row][3] * m2.m[3][col];
			}
		}
		return result;
	}

	// アフィン行列を作る関数
	Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotation, const Vector3& translate) {
		// スケール行列
		Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

		// 回転行列（Z→X→Yの順番で回す）
		Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotation.z);
		Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotation.x);
		Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotation.y);

		Matrix4x4 rotationMatrix = Multiply(Multiply(rotateZMatrix, rotateXMatrix), rotateYMatrix);

		// 平行移動行列
		Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

		// 最後にすべてを合成
		Matrix4x4 worldMatrix = Multiply(Multiply(scaleMatrix, rotationMatrix), translateMatrix);

		return worldMatrix;
	}
};
