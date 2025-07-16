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
	if (!target_) {
		return;
	}
	Vector3 targetVelocity = target_->GetVelocity();
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	
    targetPosition_.x = targetWorldTransform.translation_.x + targetOffset_.x + targetVelocity.x * kVelocityBias;
	targetPosition_.y = targetWorldTransform.translation_.y + targetOffset_.y + targetVelocity.y * kVelocityBias;
	targetPosition_.z = targetWorldTransform.translation_.z + targetOffset_.z + targetVelocity.z * kVelocityBias;


	// カメラ座標を目標座標へ補間移動
	camera_->translation_ = Lerp(camera_->translation_, targetPosition_, kInterpolationRate);

	// 範囲制限
	camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);

	// 行列を更新する
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
