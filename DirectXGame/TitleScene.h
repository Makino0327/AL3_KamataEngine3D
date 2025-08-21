#pragma once
#include "KamataEngine.h"
#include <vector>
#include <cmath>
#include "Skydome.h"
#include "MapChipField.h"
#include "Player.h"
#include "Vector.h"
#include "CameraController.h"
#include "Enemy.h" 
#include "DeathParticles.h"
#include "Fade.h"

enum class Phase {
	kFadeIn, // フェードイン中
	kCountdown,
	kPlay,   // ゲームプレイ
	kDeath,  // デス演出中
	kMain,
	kFadeOut // フェードアウト中
};

class TitleScene 
{
private:
	KamataEngine::Model* titleTextModel_ = nullptr; 
	CameraController cameraController_;
	KamataEngine::Model* playerModel_ = nullptr;
	KamataEngine::WorldTransform playerTransform_;
	KamataEngine::Camera camera_;
	Player* player_;
	WorldTransform titleTextTransform_{};
	bool finished_ = false;
	Fade* fade_ = nullptr;
	Phase phase_ = Phase::kFadeIn;

public:
	void Initialize();
	void Update();
	void Draw();
	~TitleScene();
	bool IsFinished() const { return finished_; }
};
