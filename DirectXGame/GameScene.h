#pragma once
#include "KamataEngine.h"
class GameScene {
private:
	uint32_t textureHandle_ = 0;
	float inputFloat3[3] = {0, 0, 0};

public:
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Camera camera_;
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

public:

	~GameScene() ;

	void Initialize();

	void Update();

	void Draw();
};
