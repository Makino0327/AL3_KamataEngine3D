#include "Player.h"
#include "Enemy.h"

#include <algorithm> // std::max, std::min, std::clamp
#include <array>     // std::array
#include <cmath>     // std::fmod, std::fabs
#include <numbers>   // std::numbers::pi_v

using namespace KamataEngine;

//==================================================
// Initialize / Update / Draw
//==================================================

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();

	// ※ modelHeight / modelYOffset_ が Player.h にある前提
	modelYOffset_ = (modelHeight * worldTransform_.scale_.y) / 100.0f;

	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};

	// とりあえず仮（あなたの元コードを尊重）
	textureHandle_ = TextureManager::Load("./Resources/GameOver/GameOver.png");

	onGround_ = false;

	hp_ = kMaxHP_;
	damageCooldownTimer_ = 0.0f;
	blinkTimer_ = 0.0f;
	isDead_ = false;

	// 攻撃系（未初期化で暴れるの防止）
	behaviorState_ = BehaviorState::kRoot;
	attackTimer_ = 0.0f;
	attackCooldownTimer_ = 0.0f;

	// 壁系（未初期化で暴れるの防止）
	prevHitLeft_ = false;
	prevHitRight_ = false;
	wallSliding_ = false;
	wallDir_ = 0;

	// ジャンプ系
	jumpCount_ = 0;
	firstJumpEvent_ = false;
	secondJumpEvent_ = false;

	// 行列更新（描画用は correctedTranslation を使う）
	Vector3 corrected = worldTransform_.translation_;
	corrected.y -= modelYOffset_;
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, corrected);
	worldTransform_.TransferMatrix();
}

void Player::Update(float deltaTime) {
	// クールタイム更新
	if (attackCooldownTimer_ > 0.0f) {
		attackCooldownTimer_ = std::max(0.0f, attackCooldownTimer_ - deltaTime);
	}
	if (damageCooldownTimer_ > 0.0f) {
		damageCooldownTimer_ = std::max(0.0f, damageCooldownTimer_ - deltaTime);
	}

	// 無敵点滅タイマー
	if (damageCooldownTimer_ > 0.0f) {
		blinkTimer_ += deltaTime;
	} else {
		blinkTimer_ = 0.0f;
	}

	// 攻撃開始（Root中 & E押下 & クール明け）
	if (behaviorState_ == BehaviorState::kRoot && Input::GetInstance()->TriggerKey(DIK_E) && attackCooldownTimer_ <= 0.0f) {

		behaviorState_ = BehaviorState::kAttack;
		attackTimer_ = 0.0f;
		attackHitDone_ = false;

		// ★ぴょーん防止：攻撃開始時にY速度を止める
		if (onGround_) {
			velocity_.y = 0.0f;
		} else {
			// 空中攻撃を許可するなら「上向きだけ消す」
			velocity_.y = std::min(velocity_.y, 0.0f);
		}
	}


	// 状態更新
	switch (behaviorState_) {
	case BehaviorState::kRoot:
		BehaviorRootUpdate();
		break;

	case BehaviorState::kAttack:
		BehaviorAttackUpdate();
		attackTimer_ += deltaTime;
		if (attackTimer_ >= kAttackDuration_) {
			behaviorState_ = BehaviorState::kRoot;
			attackTimer_ = 0.0f;
			attackCooldownTimer_ = kAttackCooldown_;
		}
		break;
	}

	// 行列更新（ここだけでOK：InputMove内では更新しない）
	Vector3 corrected = worldTransform_.translation_;
	corrected.y -= modelYOffset_;
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, corrected);
	worldTransform_.TransferMatrix();
}

void Player::Draw() {
	// 無敵中なら点滅
	if (damageCooldownTimer_ > 0.0f) {
		const float period = kBlinkInterval_ * 2.0f; // ON/OFFで1周期
		const float t = std::fmod(blinkTimer_, period);
		if (t >= kBlinkInterval_) {
			return; // 消える側
		}
	}

	model_->Draw(worldTransform_, *camera_, textureHandle_);
}

//==================================================
// Movement
//==================================================

void Player::InputMove() {
	const float accel = kAcceleration;
	const float maxSpeed = kLimitRunSpeed;
	const float friction = kAttenuation;

	const bool left = Input::GetInstance()->PushKey(DIK_A);
	const bool right = Input::GetInstance()->PushKey(DIK_D);

	if (left) {
		velocity_.x -= accel;
		lrDirection_ = LRDirection::kLeft;
		worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;
	}
	if (right) {
		velocity_.x += accel;
		lrDirection_ = LRDirection::kRight;
		worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	}

	// 入力なし → 摩擦
	if (!left && !right) {
		velocity_.x *= (1.0f - friction);
		if (std::fabs(velocity_.x) < 0.001f) {
			velocity_.x = 0.0f;
		}
	}

	velocity_.x = std::clamp(velocity_.x, -maxSpeed, maxSpeed);

	// ★ 行列更新は Update() の最後だけでやる（ここではやらない）
}

//==================================================
// Map Collision
//==================================================

void Player::CheckCollisionMap(CollisionInfo& info) {
	CheckCollisionMapTop(info);
	CheckCollisionMapBottom(info);
	CheckCollisionMapLeft(info);
	CheckCollisionMapRight(info);
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0}, // 右下
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0}, // 左下
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0}, // 右上
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0}  // 左上
	};
	return Add(center, offsetTable[static_cast<uint32_t>(corner)]);
}

void Player::ApplyCollisionResult(const CollisionInfo& info) { worldTransform_.translation_ = Add(worldTransform_.translation_, info.move); }

void Player::CheckHitCeiling(const CollisionInfo& info) {
	if (info.isHitTop) {
		velocity_.y = 0.0f;
	}
}

void Player::ProcessWallCollision(const CollisionInfo& info) {
	if (info.isHitLeft || info.isHitRight) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

void Player::ChangeGroundState(const CollisionInfo& info) {
	if (onGround_) {
		// ジャンプで上向きに動き始めたら空中
		if (velocity_.y > 0.0f) {
			onGround_ = false;
			return;
		}

		// 足元に地面があるか再判定（スイッチ対応）
		std::array<Vector3, kNumCorner> positions{};
		for (uint32_t i = 0; i < positions.size(); ++i) {
			Vector3 pos = CornerPosition(worldTransform_.translation_, static_cast<Corner>(i));
			if (i == kLeftBottom || i == kRightBottom) {
				pos.y += kGroundingOffsetY; // 少し下
			}
			positions[i] = pos;
		}

		bool hitLeft = false;
		bool hitRight = false;

		// 左足
		{
			IndexSet idx = mapChipField_->GetMapChipIndexByPosition(positions[kLeftBottom]);
			MapChipType t = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
			hitLeft = IsSolidForSwitch(t, blocksAreRed_);
		}
		// 右足
		{
			IndexSet idx = mapChipField_->GetMapChipIndexByPosition(positions[kRightBottom]);
			MapChipType t = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
			hitRight = IsSolidForSwitch(t, blocksAreRed_);
		}

		if (!hitLeft && !hitRight) {
			onGround_ = false;
		}

	} else {
		// 空中→接地
		if (info.isHitBottom) {
			onGround_ = true;
			velocity_.x *= (1.0f - kAttenuationLanding);
			velocity_.y = 0.0f;
			jumpCount_ = 0;

			spinActive_ = false;
			worldTransform_.rotation_.x = 0.0f;
		}
	}
}

void Player::CheckCollisionMapTop(CollisionInfo& info) {
	if (info.move.y <= 0.0f)
		return;

	std::array<Vector3, kNumCorner> positionsNew{};
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));
	}

	bool hit = false;
	IndexSet hitIndex{0, 0};

	// 左上
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftTop]);
		MapChipType tNow = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		MapChipType tNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex + 1);

		if (IsSolidForSwitch(tNow, blocksAreRed_) && !IsSolidForSwitch(tNext, blocksAreRed_)) {
			Vector3 preTop = CornerPosition(worldTransform_.translation_, kLeftTop);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preTop);
			if (preIndex.yIndex != index.yIndex) {
				hit = true;
				hitIndex = index;
			}
		}
	}
	// 右上
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightTop]);
		MapChipType tNow = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		MapChipType tNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex + 1);

		if (IsSolidForSwitch(tNow, blocksAreRed_) && !IsSolidForSwitch(tNext, blocksAreRed_)) {
			Vector3 preTop = CornerPosition(worldTransform_.translation_, kRightTop);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preTop);
			if (preIndex.yIndex != index.yIndex) {
				hit = true;
				hitIndex = index;
			}
		}
	}

	if (hit) {
		Rect rect = mapChipField_->GetRectByIndex(hitIndex.xIndex, hitIndex.yIndex);
		float playerTopY = worldTransform_.translation_.y + kHeight / 2.0f;
		float diff = rect.bottom - playerTopY;
		info.move.y = std::max(0.0f, diff);
		info.isHitTop = true;
	}
}

void Player::CheckCollisionMapBottom(CollisionInfo& info) {
	if (info.move.y >= 0.0f)
		return;

	std::array<Vector3, kNumCorner> positionsNew{};
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));
	}

	bool hit = false;
	IndexSet hitIndex{0, 0};

	// 左下
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
		MapChipType tNow = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		MapChipType tNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex - 1);

		if (IsSolidForSwitch(tNow, blocksAreRed_) && !IsSolidForSwitch(tNext, blocksAreRed_)) {
			Vector3 preBottom = CornerPosition(worldTransform_.translation_, kLeftBottom);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preBottom);
			if (preIndex.yIndex != index.yIndex) {
				hit = true;
				hitIndex = index;
			}
		}
	}

	// 右下
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightBottom]);
		MapChipType tNow = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		MapChipType tNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex - 1);

		if (IsSolidForSwitch(tNow, blocksAreRed_) && !IsSolidForSwitch(tNext, blocksAreRed_)) {
			Vector3 preBottom = CornerPosition(worldTransform_.translation_, kRightBottom);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preBottom);
			if (preIndex.yIndex != index.yIndex) {
				hit = true;
				hitIndex = index;
			}
		}
	}

	if (hit) {
		Rect rect = mapChipField_->GetRectByIndex(hitIndex.xIndex, hitIndex.yIndex);
		float playerBottomY = worldTransform_.translation_.y - kHeight / 2.0f;
		float diff = rect.top - playerBottomY;
		info.move.y = std::min(0.0f, diff);
		info.isHitBottom = true;
	}
}

void Player::CheckCollisionMapRight(CollisionInfo& info) {
	if (info.move.x <= 0.0f)
		return;

	std::array<Vector3, kNumCorner> positionsNew{};
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));
	}

	bool hit = false;
	IndexSet hitIndex{0, 0};

	// 右上
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightTop]);
		MapChipType tNow = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		MapChipType tPrev = mapChipField_->GetMapChipTypeByIndex(index.xIndex - 1, index.yIndex);

		if (IsSolidForSwitch(tNow, blocksAreRed_) && !IsSolidForSwitch(tPrev, blocksAreRed_)) {
			Vector3 preRight = CornerPosition(worldTransform_.translation_, kRightTop);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preRight);
			if (preIndex.xIndex != index.xIndex) {
				hit = true;
				hitIndex = index;
			}
		}
	}

	// 右下
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightBottom]);
		MapChipType tNow = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		MapChipType tPrev = mapChipField_->GetMapChipTypeByIndex(index.xIndex - 1, index.yIndex);

		if (IsSolidForSwitch(tNow, blocksAreRed_) && !IsSolidForSwitch(tPrev, blocksAreRed_)) {
			Vector3 preRight = CornerPosition(worldTransform_.translation_, kRightBottom);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preRight);
			if (preIndex.xIndex != index.xIndex) {
				hit = true;
				hitIndex = index;
			}
		}
	}

	if (hit) {
		Rect rect = mapChipField_->GetRectByIndex(hitIndex.xIndex, hitIndex.yIndex);
		float playerRightX = worldTransform_.translation_.x + kWidth / 2.0f;
		float diff = rect.left - playerRightX;
		info.move.x = std::min(0.0f, diff);
		info.isHitRight = true;
	}
}

void Player::CheckCollisionMapLeft(CollisionInfo& info) {
	if (info.move.x >= 0.0f)
		return;

	std::array<Vector3, kNumCorner> positionsNew{};
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));
	}

	bool hit = false;
	IndexSet hitIndex{0, 0};

	// 左上
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftTop]);
		MapChipType tNow = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		MapChipType tPrev = mapChipField_->GetMapChipTypeByIndex(index.xIndex + 1, index.yIndex); // 右隣

		if (IsSolidForSwitch(tNow, blocksAreRed_) && !IsSolidForSwitch(tPrev, blocksAreRed_)) {
			Vector3 preLeft = CornerPosition(worldTransform_.translation_, kLeftTop);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preLeft);
			if (preIndex.xIndex != index.xIndex) {
				hit = true;
				hitIndex = index;
			}
		}
	}

	// 左下
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
		MapChipType tNow = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		MapChipType tPrev = mapChipField_->GetMapChipTypeByIndex(index.xIndex + 1, index.yIndex); // 右隣

		if (IsSolidForSwitch(tNow, blocksAreRed_) && !IsSolidForSwitch(tPrev, blocksAreRed_)) {
			Vector3 preLeft = CornerPosition(worldTransform_.translation_, kLeftBottom);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preLeft);
			if (preIndex.xIndex != index.xIndex) {
				hit = true;
				hitIndex = index;
			}
		}
	}

	if (hit) {
		Rect rect = mapChipField_->GetRectByIndex(hitIndex.xIndex, hitIndex.yIndex);
		float playerLeftX = worldTransform_.translation_.x - kWidth / 2.0f;
		float diff = rect.right - playerLeftX;
		info.move.x = std::max(info.move.x, diff);
		info.isHitLeft = true;
	}
}

//==================================================
// Combat / Damage
//==================================================

void Player::OnCollision() {
	if (isDead_)
		return;

	// 無敵中はダメージ無し
	if (damageCooldownTimer_ > 0.0f)
		return;

	if (IsAttacking()) {
		return;
	}

	hp_ -= kContactDamage_;
	if (hp_ < 0)
		hp_ = 0;

	damageCooldownTimer_ = kDamageInterval_;

	if (hp_ <= 0) {
		isDead_ = true;
	}
}

//==================================================
// Behavior
//==================================================

void Player::BehaviorRootUpdate() {
	// スケール戻す
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	attackScaleTimer_ = 0.0f;

	// 入力による移動
	InputMove();

	const bool left = Input::GetInstance()->PushKey(DIK_A);
	const bool right = Input::GetInstance()->PushKey(DIK_D);
	const bool jumpPressed = Input::GetInstance()->TriggerKey(DIK_SPACE);

	// 壁ジャンプ先行判定（前フレーム接触情報）
	bool wantWallJump = false;
	int wallDirForJump = 0; // -1=左 / +1=右

	if (!onGround_ && jumpPressed) {
		if (prevHitLeft_ && left) {
			wantWallJump = true;
			wallDirForJump = -1;
		}
		if (prevHitRight_ && right) {
			wantWallJump = true;
			wallDirForJump = +1;
		}
	}

	if (wantWallJump) {
		velocity_.x = (wallDirForJump == -1) ? +kWallJumpVelX : -kWallJumpVelX;
		velocity_.y = kWallJumpVelY;

		onGround_ = false;
		wallSliding_ = false;

		jumpCount_ = maxJumps_; // 二段不可化
		spinActive_ = false;
		secondJumpEvent_ = false;

	} else {
		// 通常ジャンプ（壁すべり中は後で二段封じる）
		if (jumpPressed && jumpCount_ < maxJumps_) {
			const bool isSecondJump = (jumpCount_ == 1);

			if (velocity_.y < 0.0f)
				velocity_.y = 0.0f;
			velocity_.y = kJumpAcceleration;
			onGround_ = false;

			if (!isSecondJump) {
				firstJumpEvent_ = true;
			} else {
				secondJumpEvent_ = true;
				spinActive_ = true;
				spinTimer_ = 0.0f;
				spinStartX_ = worldTransform_.rotation_.x;
			}
			++jumpCount_;
		}
	}

	// 重力
	if (!onGround_) {
		velocity_.y += -kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}

	CollisionInfo collisionInfo{};
	collisionInfo.move = velocity_;

	// マップ衝突
	CheckCollisionMap(collisionInfo);

	// 今フレームの壁接触
	bool hitL = collisionInfo.isHitLeft;
	bool hitR = collisionInfo.isHitRight;

	// 壁すべり判定
	wallSliding_ = false;
	wallDir_ = 0;
	if (!onGround_ && velocity_.y < 0.0f) {
		if (hitL && left) {
			wallSliding_ = true;
			wallDir_ = -1;
		}
		if (hitR && right) {
			wallSliding_ = true;
			wallDir_ = +1;
		}
	}

	if (wallSliding_) {
		if (velocity_.y < kWallSlideMaxFall) {
			velocity_.y = kWallSlideMaxFall;
		}
		jumpCount_ = std::max(jumpCount_, 1); // 二段封じる
		spinActive_ = false;
		secondJumpEvent_ = false;

		collisionInfo.move.y = velocity_.y;
	}

	// 接地状態更新
	ChangeGroundState(collisionInfo);

	// 天井ヒット
	CheckCollisionMapTop(collisionInfo);

	// 移動反映
	ApplyCollisionResult(collisionInfo);

	// 壁減衰
	ProcessWallCollision(collisionInfo);

	// 天井ならY止める
	CheckHitCeiling(collisionInfo);

	// 次フレーム用に保存
	prevHitLeft_ = hitL;
	prevHitRight_ = hitR;

	// DebugText::GetInstance()->ConsolePrintf("Player Pos: x=%.2f y=%.2f z=%.2f\n",
	//     worldTransform_.translation_.x, worldTransform_.translation_.y, worldTransform_.translation_.z);
}

void Player::BehaviorAttackUpdate() {

	// =========================
	// 見た目：攻撃スケール演出（プレイヤー本体だけ）
	// =========================
	attackScaleTimer_ += 1.0f / 60.0f; // deltaTime渡してないなら仮で60fps
	float t = std::clamp(attackScaleTimer_ / kAttackScaleDuration_, 0.0f, 1.0f);

	// 0→1 で「潰れて戻る」カーブ
	// 前半: 潰れる / 後半: 戻る
	float s;
	if (t < 0.5f) {
		float u = t / 0.5f;   // 0..1
		s = 1.0f - 0.25f * u; // 1.0 → 0.75
	} else {
		float u = (t - 0.5f) / 0.5f; // 0..1
		s = 0.75f + 0.25f * u;       // 0.75 → 1.0
	}

	// 横に少し伸ばして「斬る/突く」感じ
	float sx = 1.0f + (1.0f - s) * 0.9f; // 1.0 → 1.225 くらい
	float sy = s;                        // 1.0 → 0.75
	worldTransform_.scale_ = {sx, sy, 1.0f};

	// 少し前のめり（好み）
	const float tilt = 0.18f;
	worldTransform_.rotation_.x = (lrDirection_ == LRDirection::kRight) ? +tilt : +tilt;

	// =========================
	// ここから下は「移動＆衝突」（重力ゼロ版）
	// =========================
	const float dash = (lrDirection_ == LRDirection::kRight) ? +attackDashSpeed_ : -attackDashSpeed_;

	velocity_.y = 0.0f;

	CollisionInfo collisionInfo{};
	collisionInfo.move = {dash, 0.0f, 0.0f};

	CheckCollisionMap(collisionInfo);
	ChangeGroundState(collisionInfo);
	ApplyCollisionResult(collisionInfo);
	ProcessWallCollision(collisionInfo);
	CheckHitCeiling(collisionInfo);
}



//==================================================
// Utility
//==================================================

Vector3 Player::GetWorldPosition() {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Player::GetAABB() const {
	const Vector3 c = worldTransform_.translation_;

	const float loose = 0.85f;
	const float halfW = (kWidth * worldTransform_.scale_.x) * 0.5f * loose;
	const float halfH = (kHeight * worldTransform_.scale_.y) * 0.5f * loose;
	const float halfZ = 0.5f;

	AABB aabb;
	aabb.min = {c.x - halfW, c.y - halfH, c.z - halfZ};
	aabb.max = {c.x + halfW, c.y + halfH, c.z + halfZ};
	return aabb;
}

bool Player::ConsumeFirstJumpEvent() {
	if (firstJumpEvent_) {
		firstJumpEvent_ = false;
		return true;
	}
	return false;
}

bool Player::ConsumeSecondJumpEvent() {
	if (secondJumpEvent_) {
		secondJumpEvent_ = false;
		return true;
	}
	return false;
}

//==================================================
// Switch Collision Helpers  (★cpp側に置く：inlineは付けない)
//==================================================

bool Player::IsSolidForSwitch(MapChipType t, bool blocksAreRed) {
	switch (t) {
	case MapChipType::kBlock:
		return true;
	case MapChipType::kBlockRed:
		return blocksAreRed;
	case MapChipType::kBlockBlue:
		return !blocksAreRed;
	default:
		return false;
	}
}

MapChipType Player::GetTypeSafe(int x, int y) {
	if (x < 0 || y < 0 || x >= (int)MapChipField::kNumBlockHorizontal || y >= (int)MapChipField::kNumBlockVirtical) {
		return MapChipType::kBlank;
	}
	return mapChipField_->GetMapChipTypeByIndex((uint32_t)x, (uint32_t)y);
}

void Player::KillByFall() {
	if (isDead_)
		return;

	hp_ = 0;
	isDead_ = true;

	// 無敵とか点滅が残るのが嫌ならここもリセットしてOK
	damageCooldownTimer_ = 0.0f;
	blinkTimer_ = 0.0f;
}

AABB Player::GetAttackAABB() const {
	// プレイヤー本体AABBをベースに、前に伸ばす
	AABB body = GetAABB();

	const float reach = 1.0f; // 攻撃リーチ
	if (lrDirection_ == LRDirection::kRight) {
		body.max.x += reach;
	} else {
		body.min.x -= reach;
	}

	// 縦はちょい狭めてもいい（好み）
	return body;
}
