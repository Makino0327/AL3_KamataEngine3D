#include "Bullet.h"
#include "MapChipField.h"
#include <cmath>

using KamataEngine::Vector3;

void Bullet::Initialize(KamataEngine::Model* model, const KamataEngine::Camera* cam, MapChipField* map, const Vector3& pos, const Vector3& dir, float speed, float radius) {
	model_ = model;
	camera_ = cam;
	map_ = map;
	pos_ = pos;
	vel_ = dir; // 事前に正規化して渡している想定
	speed_ = speed;
	radius_ = radius;
	alive_ = true;
	life_ = 0.0f;
}

// ==== MapChipField 連携 ====
// グリッドの有効範囲
static inline bool OutOfMap(int gx, int gy) { return (gx < 0 || gy < 0 || gx >= static_cast<int>(MapChipField::kNumBlockHorizontal) || gy >= static_cast<int>(MapChipField::kNumBlockVirtical)); }

// そのセルが「固体」か？（Blank以外）
bool Bullet::IsSolidCell_(int gx, int gy) const {
	if (!map_ || OutOfMap(gx, gy))
		return true; // 範囲外は衝突扱いで消す
	auto t = map_->GetMapChipTypeByIndex(static_cast<uint32_t>(gx), static_cast<uint32_t>(gy));
	return t != MapChipType::kBlank;
}

// 移動線分を「タイル幅ごと」にサンプリングしてヒット判定（簡易だが確実）
bool Bullet::HitMapAlongSegment_(const Vector3& p0, const Vector3& p1) const {
	if (!map_)
		return false;

	// 線分長とステップ数
	const float tile = MapChipField::kBlockWidth; // = 2.0f
	Vector3 d{p1.x - p0.x, p1.y - p0.y, p1.z - p0.z};
	float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
	if (len < 1e-6f) {
		// その場でセル判定
		int gx = static_cast<int>(std::floor(p0.x / tile));
		int gy = static_cast<int>(std::floor(p0.y / tile));
		return IsSolidCell_(gx, gy);
	}

	int steps = std::max(1, static_cast<int>(std::ceil(len / (tile * 0.5f))));
	float invSteps = 1.0f / steps;

	for (int i = 0; i <= steps; ++i) {
		float t = i * invSteps;
		Vector3 p{p0.x + d.x * t, p0.y + d.y * t, p0.z + d.z * t};
		int gx = static_cast<int>(std::floor(p.x / tile));
		int gy = static_cast<int>(std::floor(p.y / tile));
		if (IsSolidCell_(gx, gy))
			return true;
	}
	return false;
}

void Bullet::Update(float dt) {
	if (!alive_)
		return;

	// 寿命
	life_ += dt;
	if (life_ > 5.0f) {
		alive_ = false;
		return;
	}

	// 位置更新（移動前→移動後で線分衝突）
	Vector3 prev = pos_;
	pos_.x += vel_.x * speed_ * dt;
	pos_.y += vel_.y * speed_ * dt;
	pos_.z += vel_.z * speed_ * dt;

	if (HitMapAlongSegment_(prev, pos_)) {
		alive_ = false;
		return;
	}

	// 遠すぎたら消す（保険）
	const float kMaxDist = MapChipField::kNumBlockHorizontal * MapChipField::kBlockWidth + 20.0f;
	float d2 = pos_.x * pos_.x + pos_.y * pos_.y + pos_.z * pos_.z;
	if (d2 > kMaxDist * kMaxDist) {
		alive_ = false;
		return;
	}
}

void Bullet::Draw() {
	if (!alive_ || !model_)
		return;

	KamataEngine::WorldTransform wt{};
	wt.Initialize();
	wt.translation_ = pos_;
	wt.scale_ = {radius_, radius_, radius_};
	wt.matWorld_ = MakeAffineMatrix(wt.scale_, wt.rotation_, wt.translation_);
	// または wt.TransferMatrix(); / wt.Update(); のどちらか
	wt.TransferMatrix(); 

	model_->Draw(wt, *camera_);
}
