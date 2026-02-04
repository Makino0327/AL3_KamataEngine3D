#include "Enemy.h"
#include <algorithm>
#include <numbers>
#include "MapChipField.h"

using namespace KamataEngine;

bool Enemy::IsSolidAtIndex(int ix, int iy) const {
	if (!mapChipField_)
		return false;

	// 範囲外は壁扱いにすると安全（マップ外に出ない）
	if (ix < 0 || iy < 0 || ix >= (int)MapChipField::kNumBlockHorizontal || iy >= (int)MapChipField::kNumBlockVirtical) {
		return true;
	}

	MapChipType type = mapChipField_->GetMapChipTypeByIndex((uint32_t)ix, (uint32_t)iy);

	switch (type) {
	case MapChipType::kBlock:
		return true;
	case MapChipType::kBlockRed:
		return blocksAreRed_; // 赤ONのときだけ壁
	case MapChipType::kBlockBlue:
		return !blocksAreRed_; // 赤OFF(=青ON)のときだけ壁
	case MapChipType::kChargeBreakable:
		return true;        

	default:
		return false;
	}
}

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
	moveDir_ = -1; // 最初は左に歩く
	velocity_.x = moveSpeed_ * moveDir_;

}

void Enemy::Update() {

	const float dt = 1.0f / 60.0f;
	 // ★通常時も毎フレーム減らす（これが無いと2回目以降当たらない）
	enemyHitCooldown_ = std::max(0.0f, enemyHitCooldown_ - dt);

	// ===== 死亡演出中（あなたのまま）=====
	if (isDying_) {
		

		deathVel_.y += deathGravity_ * dt;
		worldTransform_.translation_.x += deathVel_.x * dt;
		worldTransform_.translation_.y += deathVel_.y * dt;
		worldTransform_.translation_.z += deathVel_.z * dt;
		worldTransform_.rotation_.z += deathRotSpeed_ * dt;

		if (worldTransform_.translation_.y < deathEndY_) {
			isDead_ = true;
			isDying_ = false;
		}

		worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
		worldTransform_.TransferMatrix();
		return;
	}



	// ===== 歩行（常に左右へ）=====
	velocity_.x = moveSpeed_ * (float)moveDir_;

	// ===== 重力 =====
	velocity_.y += gravity_ * dt;
	velocity_.y = std::max(velocity_.y, maxFallSpeed_);

	// ===== ブロック衝突（X→Yの順で解決）=====
	ResolveHorizontal(dt);
	ResolveVertical(dt);

	CheckCliffTurn();

	// 向き（見た目）
	worldTransform_.rotation_.y = (moveDir_ > 0) ? +std::numbers::pi_v<float> / 2.0f : -std::numbers::pi_v<float> / 2.0f;

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
    // ★ matWorld じゃなく translation を返す
    return worldTransform_.translation_;
}

AABB Enemy::GetAABB() const {
    // ★ translation 基準（Player方式）
    const Vector3 c = worldTransform_.translation_;

    const float loose = 1.0f;
	const float halfW = (kWidth * worldTransform_.scale_.x) * 0.5f * loose;
	const float halfH = (kHeight * worldTransform_.scale_.y) * 0.5f * loose;
    const float halfZ = 0.5f;

    AABB aabb;
    aabb.min = { c.x - halfW, c.y - halfH, c.z - halfZ };
    aabb.max = { c.x + halfW, c.y + halfH, c.z + halfZ };
    return aabb;
}



void Enemy::OnCollision(const Player* player) {
	(void)player;

	// ★壁 or プレイヤーに当たったら反転
	moveDir_ *= -1;
	velocity_.x = moveSpeed_ * moveDir_;
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

void Enemy::ResolveHorizontal(float dt) {
	if (!mapChipField_) {
		worldTransform_.translation_.x += velocity_.x * dt;
		return;
	}

	worldTransform_.translation_.x += velocity_.x * dt;

	AABB aabb = GetAABB();

	if (velocity_.x > 0.0f) {
		float probeX = aabb.max.x;
		float y1 = aabb.min.y + 0.05f;
		float y2 = aabb.max.y - 0.05f;

		IndexSet i1 = mapChipField_->GetMapChipIndexByPosition({probeX, y1, 0.0f});
		IndexSet i2 = mapChipField_->GetMapChipIndexByPosition({probeX, y2, 0.0f});

		if (IsSolidAtIndex((int)i1.xIndex, (int)i1.yIndex) || IsSolidAtIndex((int)i2.xIndex, (int)i2.yIndex)) {
			Rect rect = mapChipField_->GetRectByIndex(i1.xIndex, i1.yIndex);

			// 右面を rect.left に合わせる
			worldTransform_.translation_.x -= (aabb.max.x - rect.left);

			moveDir_ *= -1;
			velocity_.x = 0.0f;
		}
	}

	if (velocity_.x < 0.0f) {
		float probeX = aabb.min.x;
		float y1 = aabb.min.y + 0.05f;
		float y2 = aabb.max.y - 0.05f;

		IndexSet i1 = mapChipField_->GetMapChipIndexByPosition({probeX, y1, 0.0f});
		IndexSet i2 = mapChipField_->GetMapChipIndexByPosition({probeX, y2, 0.0f});

		if (IsSolidAtIndex((int)i1.xIndex, (int)i1.yIndex) || IsSolidAtIndex((int)i2.xIndex, (int)i2.yIndex)) {
			Rect rect = mapChipField_->GetRectByIndex(i1.xIndex, i1.yIndex);

			// 左面を rect.right に合わせる
			worldTransform_.translation_.x += (rect.right - aabb.min.x);

			moveDir_ *= -1;
			velocity_.x = 0.0f;
		}
	}
}


void Enemy::ResolveVertical(float dt) {
	if (!mapChipField_) {
		worldTransform_.translation_.y += velocity_.y * dt;
		return;
	}

	onGround_ = false;

	// まずYだけ動かす
	worldTransform_.translation_.y += velocity_.y * dt;

	AABB aabb = GetAABB();

	// 落下中：足元2点
	if (velocity_.y < 0.0f) {
		float probeY = aabb.min.y;
		float x1 = aabb.min.x + 0.05f;
		float x2 = aabb.max.x - 0.05f;

		// ★ index は MapChipField の関数で取る（ズレない）
		IndexSet i1 = mapChipField_->GetMapChipIndexByPosition({x1, probeY, 0.0f});
		IndexSet i2 = mapChipField_->GetMapChipIndexByPosition({x2, probeY, 0.0f});

		if (IsSolidAtIndex((int)i1.xIndex, (int)i1.yIndex) || IsSolidAtIndex((int)i2.xIndex, (int)i2.yIndex)) {
			// ★ rect から床の上面を取る（決め打ちしない）
			Rect rect = mapChipField_->GetRectByIndex(i1.xIndex, i1.yIndex);

			// aabb.min.y を rect.top に合わせる
			worldTransform_.translation_.y += (rect.top - aabb.min.y);

			velocity_.y = 0.0f;
			onGround_ = true;
		}
	}

	// 上昇中：頭2点
	if (velocity_.y > 0.0f) {
		float probeY = aabb.max.y;
		float x1 = aabb.min.x + 0.05f;
		float x2 = aabb.max.x - 0.05f;

		IndexSet i1 = mapChipField_->GetMapChipIndexByPosition({x1, probeY, 0.0f});
		IndexSet i2 = mapChipField_->GetMapChipIndexByPosition({x2, probeY, 0.0f});

		if (IsSolidAtIndex((int)i1.xIndex, (int)i1.yIndex) || IsSolidAtIndex((int)i2.xIndex, (int)i2.yIndex)) {
			Rect rect = mapChipField_->GetRectByIndex(i1.xIndex, i1.yIndex);

			// aabb.max.y を rect.bottom に合わせる（天井）
			worldTransform_.translation_.y -= (aabb.max.y - rect.bottom);

			velocity_.y = 0.0f;
		}
	}
}

void Enemy::CheckCliffTurn() {
	if (!mapChipField_) {
		return;
	}
	if (!onGround_) {
		return;
	} // ★地面に立ってる時だけ
	if (velocity_.y != 0.0f) {
		return;
	} // ★念のため（落下開始中はやらない）

	AABB aabb = GetAABB();

	// 前足のX（進行方向のちょい先）
	const float frontX = (moveDir_ > 0) ? (aabb.max.x + 0.05f) : (aabb.min.x - 0.05f);

	// 足元より「ほんの少し下」を見る → 下のマスに入る
	const float underY = aabb.min.y - 0.01f;

	IndexSet idx = mapChipField_->GetMapChipIndexByPosition({frontX, underY, 0.0f});

	// ★範囲外＝床なし扱いにして反転（落下防止）
	if ((int)idx.xIndex < 0 || (int)idx.yIndex < 0 || (int)idx.xIndex >= (int)MapChipField::kNumBlockHorizontal || (int)idx.yIndex >= (int)MapChipField::kNumBlockVirtical) {
		moveDir_ *= -1;
		return;
	}

	// そのマスが空（0）＝床なしなら反転
	if (!IsSolidAtIndex((int)idx.xIndex, (int)idx.yIndex)) {
		moveDir_ *= -1;
	}
}

void Enemy::Nudge(const Vector3& delta) {
	worldTransform_.translation_.x += delta.x;
	worldTransform_.translation_.y += delta.y;
	worldTransform_.translation_.z += delta.z;

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}
