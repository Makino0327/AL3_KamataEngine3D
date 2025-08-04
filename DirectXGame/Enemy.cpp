#include "Enemy.h"

void Enemy::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const Vector3& position) {
	assert(model);
	model_ = model;
	camera_ = camera;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;

	// 左向きにする（自キャラが右向きなら）
	worldTransform_.rotation_.y = std::numbers::pi_v<float>;

	worldTransform_.TransferMatrix();

	velocity_ = {-kWalkSpeed, 0.0f, 0.0f};
	walkTimer_ = 0.0f;
}

void Enemy::Update() {
	walkTimer_ += 1.0f / 60.0f; // 1フレームあたり1/60秒進める
	// 首振りアニメーション
	// 例えば 2倍速で揺らしたい場合
	float frequency = 4.0f; // 倍率（1.0fが標準、2.0fで2倍速）
	float param = std::sin(walkTimer_ * frequency);
	float t = (param + 1.0f) / 2.0f;
	worldTransform_.rotation_.z = std::lerp(kWalkMotionAngleStart, kWalkMotionAngleEnd, t);


	// 必要に応じて位置やロジックを更新する処理を書く
	worldTransform_.translation_.x += velocity_.x;
	// 変換行列の更新（SRT順）
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// 定数バッファなどに転送（描画のため）
	worldTransform_.TransferMatrix();
}

void Enemy::Draw() {
	if (model_ && camera_) {
		model_->Draw(worldTransform_, *camera_, textureHandle_);
	}
}


// Enemy.cpp
void Enemy::SetTexture(uint32_t textureHandle) { textureHandle_ = textureHandle; }
