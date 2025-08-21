#pragma once
#define NOMINMAX // これを入れると Windows の max マクロが無効になる
#include <Windows.h>
#include <algorithm> // std::max が有効になる
#include "KamataEngine.h"
#include <cassert>
#include <numbers>
#include <cmath>
#include "Vector.h"
#include "MapChipField.h"


using namespace KamataEngine;
static inline const float kAcceleration = 0.01f; // プレイヤーの加速度
static inline const float kAttenuation = 0.1f; // プレイヤーの減衰率f; 
static inline const float kLimitRunSpeed = 0.4f;
static inline const float kTimeTurn = 0.3f;
static inline const float kGravityAcceleration = 0.015f; // 重力加速度
static inline const float kLimitFallSpeed = 0.5f;       // 限界落下速度
static inline const float kJumpAcceleration = 0.4f;     // ジャンプ加速度
static inline const float kWidth = 1.99f;
static inline const float kHeight = 1.99f; // プレイヤーの高さ
static inline const float kAttenuationLanding = 0.2f; // 例えば20%摩擦
static inline const float kGroundingOffsetY = -0.05f; // 微小なマイナス値
const float modelHeight = 2.0f;                       // 壁接触時の減衰率（例: 20% 減衰）
static inline const float kAttenuationWall = 0.2f;


class Enemy;
enum class LRDirection
{
	kRight,
	kLeft,
};

enum class BehaviorState { kRoot, kAttack };

// マップとの衝突判定情報
struct CollisionInfo {
	bool isHitTop = false;
	bool isHitBottom = false;
	bool isHitLeft = false;
	bool isHitRight = false;
	Vector3 move; // 最終的な移動量
};

enum Corner
{
	kRightBottom,
	kLeftBottom,
	kRightTop,
	kLeftTop,

	kNumCorner
};

class MapChipField;

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
	// マップチップによるフィールド
	MapChipField* mapChipField_ = nullptr;

	public:
	const Vector3& GetPosition() const { return worldTransform_.translation_; }
	const Vector3& GetVelocity() const { return velocity_; }

	LRDirection lrDirection_ = LRDirection::kRight;

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	bool isDead_ = false;
	float modelYOffset_ = 1.0f; // モデル中心 → 足元 までの補正値

	
	BehaviorState behaviorState_ = BehaviorState::kRoot;

	float attackTimer_ = 0.0f;
	static constexpr float kAttackDuration_ = 0.30f; // 攻撃の持続秒
	float attackScaleTimer_ = 0.0f;
	const float kAttackScaleDuration = 0.2f; // 縮む時間（秒）
	                                         // --- Double Jump ---
	int jumpCount_ = 0;                      // 実行済みジャンプ回数
	int maxJumps_ = 2;                       // 上限（2で二段ジャンプ）

	// --- Spin on 2nd jump ---
	bool spinActive_ = false;
	float spinTimer_ = 0.0f;
	float spinDuration_ = 0.35f; // 一回転にかける時間（好みで調整）
	float spinStartX_ = 0.0f;    // 開始時のX回転角（戻し用）
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

	void SetMapChipField(MapChipField* mapChipField) {
		assert(mapChipField != nullptr);
		mapChipField_ = mapChipField;
	}

	void InputMove(float deltaTime);

	void CheckCollisionMap(CollisionInfo& info);

	void CheckCollisionMapTop(CollisionInfo& info);
	void CheckCollisionMapBottom(CollisionInfo& info);
	void CheckCollisionMapLeft(CollisionInfo& info);
	void CheckCollisionMapRight(CollisionInfo& info);

	Vector3 CornerPosition(const Vector3& center, Corner corner);
	void ApplyCollisionResult(const CollisionInfo& info);
	void CheckHitCeiling(const CollisionInfo& info);
	void ChangeGroundState(const CollisionInfo& info);
	// 壁に接触している場合の処理
	void ProcessWallCollision(const CollisionInfo& info);
	Vector3 GetWorldPosition();
	// AABBを取得
	AABB GetAABB();
	void OnCollision();
	bool IsDead() const { return isDead_; }
	WorldTransform& GetWorldTransform() { return worldTransform_; }

	void BehaviorRootUpdate(float deltaTime);

	// 攻撃行動更新
	void BehaviorAttackUpdate();
};
