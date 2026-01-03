#pragma once

#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <numbers>

#include "KamataEngine.h"
#include "MapChipField.h"
#include "Vector.h"

// ヘッダで using namespace は事故の元なのでやらない
// using namespace KamataEngine; ←削除

// =========================
// 定数（今のまま global でOK）
// =========================
static inline const float kAcceleration = 0.01f;
static inline const float kAttenuation = 0.1f;
static inline const float kLimitRunSpeed = 0.3f;
static inline const float kTimeTurn = 0.3f;
static inline const float kGravityAcceleration = 0.015f;
static inline const float kLimitFallSpeed = 0.5f;
static inline const float kJumpAcceleration = 0.4f;
static inline const float kWidth = 1.99f;
static inline const float kHeight = 1.99f;
static inline const float kAttenuationLanding = 0.2f;
static inline const float kGroundingOffsetY = -0.05f;
static inline const float kAttenuationWall = 0.2f;

static inline const float modelHeight = 1.0f;

class Enemy;
class MapChipField;

enum class LRDirection {
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
	KamataEngine::Vector3 move{}; // 最終的な移動量
};

enum Corner {
	kRightBottom,
	kLeftBottom,
	kRightTop,
	kLeftTop,

	kNumCorner
};

class Player {
private:
	KamataEngine::WorldTransform worldTransform_{};
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	uint32_t textureHandle_ = 0;

	KamataEngine::Vector3 velocity_{};

	bool onGround_ = true;
	bool landing_ = false;

	float turnFirstRotationY_ = 0.0f;
	float turnTimer_ = 0.0f;

	// ヘッダ内定義OK（クラス内関数は暗黙inline）
	float EaseInOut(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }

	MapChipField* mapChipField_ = nullptr;

public:
	// ========= Getter =========
	const KamataEngine::Vector3& GetPosition() const { return worldTransform_.translation_; }
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }

	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	KamataEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

	// ========= State =========
	LRDirection lrDirection_ = LRDirection::kRight;

	bool isDead_ = false;
	float modelYOffset_ = 1.0f;

	BehaviorState behaviorState_ = BehaviorState::kRoot;

	// ========= Attack =========
	float attackTimer_ = 0.0f;
	static constexpr float kAttackDuration_ = 0.30f;
	float attackCooldownTimer_ = 0.0f;
	static constexpr float kAttackCooldown_ = 0.60f;
	float attackScaleTimer_ = 0.0f;
	static constexpr float kAttackScaleDuration_ = 0.2f;
	float attackDashSpeed_ = 0.35f; // 横ダッシュ量（今の0.4より少し弱め）
	bool attackHitDone_ = false;    // 1回だけ当てたい場合

	// ========= Jump =========
	int jumpCount_ = 0;
	int maxJumps_ = 2;

	bool spinActive_ = false;
	float spinTimer_ = 0.0f;
	float spinDuration_ = 0.35f;
	float spinStartX_ = 0.0f;

	bool firstJumpEvent_ = false;
	bool secondJumpEvent_ = false;

	// ========= Switch blocks =========
	bool blocksAreRed_ = true;

	// ★これを追加：外部bool参照をしたい時用
	const bool* blocksAreRedPtr_ = nullptr;

	// ========= Wall =========
	static constexpr float kWallSlideMaxFall = -0.18f;
	static constexpr float kWallJumpVelX = 0.35f;
	static constexpr float kWallJumpVelY = 0.42f;

	bool wallSliding_ = false;
	int wallDir_ = 0;

	bool prevHitLeft_ = false;
	bool prevHitRight_ = false;

	// ========= HP / Damage =========
	static constexpr int kMaxHP_ = 100;
	int hp_ = kMaxHP_;

	float damageCooldownTimer_ = 0.0f;
	static constexpr float kDamageInterval_ = 0.5f;
	static constexpr int kContactDamage_ = 10;

	float blinkTimer_ = 0.0f;
	static constexpr float kBlinkInterval_ = 0.08f;

public:
	// ========= Core =========
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update(float deltaTime);
	void Draw();

	void SetMapChipField(MapChipField* mapChipField) {
		assert(mapChipField != nullptr);
		mapChipField_ = mapChipField;
	}

	bool IsOnGround() const { return onGround_; }
	int GetHP() const { return hp_; }
	int GetMaxHP() const { return kMaxHP_; }
	bool IsDead() const { return isDead_; }
	void KillByFall(); // 奈落即死

	bool IsAttacking() const { return behaviorState_ == BehaviorState::kAttack; }
	AABB GetAttackAABB() const; // 攻撃判定用

	// ========= Move / Collision =========
	void InputMove();

	void CheckCollisionMap(CollisionInfo& info);
	void CheckCollisionMapTop(CollisionInfo& info);
	void CheckCollisionMapBottom(CollisionInfo& info);
	void CheckCollisionMapLeft(CollisionInfo& info);
	void CheckCollisionMapRight(CollisionInfo& info);

	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner);
	void ApplyCollisionResult(const CollisionInfo& info);
	void CheckHitCeiling(const CollisionInfo& info);
	void ChangeGroundState(const CollisionInfo& info);
	void ProcessWallCollision(const CollisionInfo& info);

	KamataEngine::Vector3 GetWorldPosition();
	AABB GetAABB() const;

	void OnCollision();

	// ========= Behavior =========
	void BehaviorRootUpdate();
	void BehaviorAttackUpdate();

	// ========= Switch =========
	// ★型エラー修正：bool* を保持する先を分ける
	void SetBlocksAreRedPtr(const bool* p) { blocksAreRedPtr_ = p; }
	void SetBlocksAreRed(bool v) { blocksAreRed_ = v; }

	// ★重要：inline を外す（cppに実装してもリンク事故が起きにくい）
	bool IsSolidForSwitch(MapChipType t, bool blocksAreRed);
	MapChipType GetTypeSafe(int x, int y);

	bool ConsumeFirstJumpEvent();
	bool ConsumeSecondJumpEvent();

	// blocksAreRed の参照元を統一（cppでもこれを使うと安全）
	bool GetBlocksAreRed() const { return (blocksAreRedPtr_ != nullptr) ? *blocksAreRedPtr_ : blocksAreRed_; }
};
