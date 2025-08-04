#pragma once
#include <Windows.h>
#include "KamataEngine.h"
#include "Vector.h"
#include <numbers>
#include <cmath>

static inline const float kWalkSpeed = 0.1f;
// Enemy.h 内（Enemyクラスの中に static で定義）
static inline const float kWalkMotionAngleStart = -0.25f;
static inline const float kWalkMotionAngleEnd = 0.25f; // 終了角
static inline const float kWalkMotionTime = 60.0f;       // アニメーション周期（フレーム数）


class Enemy 
{
public:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	uint32_t textureHandle_ = 0;
	Vector3 velocity_ = {};
	float walkTimer_ = 0.0f;

	
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const Vector3& position);
	void Update();
	void Draw();
	void SetTexture(uint32_t textureHandle);
};
