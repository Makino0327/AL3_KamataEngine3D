#include "Player.h"

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model;
	camera_ = camera;
	worldTransform_.Initialize();
	worldTransform_.translation_ = {position.x, position.y, position.z};
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
	textureHandle_ = TextureManager::Load("./Resources/cube/cube.jpg");

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}

void Player::Update(float deltaTime) {

	if (onGround_)
	{
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

		if (Input::GetInstance()->PushKey(DIK_SPACE)) {
			velocity_.y += kJumpAcceleration;
			// ジャンプ加速度を適用
		}

		

		if (velocity_.y < 0.0f) {
			if (worldTransform_.translation_.y <= 2.0f) {
				landing = true;
			}
		}

		// --- 接地判定 ---
		if (onGround_) {
			// ジャンプ開始判定：上方向に移動し始めたら空中へ
			if (velocity_.y > 0.0f) {
				onGround_ = false;
			}
		} else {
			// 空中にいた → 着地判定
			if (landing) {
				// めり込み修正
				worldTransform_.translation_.y = 2.0f;

				// 横速度に摩擦
				velocity_.x *= (1.0f - kAttenuation);

				// Y速度をリセット
				velocity_.y = 0.0f;

				// 接地状態に復帰
				onGround_ = true;
			}
		}


	} else
	{
		velocity_.x += 0.0f;
		velocity_.y += -kGravityAcceleration;
		velocity_.z += 0.0f;

		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
	

	// 移動
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	if (!onGround_) {
		if (velocity_.y < 0.0f && worldTransform_.translation_.y <= 2.0f) {
			worldTransform_.translation_.y = 2.0f;
			velocity_.y = 0.0f;
			velocity_.x *= (1.0f - kAttenuation);
			onGround_ = true;
			landing = true;
		}
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);


	worldTransform_.TransferMatrix();
}

void Player::Draw() { 
	model_->Draw(worldTransform_, *camera_, textureHandle_); }