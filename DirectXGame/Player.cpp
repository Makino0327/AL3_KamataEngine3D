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
	textureHandle_ = TextureManager::Load("./Resources/player.png");

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
	guideVisible_ = true;
	hasGuideTarget_ = false;


}

void Player::Update(float deltaTime) {
	// クールタイム更新
	if (attackCooldownTimer_ > 0.0f) {
		attackCooldownTimer_ = std::max(0.0f, attackCooldownTimer_ - deltaTime);
	}
	if (damageCooldownTimer_ > 0.0f) {
		damageCooldownTimer_ = std::max(0.0f, damageCooldownTimer_ - deltaTime);
	}
	// 弾クールタイム
	if (bulletCooldown_ > 0.0f) {
		bulletCooldown_ = std::max(0.0f, bulletCooldown_ - deltaTime);
	}

	// --------------------
	// E：短押し=通常 / 長押し=チャージ（離した瞬間に発射）
	// --------------------
	const bool eDown = KamataEngine::Input::GetInstance()->IsTriggerMouse(0);
	const bool eTrigger = KamataEngine::Input::GetInstance()->IsPressMouse(0);

	// Release（離した瞬間）を自前で作る
	static bool prevEDown = false;
	const bool eRelease = (!eDown && prevEDown);
	prevEDown = eDown;

	// 押し始め：チャージ開始
	if (eTrigger && bulletCooldown_ <= 0.0f) {
		isCharging_ = true;
		chargeTime_ = 0.0f;
		chargePulse_ = 0.0f;
	}

	// 押してる間：溜める
	if (isCharging_ && eDown) {
		chargeTime_ += deltaTime;
		if (chargeTime_ > kChargeMax_)
			chargeTime_ = kChargeMax_;
		chargePulse_ = std::clamp(chargeTime_ / kChargeMax_, 0.0f, 1.0f);

		// ★しきい値を超えた瞬間に「ドン！」（1回だけ）
		if (!chargeReady_ && chargeTime_ >= kChargeThreshold_) {
			chargeReady_ = true;
			chargePopTimer_ = kChargePopTime_;
		}

		// ★点滅（溜めるほど強い）
		chargeReadyFlash_ += deltaTime * (6.0f + 10.0f * chargePulse_);
		if (chargeReadyFlash_ > 1000.0f)
			chargeReadyFlash_ = 0.0f;
	}

	// 離した瞬間：発射
	if (isCharging_ && eRelease) {
		const bool isChargeShot = (chargeTime_ >= kChargeThreshold_);
		float power01 = isChargeShot ? 1.0f : 0.0f;

		SpawnBulletWithPower(power01);
		bulletCooldown_ = kBulletCool;

		// ★発射反動
		shootRecoilTimer_ = kShootRecoilTime_;

		// リセット
		isCharging_ = false;
		chargeTime_ = 0.0f;
		chargePulse_ = 0.0f;
		chargeReady_ = false;
		chargeReadyFlash_ = 0.0f;
		chargePopTimer_ = 0.0f;
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

	UpdateWireDots();
	wireDotWts_.reserve(128);


	// 行列更新（ここだけでOK：InputMove内では更新しない）
	Vector3 corrected = worldTransform_.translation_;
	corrected.y -= modelYOffset_;
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, corrected);
	worldTransform_.TransferMatrix();
}

void Player::UpdateWireAimDebug(const Ray& mouseRay) 
{
	if (!wireDebugDraw_ || !mapChipField_) {
		return;
	}

	Vector3 mouseOnPlane{};
	if (!IntersectPlaneZ(mouseRay, worldTransform_.translation_.z, mouseOnPlane)) {
		return;
	}

	Vector3 origin = worldTransform_.translation_;
	Vector3 dir = {mouseOnPlane.x - origin.x, mouseOnPlane.y - origin.y, 0.0f};
	float len2 = dir.x * dir.x + dir.y * dir.y;
	if (len2 < 0.000001f) {
		return;
	}
	dir = MyMath::Normalize3(dir);

	Rect r00 = mapChipField_->GetRectByIndex(0, 0);
	float cellW = (r00.right - r00.left);
	float cellH = (r00.top - r00.bottom);
	float cell = (std::min)(cellW, cellH);
	float maxLen = cell * 10.0f;

	wireAimFrom_ = origin;
	wireAimTo_ = {origin.x + dir.x * maxLen, origin.y + dir.y * maxLen, origin.z};

	wireAimHasHit_ = FindWireHitPoint(origin, dir, maxLen, wireAimHit_);
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
	}
	if (right) {
		velocity_.x += accel;
		lrDirection_ = LRDirection::kRight;
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
	if (wireActive_) {

		Vector3 pos = worldTransform_.translation_;

		// ---- 2D化：Zは固定（レイのzズレで変な方向にならない）
		Vector3 to{wireTarget_.x - pos.x, wireTarget_.y - pos.y, 0.0f};

		float dist2 = to.x * to.x + to.y * to.y;
		if (dist2 <= wireStop_ * wireStop_) {
			wireActive_ = false;
			velocity_.x = 0.0f;
			velocity_.y = 0.0f;
			return;
		}

		Vector3 dir = MyMath::Normalize3(to);

		// ★向きをワイヤー方向に合わせる（turnで逆向きに見えるの防止）
		lrDirection_ = (dir.x >= 0.0f) ? LRDirection::kRight : LRDirection::kLeft;

		// ---- dt を BehaviorRootUpdate に渡してない設計なので 60fps 換算で進める
		// wireSpeed_ を「1秒あたり」とするなら 1/60 を掛ける
		const float dt = 1.0f / 60.0f;

		Vector3 move{dir.x * wireSpeed_ * dt, dir.y * wireSpeed_ * dt, 0.0f};

		// 行き過ぎ防止（2D距離で判定）
		float move2 = move.x * move.x + move.y * move.y;
		if (move2 >= dist2) {
			worldTransform_.translation_.x = wireTarget_.x;
			worldTransform_.translation_.y = wireTarget_.y;
			// zは維持
			wireActive_ = false;
			velocity_.x = 0.0f;
			velocity_.y = 0.0f;
			return;
		}

		CollisionInfo info{};
		info.move = move;

		CheckCollisionMap(info);

		// 壁に当たったら解除
		if (info.isHitLeft || info.isHitRight || info.isHitTop || info.isHitBottom) {
			wireActive_ = false;
			velocity_.x = 0.0f;
			velocity_.y = 0.0f;
			return;
		}

		ApplyCollisionResult(info);
		return;
	}
	// スケール戻す
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	attackScaleTimer_ = 0.0f;

	// ★追加：攻撃の前のめり等を確実に戻す
	worldTransform_.rotation_.x = 0.0f;
	worldTransform_.rotation_.z = 0.0f;

	// =======================
	// ★チャージ演出（分かりやすい版）
	// =======================
	if (isCharging_) {
		// ぷるぷる（溜めるほど激しい）
		float t = chargePulse_; // 0..1
		float wobble = std::sin(chargeTime_ * 24.0f) * (0.03f + 0.07f * t);

		// 基本の膨らみ：溜めるほど大きく
		float base = 1.0f + 0.10f * t;

		// チャージ中は「縦に圧縮→横に膨張」で “溜めてる感”
		worldTransform_.scale_.x *= (base + wobble);
		worldTransform_.scale_.y *= (1.0f - 0.06f * t - wobble * 0.5f);

		// しきい値到達後は点滅みたいに “鼓動” を強める（スケールで表現）
		if (chargeReady_) {
			float beat = (std::sin(chargeReadyFlash_ * 6.0f) * 0.5f + 0.5f); // 0..1
			float bump = 1.0f + 0.06f * beat;
			worldTransform_.scale_.x *= bump;
			worldTransform_.scale_.y *= bump;
		}
	}

	// 完了時の「ドン！」（1回だけ大きく）
	if (chargePopTimer_ > 0.0f) {
		chargePopTimer_ -= (1.0f / 60.0f);
		float u = std::clamp(chargePopTimer_ / kChargePopTime_, 0.0f, 1.0f); // 1→0
		// 1→0 を使って、最初が一番大きい
		float pop = 1.0f + 0.35f * u;
		worldTransform_.scale_.x *= pop;
		worldTransform_.scale_.y *= pop;
	}

	// 発射反動：一瞬だけ縮む（撃った感が出る）
	if (shootRecoilTimer_ > 0.0f) {
		shootRecoilTimer_ -= (1.0f / 60.0f);
		float u = std::clamp(shootRecoilTimer_ / kShootRecoilTime_, 0.0f, 1.0f); // 1→0
		float recoil = 1.0f - 0.25f * u;
		worldTransform_.scale_.x *= recoil;
		worldTransform_.scale_.y *= recoil;
	}

	// 入力による移動
	// 入力による移動（ワイヤー中は入力で上書きしない）
	if (!wireActive_) {
		InputMove();
	}

	// =======================
	// 振り返りイージング
	// =======================
	const float targetY = (lrDirection_ == LRDirection::kRight) ? +std::numbers::pi_v<float> / 2.0f : -std::numbers::pi_v<float> / 2.0f;

	// 「まだ目標と違う」なら回し始める
	if (std::fabs(worldTransform_.rotation_.y - targetY) > 0.0001f) {

		// 初回（または目標が変わった瞬間）に開始角を保存
		if (turnTimer_ <= 0.0f || turnTimer_ >= kTimeTurn) {
			turnFirstRotationY_ = worldTransform_.rotation_.y;
			turnTimer_ = 0.0f;
		}

		// タイマー進行（deltaTimeが取れるなら deltaTime を使う）
		// 今の君のコードは 1/60 を多用してるので、それに合わせるならこれでOK
		turnTimer_ += 1.0f / 60.0f;

		float t = std::clamp(turnTimer_ / kTimeTurn, 0.0f, 1.0f);
		float e = EaseInOut(t);

		// イージング補間
		worldTransform_.rotation_.y = turnFirstRotationY_ + (targetY - turnFirstRotationY_) * e;

		// 完了したら固定
		if (t >= 1.0f) {
			worldTransform_.rotation_.y = targetY;
			turnTimer_ = kTimeTurn; // 終了状態にしておく
		}
	} else {
		// もう向いてるならタイマーは終端に
		turnTimer_ = kTimeTurn;
	}

	const bool left = Input::GetInstance()->PushKey(DIK_A);
	const bool right = Input::GetInstance()->PushKey(DIK_D);
	const bool jumpPressed = Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->TriggerKey(DIK_W);

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
	case MapChipType::kChargeBreakable: // ★追加：普段は壁
		return true;
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

bool Player::BulletHitMap(const Bullet& b) const {
	if (!mapChipField_)
		return false;

	static constexpr float kBulletBaseScale = 0.35f;

	float half = kBulletHalf * (b.wt.scale_.x / kBulletBaseScale);

	// 進行方向（xだけ動いてる前提）
	float dir = (b.vel.x >= 0.0f) ? +1.0f : -1.0f;

	// 先端ポイント
	Vector3 tip = b.wt.translation_;
	tip.x += dir * half;

	// ついでに上下も少し見る（太い弾の抜け防止）
	Vector3 tipUp = tip;
	tipUp.y += half * 0.6f;
	Vector3 tipDn = tip;
	tipDn.y -= half * 0.6f;

	auto isSolidAt = [&](const Vector3& pos) {
		IndexSet idx = mapChipField_->GetMapChipIndexByPosition(pos);
		MapChipType t = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
		return IsSolidForSwitch(t, blocksAreRed_);
	};

	return isSolidAt(tip) || isSolidAt(tipUp) || isSolidAt(tipDn);
}

void Player::UpdateBullets(float dt, const std::list<Enemy*>& enemies) {

	for (auto it = bullets_.begin(); it != bullets_.end();) {
		Bullet& b = *it;

		b.life -= dt;
		if (b.life <= 0.0f) {
			it = bullets_.erase(it);
			continue;
		}

		// 移動
		b.wt.translation_.x += b.vel.x * dt;

		// ----------------------------
		// ① マップヒット判定（Indexも取る）
		// ----------------------------
		IndexSet hitIdx{};
		MapChipType hitType = MapChipType::kBlank;

		if (GetBulletHitMapInfo(b, hitIdx, hitType)) {

			// ★チャージ弾 + 5番なら「壊して貫通（回数消費）」
			if (b.pierce && hitType == MapChipType::kChargeBreakable) {

				// ブロック消す
				mapChipField_->SetMapChipTypeByIndex(hitIdx.xIndex, hitIdx.yIndex, MapChipType::kBlank);

				// 貫通回数を減らす（敵でもブロックでも共通で減る）
				b.pierceLeft--;
				if (b.pierceLeft <= 0) {
					it = bullets_.erase(it);
					continue;
				}

				// まだ貫通残ってるので弾は残す
			} else {
				// それ以外の壁・ブロックは普通に弾が消える
				it = bullets_.erase(it);
				continue;
			}
		}

		// ----------------------------
		// ② 敵ヒット判定（貫通対応）
		// ----------------------------
		static constexpr float kBulletBaseScale = 0.35f;
		float half = kBulletHalf * (b.wt.scale_.x / kBulletBaseScale);

		AABB ba;
		Vector3 c = b.wt.translation_;
		ba.min = {c.x - half, c.y - half, c.z - half};
		ba.max = {c.x + half, c.y + half, c.z + half};

		bool erased = false;

		for (Enemy* e : enemies) {
			if (!e)
				continue;
			if (e->IsDead() || e->IsDying())
				continue;

			if (!IntersectsAABB(ba, e->GetAABB()))
				continue;

			// 既に同じ敵に当ててたらスキップ（貫通弾で多段ヒット防止）
			if (b.pierce) {
				if (b.hitSet.contains(e)) {
					continue;
				}
				b.hitSet.insert(e);
			}

			e->OnHit(1, worldTransform_.translation_);

			if (b.pierce) {
				b.pierceLeft--;
				if (b.pierceLeft <= 0) {
					it = bullets_.erase(it);
					erased = true;
				}
				// まだ残ってるなら弾は残して次へ
			} else {
				it = bullets_.erase(it);
				erased = true;
			}
			break;
		}

		if (erased) {
			continue;
		}

		++it;
	}
}

// Player.cpp
std::vector<IndexSet> Player::ConsumeBrokenChargeBlocks() {
	std::vector<IndexSet> out;
	out.swap(brokenChargeBlocks_);
	return out;
}

void Player::DrawBullets() {
	if (!bulletModel_) {
		return;
	}

	for (auto& b : bullets_) {
		if (!b.alive)
			continue;

		b.wt.matWorld_ = MakeAffineMatrix(b.wt.scale_, b.wt.rotation_, b.wt.translation_);
		b.wt.TransferMatrix();

		bulletModel_->Draw(b.wt, *camera_, textureHandle_ /*←弾専用があるなら差し替え*/);
	}
}

void Player::SpawnBulletWithPower(float power01) {
	if (!bulletModel_)
		return;

	bullets_.emplace_back();
	Bullet& b = bullets_.back();

	b.alive = true;

	// 強さで寿命・サイズ・速度を変える（通常=0 / チャージ=1）
	b.life = kBulletLife + power01 * 0.8f;

	b.wt.Initialize();

	Vector3 p = worldTransform_.translation_;
	float dir = (lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f;

	// 発射位置
	p.x += dir * (kWidth * 0.65f);
	p.y += 0.2f;

	b.wt.translation_ = p;

	// サイズ（通常小・チャージ大）
	float s = 0.35f + power01 * 0.65f; // 0.35 → 1.0
	b.wt.scale_ = {s, s, s};
	b.wt.rotation_ = {0, 0, 0};

	// 速度（チャージの方が速い）
	float spd = kBulletSpeed + power01 * 10.0f;
	b.vel = {dir * spd, 0.0f, 0.0f};

	// power01 が 1 に近いならチャージ弾扱い
	const bool isCharge = (power01 >= 0.999f);

	b.pierce = isCharge;
	b.pierceLeft = isCharge ? kChargePierceCount_ : 0;
	b.hitSet.clear();

	b.wt.matWorld_ = MakeAffineMatrix(b.wt.scale_, b.wt.rotation_, b.wt.translation_);
	b.wt.TransferMatrix();
}

bool Player::GetBulletHitMapInfo(const Bullet& b, IndexSet& outIdx, MapChipType& outType) const {
	if (!mapChipField_) {
		return false;
	}

	static constexpr float kBulletBaseScale = 0.35f;
	float half = kBulletHalf * (b.wt.scale_.x / kBulletBaseScale);

	float dir = (b.vel.x >= 0.0f) ? +1.0f : -1.0f;

	Vector3 tip = b.wt.translation_;
	tip.x += dir * half;

	Vector3 tipUp = tip;
	tipUp.y += half * 0.6f;
	Vector3 tipDn = tip;
	tipDn.y -= half * 0.6f;

	auto check = [&](const Vector3& pos) -> bool {
		IndexSet idx = mapChipField_->GetMapChipIndexByPosition(pos);
		MapChipType t = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);
		if (IsSolidForSwitch(t, blocksAreRed_)) {
			outIdx = idx;
			outType = t;
			return true;
		}
		return false;
	};

	if (check(tip))
		return true;
	if (check(tipUp))
		return true;
	if (check(tipDn))
		return true;

	return false;
}

void Player::StartWireTo(const KamataEngine::Vector3& target) {
	wireTarget_ = target;
	wireActive_ = true;
}

void Player::UpdateWire(float dt) {
	if (!wireActive_) {
		return;
	}

	Vector3 pos = worldTransform_.translation_;
	Vector3 to{wireTarget_.x - pos.x, wireTarget_.y - pos.y, wireTarget_.z - pos.z};

	float dist2 = to.x * to.x + to.y * to.y + to.z * to.z;
	if (dist2 <= wireStop_ * wireStop_) {
		wireActive_ = false;
		velocity_.x = 0.0f;
		velocity_.y = 0.0f;
		return;
	}

	Vector3 dir = MyMath::Normalize3(to);

	// “秒速” → “このフレームの移動量”
	Vector3 move{dir.x * wireSpeed_ * dt, dir.y * wireSpeed_ * dt, 0.0f};

	// 行き過ぎ防止
	float move2 = move.x * move.x + move.y * move.y + move.z * move.z;
	if (move2 >= dist2) {
		worldTransform_.translation_ = wireTarget_;
		wireActive_ = false;
		velocity_.x = 0.0f;
		velocity_.y = 0.0f;
		return;
	}

	// “move量”としてvelocity_に入れる（君の設計に合わせる）
	velocity_.x = move.x;
	velocity_.y = move.y;
}


// 直線上の最初の壁を探して、引っかかり点(outHit)を返す
bool Player::FindWireHitPoint(
    const Vector3& origin,
    const Vector3& dirN, // 正規化済み
    float maxLen, Vector3& outHit) const {
	if (!mapChipField_) {
		return false;
	}

	// ステップ幅：マップチップの1/4くらいで十分安定（小さすぎると重い）
	// Rect を使ってセルサイズを拾う（0,0 が取れる前提）
	Rect r00 = mapChipField_->GetRectByIndex(0, 0);
	float cellW = (r00.right - r00.left);
	float cellH = (r00.top - r00.bottom);
	float step = (std::min)(cellW, cellH) * 0.25f;
	if (step <= 0.0001f) {
		step = 0.25f;
	}

	// 直線を前進
	Vector3 prevPos = origin;

	// 「最初から壁の中」対策：少し前に出た所から始める
	float t = step;

	for (; t <= maxLen; t += step) {
		Vector3 p = {origin.x + dirN.x * t, origin.y + dirN.y * t, origin.z + dirN.z * t};

		IndexSet idx = mapChipField_->GetMapChipIndexByPosition(p);
		MapChipType type = mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex);

		if (IsSolidForSwitch(type, blocksAreRed_)) {
			// 当たったセルのRect
			Rect rect = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);

			// prevPos -> p のどっち側から入ったかで、面を決める（簡易）
			// 直線状にある壁に “引っかかる” ならこれで十分
			Vector3 hit = p;

			// 左から入った
			if (prevPos.x < rect.left && p.x >= rect.left) {
				hit.x = rect.left;
			}
			// 右から入った
			else if (prevPos.x > rect.right && p.x <= rect.right) {
				hit.x = rect.right;
			}

			// 下から入った
			if (prevPos.y < rect.bottom && p.y >= rect.bottom) {
				hit.y = rect.bottom;
			}
			// 上から入った
			else if (prevPos.y > rect.top && p.y <= rect.top) {
				hit.y = rect.top;
			}

			// めり込み防止：少し手前で止める（壁面から少し離す）
			const float back = (std::min)(step, 0.15f);
			hit.x -= dirN.x * back;
			hit.y -= dirN.y * back;
			hit.z -= dirN.z * back;

			outHit = hit;
			return true;
		}

		prevPos = p;
	}

	return false;
}
void Player::StartWireByMouseRay(const Ray& mouseRay) {
	Vector3 mouseOnPlane{};
	if (!IntersectPlaneZ(mouseRay, worldTransform_.translation_.z, mouseOnPlane)) {
		return;
	}

	Vector3 origin = worldTransform_.translation_;
	Vector3 dir = {mouseOnPlane.x - origin.x, mouseOnPlane.y - origin.y, 0.0f};

	const float len2 = dir.x * dir.x + dir.y * dir.y;
	if (len2 < 0.000001f) {
		return;
	}
	dir = MyMath::Normalize3(dir);

	Rect r00 = mapChipField_->GetRectByIndex(0, 0);
	float cellW = (r00.right - r00.left);
	float cellH = (r00.top - r00.bottom);
	float cell = (std::min)(cellW, cellH);
	float maxLen = cell * 10.0f;

	Vector3 hit{};
	if (FindWireHitPoint(origin, dir, maxLen, hit)) {

		StartWireTo(hit);

		// ★長さ（可視化用の値だけ保持）
		Vector3 d = {hit.x - origin.x, hit.y - origin.y, 0.0f};
		wireDebugLength_ = std::sqrt(d.x * d.x + d.y * d.y);

	} else {
		// 壁が無い時は長さ0
		wireDebugLength_ = 0.0f;
	}
}


void Player::DrawWireDots() const {
	// ワイヤー中 OR ガイド表示中 のどちらかなら描く
	const bool draw = wireActive_ || (guideVisible_ && hasGuideTarget_);
	if (!draw) {
		return;
	}

	for (const auto& p : wireDotWts_) {
		if (!p)
			continue;
		model_->Draw(*p, *camera_, textureHandle_);
	}
}



void Player::UpdateWireDots() {
	// ワイヤー中なら wireTarget_、そうでないなら guideTarget_
	bool active = wireActive_;
	if (!active) {
		if (!hasGuideTarget_) {
			wireDotWts_.clear();
			return;
		}
	}

	Vector3 a = worldTransform_.translation_;
	Vector3 b = active ? wireTarget_ : guideTarget_;

	Vector3 ab{b.x - a.x, b.y - a.y, 0.0f};
	float dist2 = ab.x * ab.x + ab.y * ab.y;
	if (dist2 < 1e-6f) {
		wireDotWts_.clear();
		return;
	}
	float dist = std::sqrt(dist2);
	Vector3 dir = MyMath::Normalize3(ab);

	// ★ここが “目立たなくする” パラメータ
	float spacing = active ? 0.6f : 1.4f;  // ガイドは間隔広め
	float sSmall = active ? 0.18f : 0.10f; // ガイドは小さめ
	float sBig = active ? 0.32f : 0.14f;   // ガイド終点も控えめ
	int maxDots = active ? 64 : 18;        // ガイドは本数制限

	int count = (int)(dist / spacing);
	if (count < 1)
		count = 1;
	if (count > maxDots)
		count = maxDots;

	int total = count + 1;

	while ((int)wireDotWts_.size() < total) {
		auto wt = std::make_unique<KamataEngine::WorldTransform>();
		wt->Initialize();
		wt->rotation_ = {0, 0, 0};
		wireDotWts_.push_back(std::move(wt));
	}
	while ((int)wireDotWts_.size() > total) {
		wireDotWts_.pop_back();
	}

	for (int i = 0; i < total; ++i) {
		float t = (total <= 1) ? 1.0f : (float)i / (float)(total - 1);

		Vector3 p{a.x + dir.x * (dist * t), a.y + dir.y * (dist * t), a.z};

		float s = (i == total - 1) ? sBig : sSmall;

		auto& wt = *wireDotWts_[i];
		wt.translation_ = p;
		wt.scale_ = {s, s, s};

		wt.matWorld_ = MakeAffineMatrix(wt.scale_, wt.rotation_, wt.translation_);
		wt.TransferMatrix();
	}
}

void Player::SetWireGuideTarget(const Vector3& p) {
	guideTarget_ = p;
	hasGuideTarget_ = true;
}

void Player::UpdateWireGuideByMouseRay(const Ray& mouseRay) {
	if (!mapChipField_) {
		return;
	}

	// マウスがいる平面上の点（プレイヤーと同じZ平面）
	Vector3 mouseOnPlane{};
	if (!IntersectPlaneZ(mouseRay, worldTransform_.translation_.z, mouseOnPlane)) {
		hasGuideTarget_ = false;
		return;
	}

	Vector3 origin = worldTransform_.translation_;

	// 2D化（Z固定）
	Vector3 dir = {mouseOnPlane.x - origin.x, mouseOnPlane.y - origin.y, 0.0f};
	float len2 = dir.x * dir.x + dir.y * dir.y;
	if (len2 < 1e-6f) {
		hasGuideTarget_ = false;
		return;
	}
	dir = MyMath::Normalize3(dir);

	// “届く距離” = セルサイズ×10（StartWireByMouseRay と同じルール）
	Rect r00 = mapChipField_->GetRectByIndex(0, 0);
	float cellW = (r00.right - r00.left);
	float cellH = (r00.top - r00.bottom);
	float cell = (std::min)(cellW, cellH);
	float maxLen = cell * 10.0f;

	// 壁があればそこまで、無ければ最大距離まで
	Vector3 hit{};
	if (FindWireHitPoint(origin, dir, maxLen, hit)) {
		guideTarget_ = hit;
	} else {
		guideTarget_ = {origin.x + dir.x * maxLen, origin.y + dir.y * maxLen, origin.z};
	}

	hasGuideTarget_ = true;
}
