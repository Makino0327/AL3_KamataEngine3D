#include "Scenery.h"

using namespace KamataEngine;

void Scenery::Initialize(Model* model, Camera* camera, const Vector3& position, const Vector3& scale, const Vector3& rotation) {
	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.scale_ = scale;
	worldTransform_.rotation_ = rotation;
	worldTransform_.translation_ = position;

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Scenery::Update() {
	// 特に動きはなし、必要なら揺らし処理など追加可
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Scenery::Draw() {
	if (model_ && camera_) {
		model_->Draw(worldTransform_, *camera_, textureHandle_);
	}
}

void Scenery::SetTexture(uint32_t textureHandle) { textureHandle_ = textureHandle; }

void Scenery::UpdateMatrix_() {
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void Scenery::SetYaw(float rad) {
	worldTransform_.rotation_.y = rad;
	UpdateMatrix_(); // ★行列更新＆転送を忘れない
}