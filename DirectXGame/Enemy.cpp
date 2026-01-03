#include "Enemy.h"
#include <algorithm>
#include <numbers>

using namespace KamataEngine;

void Enemy::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;

	// ★これが無いと出ない
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	velocity_ = {0.0f, 0.0f, 0.0f};
	walkTimer_ = 0.0f;
}

void Enemy::Update() {

	if (isDying_) {
		const float dt = 1.0f / 60.0f;

		// ★重力で放物線
		deathVel_.y += deathGravity_ * dt;

		// ★位置更新
		worldTransform_.translation_.x += deathVel_.x * dt;
		worldTransform_.translation_.y += deathVel_.y * dt;
		worldTransform_.translation_.z += deathVel_.z * dt;
		// こてっと：回転は控えめ（無くしてもOK）
		worldTransform_.rotation_.z += deathRotSpeed_ * dt;

		// 画面外で消す
		if (worldTransform_.translation_.y < deathEndY_) {
			isDead_ = true;
			isDying_ = false;
		}

		worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
		worldTransform_.TransferMatrix();
		return;
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}



void Enemy::Draw() {
	if (isDead_) {
		return;
	}
	model_->Draw(worldTransform_, *camera_, textureHandle_);
}

// Enemy.cpp
void Enemy::SetTexture(uint32_t textureHandle) { textureHandle_ = textureHandle; }

Vector3 Enemy::GetWorldPosition() const {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}


AABB Enemy::GetAABB() const {
	Vector3 worldPos = GetWorldPosition(); // ワールド座標を取得

	AABB aabb;
	aabb.min = {worldPos.x - 1.0f, worldPos.y - 1.0f, worldPos.z - 1.0f};
	aabb.max = {worldPos.x + 1.0f, worldPos.y + 1.0f, worldPos.z + 1.0f};

	return aabb;
}


void Enemy::OnCollision(const Player* player) {
	(void)player; // 引数未使用警告を回避するためのキャスト

	// 今は何も処理なし（後でHP減らす処理などをここに追加）
}

void Enemy::OnHit(int damage, const Vector3& hitterPos) {
	(void)damage;
	StartDeath(hitterPos);
}


void Enemy::StartDeath(const Vector3& hitterPos) {
	if (isDead_ || isDying_) {
		return;
	}

	isDying_ = true;

	// ひっくり返る（こてっ感）
	worldTransform_.rotation_.z = std::numbers::pi_v<float>;

	// ---- 弧の初速 ----
	// 上に少し跳ねる
	const float up = 8.5f; // ここ上げると弧が大きくなる
	// 横に少し飛ぶ
	const float side = 5.0f; // ここ上げると横に飛ぶ

	// hitterPos(=プレイヤー位置) が左なら右へ、右なら左へ
	float dir = (hitterPos.x < worldTransform_.translation_.x) ? +1.0f : -1.0f;

	deathVel_.x = side * dir;
	deathVel_.y = up;
	deathVel_.z = 0.0f;

	// ===== 追加：Z方向だけ =====
	const float zPush = -2.25f; // ← カメラ側に寄せる量（調整用）
	deathVel_.z = zPush;


	// 通常の移動は止める
	velocity_ = {0, 0, 0};
}
