#pragma once
#include "KamataEngine.h"
#include <vector>

class Skydome {
private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	uint32_t textureHandle_ = 0;

public:
	void Initialize(); // ← 追加
	void Draw(KamataEngine::Model* model, const KamataEngine::Camera& camera);
};
