#pragma once
#define NOMINMAX // これを入れると Windows の max マクロが無効になる
#include <Windows.h>
#include <algorithm> // std::max が有効になる
#include "KamataEngine.h"
#include <cassert>
#include <numbers>
#include <cmath>
#include "Vector.h"
#include <algorithm>
#include <algorithm>


using namespace KamataEngine;

static inline const float kAcceleration = 0.05f; // プレイヤーの加速度

static inline const float kAttenuation = 0.1f; // プレイヤーの減衰率f; 

static inline const float kLimitRunSpeed = 0.5f;

static inline const float kTimeTurn = 0.3f;

static inline const float kGravityAcceleration = 0.01f; // 重力加速度
static inline const float kLimitFallSpeed = 0.5f;       // 限界落下速度
static inline const float kJumpAcceleration = 0.3f;     // ジャンプ加速度

enum class LRDirection
{
	kRight,
	kLeft,
};

class Player {
private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	uint32_t textureHandle_ = 0;
	Vector3 velocity_ = {};
	bool onGround_ = true;
	bool landing = false;

	float turnFirstRotationY_ = 0.0f; // 初回の回転角度
	float turnTimer_ = 0.0f;          // 回転タイマー

	float EaseInOut(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }


	public:
	const Vector3& GetPosition() const { return worldTransform_.translation_; }
	const Vector3& GetVelocity() const { return velocity_; }

	LRDirection lrDirection_ = LRDirection::kRight;


public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(KamataEngine::Model* model,KamataEngine::Camera* camera,const Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update(float deltaTime);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

};
