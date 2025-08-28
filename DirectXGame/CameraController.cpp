#include "CameraController.h"
#include "Player.h"
#include <algorithm>

void CameraController::Initialize()
{
	camera_->Initialize();
	movableArea_ = kCameraMargin;
}

Vector3 Lerp(const Vector3& start, const Vector3& end, float t) { return {start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t, start.z + (end.z - start.z) * t}; }


void CameraController::Update() {
	if (!target_ || !camera_)
		return;

	Vector3 v = target_->GetVelocity();
	const WorldTransform& wt = target_->GetWorldTransform();

	// 目標位置（Xは速度バイアスあり／Yはバイアス無しがおすすめ）
	targetPosition_.x = wt.translation_.x + targetOffset_.x + v.x * kVelocityBias;
	targetPosition_.y = wt.translation_.y + targetOffset_.y; // ★Yはバイアス無し
	targetPosition_.z = wt.translation_.z + targetOffset_.z + v.z * kVelocityBias;

	// --- Yだけデッドゾーン適用 ---
	float currentY = camera_->translation_.y;
	float dy = targetPosition_.y - currentY;

	// はみ出した時だけ、はみ出し分を解消する目標Yを作る
	float targetY = currentY; // デフォルト：動かさない
	if (dy > kDeadZoneTop)
		targetY = targetPosition_.y - kDeadZoneTop;
	else if (dy < kDeadZoneBottom)
		targetY = targetPosition_.y - kDeadZoneBottom;

	// --- 追従 ---
	// XとZは従来のLerpでOK（まとめてでも可）
	camera_->translation_.x = camera_->translation_.x + (targetPosition_.x - camera_->translation_.x) * kInterpolationRate;
	camera_->translation_.z = camera_->translation_.z + (targetPosition_.z - camera_->translation_.z) * kInterpolationRate;

	// Yは遅めにスムーズ追従（デッドゾーン内なら変化なし）
	camera_->translation_.y = camera_->translation_.y + (targetY - camera_->translation_.y) * kLerpY;

	// 範囲制限（可動域Clamp）
	camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);

	camera_->UpdateMatrix();
}


void CameraController::Reset() {
	if (!target_) {
		return;
	}
	// ワールドトランスフォームを参照で取得する
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// 追従対象とオフセット分だけカメラの座標を合わせる
	camera_->translation_.x = targetWorldTransform.translation_.x + targetOffset_.x;
	camera_->translation_.y = targetWorldTransform.translation_.y + targetOffset_.y;
	camera_->translation_.z = targetWorldTransform.translation_.z + targetOffset_.z;
}
