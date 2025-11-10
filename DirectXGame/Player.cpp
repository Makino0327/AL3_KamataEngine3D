#include "Player.h"
#include "Enemy.h"

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model;
	camera_ = camera;
	worldTransform_.Initialize();
	modelYOffset_ = (modelHeight * worldTransform_.scale_.y) / 100.0f;

	worldTransform_.translation_ = {position.x, position.y, position.z};
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	textureHandle_ = TextureManager::Load("./Resources/GameOver/GameOver.png");

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
	onGround_ = false;
}

void Player::Update(float deltaTime) {
	// 攻撃開始
	if (behaviorState_ == BehaviorState::kRoot && Input::GetInstance()->TriggerKey(DIK_E)) {
		behaviorState_ = BehaviorState::kAttack;
		attackTimer_ = 0.0f;
	}

	// 状態ごとの更新
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
		}
		break;
	}

	// ワールド行列の更新（スケールを適用）
	Vector3 correctedTranslation = worldTransform_.translation_;
	correctedTranslation.y -= modelYOffset_;
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, correctedTranslation);
	worldTransform_.TransferMatrix();
}




void Player::Draw() { 
	model_->Draw(worldTransform_, *camera_, textureHandle_);
}

void Player::InputMove() {
	const float accel = kAcceleration;
	const float maxSpeed = kLimitRunSpeed;
	const float friction = kAttenuation;

	// 入力取得
	bool left = Input::GetInstance()->PushKey(DIK_A);
	bool right = Input::GetInstance()->PushKey(DIK_D);

	// 入力に応じた速度変化
	if (left) {
		velocity_.x -= accel;
		lrDirection_ = LRDirection::kLeft;
		worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f; // 左向き
	}
	if (right) {
		velocity_.x += accel;
		lrDirection_ = LRDirection::kRight;
		worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f; // 右向き
	}

	// 摩擦減衰（入力なし時）
	if (!left && !right) {
		velocity_.x *= (1.0f - friction);
		if (std::fabs(velocity_.x) < 0.001f) {
			velocity_.x = 0.0f;
		}
	}

	// 最大速度制限
	velocity_.x = std::clamp(velocity_.x, -maxSpeed, maxSpeed);

	// --- 行列更新 ---
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}


void Player::CheckCollisionMapTop(CollisionInfo& info) {
	if (info.move.y <= 0.0f)
		return;

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));
	}

	bool hit = false;
	MapChipType mapChipType = MapChipType::kBlank, mapChipTypeNext = MapChipType::kBlank;
	IndexSet indexSet{0, 0};

	// 左上
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftTop]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex + 1); // ★上を見る

		if (IsSolidForSwitch(mapChipType, blocksAreRed_) && !IsSolidForSwitch(mapChipTypeNext, blocksAreRed_)) {
			Vector3 preTop = CornerPosition(worldTransform_.translation_, kLeftTop);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preTop);
			if (preIndex.yIndex != index.yIndex) {
				hit = true;
				indexSet = index;
			}
		}
	}
	// 右上
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightTop]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex + 1); // ★上を見る

		if (IsSolidForSwitch(mapChipType, blocksAreRed_) && !IsSolidForSwitch(mapChipTypeNext, blocksAreRed_)) {
			Vector3 preTop = CornerPosition(worldTransform_.translation_, kRightTop);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preTop);
			if (preIndex.yIndex != index.yIndex) {
				hit = true;
				indexSet = index;
			}
		}
	}

	if (hit) {
		Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		float playerTopY = worldTransform_.translation_.y + kHeight / 2.0f;
		float diff = rect.bottom - playerTopY;
		info.move.y = std::max(0.0f, diff);
		info.isHitTop = true;
	}
}


void Player::CheckCollisionMapBottom(CollisionInfo& info) {
	if (info.move.y >= 0.0f) {
		return; // 下に移動していない場合は無視
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));
	}

	bool hit = false;
	MapChipType mapChipType = MapChipType::kBlank;
	MapChipType mapChipTypeNext = MapChipType::kBlank;
	IndexSet indexSet;

	// 左下
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex - 1);

		if (IsSolidForSwitch(mapChipType, blocksAreRed_) && !IsSolidForSwitch(mapChipTypeNext, blocksAreRed_)) {
			Vector3 preBottom = CornerPosition(worldTransform_.translation_, kLeftBottom);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preBottom);
			if (preIndex.yIndex != index.yIndex) {
				hit = true;
				indexSet = index;
			}
		}
	}

	// 右下
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightBottom]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex - 1);

		if (IsSolidForSwitch(mapChipType, blocksAreRed_) && !IsSolidForSwitch(mapChipTypeNext, blocksAreRed_)) {
			Vector3 preBottom = CornerPosition(worldTransform_.translation_, kRightBottom);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preBottom);
			if (preIndex.yIndex != index.yIndex) {
				hit = true;
				indexSet = index;
			}
		}
	}

	if (hit) {
		// この位置であらためて indexSet を取得
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);

		Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		float playerBottomY = worldTransform_.translation_.y - kHeight / 2.0f;
		float diff = rect.top - playerBottomY;
		info.move.y = std::min(0.0f, diff);
		info.isHitBottom = true;
	}
}


void Player::CheckCollisionMapRight(CollisionInfo& info) {
	if (info.move.x <= 0.0f) {
		return;
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));
	}

	bool hit = false;
	MapChipType mapChipType = MapChipType::kBlank;
	MapChipType mapChipTypePrev = MapChipType::kBlank; // 左隣（直前側）
	IndexSet indexSet{0, 0};

	// 右上
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightTop]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		mapChipTypePrev = mapChipField_->GetMapChipTypeByIndex(index.xIndex - 1, index.yIndex);
		if (IsSolidForSwitch(mapChipType, blocksAreRed_) && !IsSolidForSwitch(mapChipTypePrev, blocksAreRed_)) {
			Vector3 preRight = CornerPosition(worldTransform_.translation_, kRightTop);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preRight);
			if (preIndex.xIndex != index.xIndex) {
				hit = true;
				indexSet = index;
			}
		}
	}
	// 右下
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightBottom]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		mapChipTypePrev = mapChipField_->GetMapChipTypeByIndex(index.xIndex - 1, index.yIndex);
		if (IsSolidForSwitch(mapChipType, blocksAreRed_) && !IsSolidForSwitch(mapChipTypePrev, blocksAreRed_)) {
			Vector3 preRight = CornerPosition(worldTransform_.translation_, kRightBottom);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preRight);
			if (preIndex.xIndex != index.xIndex) {
				hit = true;
				indexSet = index;
			}
		}
	}

	if (hit) {
		Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		float playerRightX = worldTransform_.translation_.x + kWidth / 2.0f;
		float diff = rect.left - playerRightX;
		info.move.x = std::min(0.0f, diff);
		info.isHitRight = true;
	}
}


void Player::CheckCollisionMapLeft(CollisionInfo& info) {
	if (info.move.x >= 0)
		return;

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));
	}

	MapChipType mapChipType = MapChipType::kBlank, mapChipTypePrev = MapChipType::kBlank;
	bool hit = false;

	// 左上
	IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypePrev = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex); // ★右隣＝直前側

	if (IsSolidForSwitch(mapChipType, blocksAreRed_) && !IsSolidForSwitch(mapChipTypePrev, blocksAreRed_)) {
		Vector3 preLeft = CornerPosition(worldTransform_.translation_, kLeftTop);
		IndexSet now = mapChipField_->GetMapChipIndexByPosition(preLeft);
		if (now.xIndex != indexSet.xIndex)
			hit = true;
	}

	// 左下
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypePrev = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex); // ★右隣

	if (IsSolidForSwitch(mapChipType, blocksAreRed_) && !IsSolidForSwitch(mapChipTypePrev, blocksAreRed_)) {
		Vector3 preLeft = CornerPosition(worldTransform_.translation_, kLeftBottom);
		IndexSet now = mapChipField_->GetMapChipIndexByPosition(preLeft);
		if (now.xIndex != indexSet.xIndex)
			hit = true;
	}

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
		Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		float playerLeftX = worldTransform_.translation_.x - kWidth / 2.0f;
		float diff = rect.right - playerLeftX;
		info.move.x = std::max(info.move.x, diff);
		info.isHitLeft = true;
	}
}


void Player::ChangeGroundState(const CollisionInfo& info) {
	if (onGround_) {
		// --- ジャンプ開始による空中遷移 ---
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			// 足元に地面があるか再判定（★ここをスイッチ対応に）
			std::array<Vector3, kNumCorner> positions;
			for (uint32_t i = 0; i < positions.size(); ++i) {
				Vector3 pos = CornerPosition(worldTransform_.translation_, static_cast<Corner>(i));
				if (i == kLeftBottom || i == kRightBottom) {
					pos.y += kGroundingOffsetY; // 少し下を探る
				}
				positions[i] = pos;
			}

			bool hitLeft = false;
			bool hitRight = false;

			// 左足元
			IndexSet leftIndex = mapChipField_->GetMapChipIndexByPosition(positions[kLeftBottom]);
			{
				MapChipType t = mapChipField_->GetMapChipTypeByIndex(leftIndex.xIndex, leftIndex.yIndex);
				if (IsSolidForSwitch(t, blocksAreRed_)) {
					hitLeft = true;
				}
			}
			// 右足元
			IndexSet rightIndex = mapChipField_->GetMapChipIndexByPosition(positions[kRightBottom]);
			{
				MapChipType t = mapChipField_->GetMapChipTypeByIndex(rightIndex.xIndex, rightIndex.yIndex);
				if (IsSolidForSwitch(t, blocksAreRed_)) {
					hitRight = true;
				}
			}

			if (!hitLeft && !hitRight) {
				onGround_ = false; // 地面なし → 空中へ
			}
		}
	} else {
		// --- 空中→接地切り替え ---
		if (info.isHitBottom) {
			onGround_ = true;
			velocity_.x *= (1.0f - kAttenuationLanding); // 着地時の摩擦（1回だけ）
			velocity_.y = 0.0f;
			jumpCount_ = 0;

			spinActive_ = false;
			worldTransform_.rotation_.x = 0.0f;
		}
	}
}


void Player::ProcessWallCollision(const CollisionInfo& info) {
	// 右または左の壁に接触しているならX速度を減衰
	if (info.isHitLeft || info.isHitRight) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

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

void Player::ApplyCollisionResult(const CollisionInfo& info) {
	// 移動量を座標に反映
	worldTransform_.translation_ = Add(worldTransform_.translation_, info.move);
}

void Player::CheckHitCeiling(const CollisionInfo& info) {
	if (info.isHitTop) {
		DebugText::GetInstance()->ConsolePrintf("hit ceiling\n");
		velocity_.y = 0;
	}
}

Vector3 Player::GetWorldPosition() {
	Vector3 worldPos;
	// ワールド行列の4列目（index[3][0-2]）から平行移動成分を取り出す
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Player::GetAABB() {
	Vector3 worldPos = GetWorldPosition();

	AABB aabb;
	aabb.min = {worldPos.x - 2.0f, worldPos.y - 2.0f, worldPos.z - 2.0f};
	aabb.max = {worldPos.x + 2.0f, worldPos.y + 2.0f, worldPos.z + 2.0f};

	return aabb;
}

void Player::OnCollision() {
	if (!isDead_) {
		isDead_ = true;
	}
}

void Player::BehaviorRootUpdate() {
	// スケールを戻す
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
	// タイマーリセット
	attackScaleTimer_ = 0.0f;

	// 入力による移動
	InputMove();

	bool left = Input::GetInstance()->PushKey(DIK_A);
	bool right = Input::GetInstance()->PushKey(DIK_D);
	const bool jumpPressed = Input::GetInstance()->TriggerKey(DIK_SPACE);

	// --- 壁ジャンプの先行判定（前フレームの接触情報で判断）---
	bool wantWallJump = false;
	int wallDirForJump = 0; // -1=左壁 / +1=右壁
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
		// 壁から離れる方向にキック
		velocity_.x = (wallDirForJump == -1) ? +kWallJumpVelX : -kWallJumpVelX;
		velocity_.y = kWallJumpVelY;
		onGround_ = false;
		wallSliding_ = false;
		// 壁ジャンプ後は二段不可に（＝もう使い切った扱い）
		jumpCount_ = maxJumps_;
		// スピン演出も封じる
		spinActive_ = false;
		secondJumpEvent_ = false;
	} else {
		// 通常ジャンプ（壁すべり中は二段不可）
		if (jumpPressed && jumpCount_ < maxJumps_) {
			// 2回目ジャンプになるか？
			const bool isSecondJump = (jumpCount_ == 1);

			if (velocity_.y < 0.0f)
				velocity_.y = 0.0f;
			velocity_.y = kJumpAcceleration;
			onGround_ = false;

			if (!isSecondJump) {
				firstJumpEvent_ = true;
			} else {
				// 二段ジャンプ演出（すべり中はこのブロックに入らないよう後段で制御）
				secondJumpEvent_ = true;
				spinActive_ = true;
				spinTimer_ = 0.0f;
				spinStartX_ = worldTransform_.rotation_.x;
			}
			++jumpCount_;
		}
	}

	// 空中中は重力
	if (!onGround_) {
		velocity_.y += -kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}

	CollisionInfo collisionInfo;
	collisionInfo.move = velocity_;

	// --- マップとの衝突判定 ---
	CheckCollisionMap(collisionInfo);

	// ← この時点で今フレームの壁接触が分かる
	bool hitL = collisionInfo.isHitLeft;
	bool hitR = collisionInfo.isHitRight;

	// --- 壁すべり判定 ---
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
	// すべり中は落下速度を弱め、二段を封じる
	if (wallSliding_) {
		if (velocity_.y < kWallSlideMaxFall)
			velocity_.y = kWallSlideMaxFall;
		// 二段不可：すでに1回使っている扱い
		jumpCount_ = std::max(jumpCount_, 1);
		// スピン禁止
		spinActive_ = false;
		secondJumpEvent_ = false;

		// 衝突解決に使う移動量も更新
		collisionInfo.move.y = velocity_.y;
	}

	// --- 接地 or 空中状態の切り替え ---
	ChangeGroundState(collisionInfo);

	// --- 上方向の天井判定 ---
	CheckCollisionMapTop(collisionInfo);

	// --- 衝突後の移動量を適用 ---
	ApplyCollisionResult(collisionInfo);

	// 壁との接触による速度減衰
	ProcessWallCollision(collisionInfo);

	// --- 天井に当たってたらY速度を止める ---
	CheckHitCeiling(collisionInfo);

	// このフレームの壁接触を保存（次フレームの“即時壁ジャンプ”受付用）
	prevHitLeft_ = hitL;
	prevHitRight_ = hitR;



	// --- 行列更新（Update の最後）---
	Vector3 correctedTranslation = worldTransform_.translation_;
	correctedTranslation.y -= modelYOffset_;

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, correctedTranslation);

	worldTransform_.TransferMatrix();

	DebugText::GetInstance()->ConsolePrintf("Player Pos: x=%.2f y=%.2f z=%.2f\n", worldTransform_.translation_.x, worldTransform_.translation_.y, worldTransform_.translation_.z);
}

void Player::BehaviorAttackUpdate() {
	const float attackSpeed = 0.4f;

	// 向いてる方向に攻撃移動
	velocity_.x = (lrDirection_ == LRDirection::kRight) ? +attackSpeed : -attackSpeed;

	// --- 衝突処理 ---
	CollisionInfo collisionInfo;
	collisionInfo.move = velocity_;
	CheckCollisionMap(collisionInfo);
	ChangeGroundState(collisionInfo);
	CheckCollisionMapTop(collisionInfo);
	ApplyCollisionResult(collisionInfo);
	ProcessWallCollision(collisionInfo);
	CheckHitCeiling(collisionInfo);

	// ※ スケールや行列更新は削除
}

// Player.cpp（またはPlayer.h内のprivateヘルパ）
inline bool Player::IsSolidForSwitch(MapChipType t, bool blocksAreRed) {
	switch (t) {
	case MapChipType::kBlock:
		return true; // 草は常時当たり
	case MapChipType::kBlockRed:
		return blocksAreRed; // 赤は赤ONのとき
	case MapChipType::kBlockBlue:
		return !blocksAreRed; // 青は赤OFFのとき
	default:
		return false; // それ以外は空白
	}
}



inline MapChipType Player::GetTypeSafe(int x, int y)  {
	if (x < 0 || y < 0 || x >= (int)MapChipField::kNumBlockHorizontal || y >= (int)MapChipField::kNumBlockVirtical) {
		return MapChipType::kBlank;
	}
	return mapChipField_->GetMapChipTypeByIndex((uint32_t)x, (uint32_t)y);
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
