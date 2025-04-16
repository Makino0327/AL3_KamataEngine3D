#pragma once
#include "KamataEngine.h"
class GameScene {
public:

	uint32_t textureHandle_ = 0;
	KamataEngine::Sprite* sprite_ = nullptr;

public:

	~GameScene();

	void Initialize();

	void Update();

	void Draw();
};
