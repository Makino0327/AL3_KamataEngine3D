#pragma once
#include "KamataEngine.h"
#include "Vector.h"

// 前方宣言
class Player;

static inline const float kInterpolationRate = 0.1f;
static inline const float kVelocityBias = 35.0f; // カメラの移動速度のバイアス
// Y方向デッドゾーン（上に広く、下も少し広げる）
static inline const float kDeadZoneTop = 120.0f;    // 上方向の許容(大きくすると動きにくくなる)
static inline const float kDeadZoneBottom = -80.0f; // 下方向の許容(負の値)
static inline const float kLerpY = 0.07f;           // Yの追従の遅さ（0〜1、小さいほどヌルい）


static inline const Rect kCameraMargin = {
    -50.0f, // left
    150.0f, // right
    -30.0f, // bottom
    80.0f   // top
};

class CameraController
{
private:
	KamataEngine::Camera *camera_;
	Player* target_ = nullptr;
	Vector3 targetOffset_ = {0.0f, 0.0f, -30.0f}; // ターゲットからのオフセット
	Rect movableArea_ = {0, 100, 0, 100};
	Vector3 targetPosition_;

	

public:

	void Initialize();

	void Update();

	void SetTarget(Player* target) { target_ = target; }

	void Reset();

	 void SetCamera(Camera* camera) { camera_ = camera; }



	 void SetMovableArea(const Rect& area) 
	 {
		 movableArea_ = area;
	 }
};

Vector3 Lerp(const Vector3& start, const Vector3& end, float t);
