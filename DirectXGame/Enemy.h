#pragma once
#define NOMINMAX // Windows の min/max マクロ無効化（Windows.h より前）
#include <Windows.h>
#include "KamataEngine.h"
#include "Vector.h"
#include <numbers>
#include <cmath>
#include "Player.h"

static inline const float kWalkSpeed = 0.1f;
// Enemy.h 内（Enemyクラスの中に static で定義）
static inline const float kWalkMotionAngleStart = -0.25f;
static inline const float kWalkMotionAngleEnd = 0.25f; // 終了角
static inline const float kWalkMotionTime = 60.0f;       // アニメーション周期（フレーム数）
class Player;
class MapChipField;
class Enemy 
{
public:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	uint32_t textureHandle_ = 0;
	Vector3 velocity_ = {};
	float walkTimer_ = 0.0f;

	 // ★追加：死亡演出用
	bool isDead_ = false;                    // 完全に死んだ（消してOK）
	bool isDying_ = false;                   // 死亡演出中
	float deathTimer_ = 0.0f;                // 経過秒
	float deathDuration_ = 0.35f;            // 演出時間
	Vector3 deathVel_ = {0.0f, 0.25f, 0.0f}; // ふわっと上がる
	float deathSpinSpeed_ = 18.0f;           // 回転スピード（ラジアン/秒くらいの気持ち）

	// 落ちる加速（Y方向） ※左手座標系：落下はマイナス
	float deathGravity_ = -18.0f; // 1秒あたりの加速度（好みで調整）
	float deathRotSpeed_ = 10.0f; // 回転速度（rad/sec）
	float deathEndY_ = -50.0f;    // これより下に落ちたら消す

	 // ---- 歩行用 ----
	float moveSpeed_ = 2.0f; // 移動速度
	int moveDir_ = -1;       // -1: 左, +1: 右
	                         // ---- 物理 ----
	float gravity_ = -18.0f; // 重力(マイナス)
	float maxFallSpeed_ = -25.0f; // 落下速度の下限
	bool onGround_ = false;

	void StartDeath(const Vector3& hitterPos); // ★追加：死亡開始
	bool IsDead() const { return isDead_; } // ★追加
	bool IsDying() const { return isDying_; }
	Vector3 halfSize_ = {0.45f, 0.45f, 0.45f};

	  void SetMapChipField(MapChipField* field) { mapChipField_ = field; }
	void SetBlocksAreRed(bool v) { blocksAreRed_ = v; }
	  MapChipField* mapChipField_ = nullptr;
	  bool blocksAreRed_ = true;
	
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const Vector3& position);
	void Update();
	void Draw();
	void SetTexture(uint32_t textureHandle);
	AABB GetAABB() const;
	Vector3 GetWorldPosition() const;
	void OnCollision(const Player* player);
	void OnHit(int damage, const Vector3& hitterPos);

	bool IsSolidAtIndex(int ix, int iy) const;
	void ResolveHorizontal(float dt);
	void ResolveVertical(float dt);
	void CheckCliffTurn();

	public:
	void Nudge(const KamataEngine::Vector3& delta);
	bool CanHitEnemy() const { return enemyHitCooldown_ <= 0.0f; }
	void StartEnemyHitCooldown(float t) { enemyHitCooldown_ = t; }

	private:
	float enemyHitCooldown_ = 0.0f;
};
