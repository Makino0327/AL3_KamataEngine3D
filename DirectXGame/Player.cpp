#include "Player.h"
#include "Enemy.h"

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model;
	camera_ = camera;
	worldTransform_.Initialize();
	worldTransform_.translation_ = {position.x, position.y, position.z};
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	textureHandle_ = TextureManager::Load("./Resources/uvChecker.png");

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}

void Player::Update(float deltaTime) {
	// 入力による移動
	InputMove(deltaTime);

	// ジャンプ入力
	if (onGround_) {
		if (Input::GetInstance()->PushKey(DIK_SPACE)) {
			velocity_.y += kJumpAcceleration;
		}
	} else {
		// 空中中は重力加速度を加える
		velocity_.y += -kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}

	// --- 衝突前の移動量を CollisionInfo に渡す ---
	CollisionInfo collisionInfo;
	collisionInfo.move = velocity_;

	// --- マップとの衝突判定 ---
	CheckCollisionMap(collisionInfo);

	// --- 接地 or 空中状態の切り替え処理 ---
	ChangeGroundState(collisionInfo);

	// --- 上方向の天井判定 ---
	CheckCollisionMapTop(collisionInfo);

	// --- 衝突後の移動量を適用 ---
	// 衝突後の移動量を適用
	ApplyCollisionResult(collisionInfo);

	// 壁との接触による速度減衰処理を追加
	ProcessWallCollision(collisionInfo);


	// --- 壁に当たっている場合の処理 ---
	ProcessWallCollision(collisionInfo);


	// --- 天井に当たってたらY速度を止める ---
	CheckHitCeiling(collisionInfo);

	// === 行列更新 ===
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	DebugText::GetInstance()->ConsolePrintf("Player Pos: x=%.2f y=%.2f z=%.2f\n", worldTransform_.translation_.x, worldTransform_.translation_.y, worldTransform_.translation_.z);
}


void Player::Draw() { 
	model_->Draw(worldTransform_, *camera_, textureHandle_);
}

void Player::InputMove(float deltaTime) {
	// 移動入力
	if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {
		Vector3 acceleration = {};

		// 右キーが押されたとき
		if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
			// 向きが右でないなら変更
			if (lrDirection_ != LRDirection::kRight) {
				lrDirection_ = LRDirection::kRight;

				// 補間開始
				turnFirstRotationY_ = worldTransform_.rotation_.y;
				turnTimer_ = 0.0f;
			}
			// 逆向き移動ならブレーキ
			if (velocity_.x < 0.0f) {
				velocity_.x *= (1.0f - kAttenuation);
			}
			acceleration.x += kAcceleration;

		} else if (Input::GetInstance()->PushKey(DIK_LEFT)) {
			// 向きが左でないなら変更
			if (lrDirection_ != LRDirection::kLeft) {
				lrDirection_ = LRDirection::kLeft;

				// 補間開始
				turnFirstRotationY_ = worldTransform_.rotation_.y;
				turnTimer_ = 0.0f;
			}
			// 逆向き移動ならブレーキ
			if (velocity_.x > 0.0f) {
				velocity_.x *= (1.0f - kAttenuation);
			}
			acceleration.x -= kAcceleration;
		}

		velocity_.x += acceleration.x;
		velocity_.y += acceleration.y;
		velocity_.z += acceleration.z;

		velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);

		// --- 角度補間処理 ---
		if (turnTimer_ < kTimeTurn) {
			// 経過時間を進める
			turnTimer_ += deltaTime;
			float t = std::clamp(turnTimer_ / kTimeTurn, 0.0f, 1.0f);

			// イージングで補間率計算
			float easedT = EaseInOut(t);

			// 目標角度（左右）
			std::array<float, 2> destinationRotationYTable = {
			    std::numbers::pi_v<float> / 2.0f,       // 右向き (90度)
			    std::numbers::pi_v<float> * 3.0f / 2.0f // 左向き (270度)
			};
			float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

			// 補間
			worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, destinationRotationY, easedT);

		} else {
			// 補間完了後は目標角度にぴったり合わせる
			std::array<float, 2> destinationRotationYTable = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
			worldTransform_.rotation_.y = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		}

		// === プレイヤーのワールド行列更新 ===
		worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

		worldTransform_.TransferMatrix();

	} else {
		// 非入力時は移動を減衰させる
		velocity_.x *= (1.0f - kAttenuation);
	}
}

void Player::CheckCollisionMapTop(CollisionInfo& info) {
	if (info.move.y <= 0.0f) {
		return; // 上に移動していない場合は無視
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));
	}

	bool hit = false;
	MapChipType mapChipType = MapChipType::kBlank;
	MapChipType mapChipTypeNext = MapChipType::kBlank;
	IndexSet indexSet{0, 0}; // 初期化

	// 左上
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftTop]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex + 1);

		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
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
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex + 1);

		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
			Vector3 preTop = CornerPosition(worldTransform_.translation_, kRightTop);
			IndexSet preIndex = mapChipField_->GetMapChipIndexByPosition(preTop);
			if (preIndex.yIndex != index.yIndex) {
				hit = true;
				indexSet = index;
			}
		}
	}

	if (hit) {
		// この位置であらためて indexSet を取得（左上）
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftTop]);

		Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		float playerTopY = worldTransform_.translation_.y + kHeight / 2.0f;
		float diff = rect.bottom - playerTopY;
		info.move.y = std::max(0.0f, diff); // 上向きにめり込んだら下方向に戻す
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

		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
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

		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
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

void Player::ChangeGroundState(const CollisionInfo& info) {
	if (onGround_) {
		// --- ジャンプ開始による空中遷移 ---
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			

			std::array<Vector3, kNumCorner> positions;
			for (uint32_t i = 0; i < positions.size(); ++i) {
				Vector3 pos = CornerPosition(worldTransform_.translation_, static_cast<Corner>(i));
				if (i == kLeftBottom || i == kRightBottom) {
					pos.y += kGroundingOffsetY; // 少し下に補正して地面を探す
				}
				positions[i] = pos;
			}

			bool hitLeft = false;
			bool hitRight = false;

			IndexSet leftIndex = mapChipField_->GetMapChipIndexByPosition(positions[kLeftBottom]);
			if (mapChipField_->GetMapChipTypeByIndex(leftIndex.xIndex, leftIndex.yIndex) == MapChipType::kBlock) {
				hitLeft = true;
			}

			IndexSet rightIndex = mapChipField_->GetMapChipIndexByPosition(positions[kRightBottom]);
			if (mapChipField_->GetMapChipTypeByIndex(rightIndex.xIndex, rightIndex.yIndex) == MapChipType::kBlock) {
				hitRight = true;
			}

			if (!hitLeft && !hitRight) {
				onGround_ = false; // 足元にブロックがなければ空中に遷移
			}
		}
	} else {
		// --- 空中から接地に切り替える場合 ---
		if (info.isHitBottom) {
			onGround_ = true;
			velocity_.x *= (1.0f - kAttenuationLanding); // 着地時の摩擦
			velocity_.y = 0.0f;
		}
	}
}

void Player::CheckCollisionMapLeft(CollisionInfo& info) {
	if (info.move.x >= 0) {
		return; // 左に動いてないなら無視
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));
	}

	MapChipType mapChipType = MapChipType::kBlank;
	MapChipType mapChipTypeNext = MapChipType::kBlank;
	bool hit = false;

	// 左上
	IndexSet indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex); // ひとつ左

	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		Vector3 prePos = worldTransform_.translation_;
		Vector3 preLeft = CornerPosition(prePos, kLeftTop);
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(preLeft);
		if (indexSetNow.xIndex != indexSet.xIndex) {
			hit = true;
		}
	}

	// 左下
	indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);

	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		Vector3 prePos = worldTransform_.translation_;
		Vector3 preLeft = CornerPosition(prePos, kLeftBottom);
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(preLeft);
		if (indexSetNow.xIndex != indexSet.xIndex) {
			hit = true;
		}
	}

	if (hit) {
		// indexSet は再取得しなおす（未初期化回避）
		indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
		Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);

		float playerLeftX = worldTransform_.translation_.x - kWidth / 2.0f;
		float diff = rect.right - playerLeftX;

		info.move.x = std::max(info.move.x, diff); // めり込み防止
		info.isHitLeft = true;
	}
}

void Player::ProcessWallCollision(const CollisionInfo& info) {
	// 右または左の壁に接触しているならX速度を減衰
	if (info.isHitLeft || info.isHitRight) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

void Player::CheckCollisionMapRight(CollisionInfo& info) {
	if (info.move.x <= 0.0f) {
		return; // 右に移動していないなら無視
	}

	std::array<Vector3, kNumCorner> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(Add(worldTransform_.translation_, info.move), static_cast<Corner>(i));

	}

	bool hit = false;
	MapChipType mapChipType = MapChipType::kBlank;
	MapChipType mapChipTypeNext = MapChipType::kBlank;
	IndexSet indexSet{0, 0};

	// 右上
	{
		IndexSet index = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightTop]);
		mapChipType = mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex + 1, index.yIndex);

		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
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
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(index.xIndex + 1, index.yIndex);

		if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
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

void Player::OnCollision(const Enemy* enemy) {
	(void)enemy; // 未使用警告防止（後で使うかも）
}