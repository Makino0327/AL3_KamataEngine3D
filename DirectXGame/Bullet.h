#pragma once
#define NOMINMAX // これを入れると Windows の max マクロが無効になる
#include <Windows.h>
#include "KamataEngine.h"
#include "MapChipField.h"

class MapChipField; // ← 前方宣言だけでOK

class Bullet {
public:
	void Initialize(
	    KamataEngine::Model* model, const KamataEngine::Camera* cam,
	    MapChipField* map, // ← 追加
	    const KamataEngine::Vector3& pos,
	    const KamataEngine::Vector3& dir, // 正規化済み
	    float speed, float radius = 0.25f);

	void Update(float dt);
	void Draw();

	bool IsDead() const { return !alive_; }
	const KamataEngine::Vector3& GetPosition() const { return pos_; }

private:
	bool HitMapAlongSegment_(const KamataEngine::Vector3& p0, const KamataEngine::Vector3& p1) const;
	bool IsSolidCell_(int gx, int gy) const;

private:
	KamataEngine::Model* model_ = nullptr;
	const KamataEngine::Camera* camera_ = nullptr;
	MapChipField* map_ = nullptr;

	KamataEngine::Vector3 pos_{};
	KamataEngine::Vector3 vel_{}; // 方向
	float speed_ = 0.0f;
	float radius_ = 0.25f;

	bool alive_ = false;
	float life_ = 0.0f; // 経過秒
};