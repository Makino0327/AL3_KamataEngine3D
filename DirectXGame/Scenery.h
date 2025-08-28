#pragma once
#include "KamataEngine.h"
#include "Vector.h"

class Scenery {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const Vector3& position, const Vector3& scale = {1.0f, 1.0f, 1.0f}, const Vector3& rotation = {0.0f, 0.0f, 0.0f});

	void Update();
	void Draw();

	void SetTexture(uint32_t textureHandle);

	  void SetYaw(float rad);         

	   void UpdateMatrix_();   

private:
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::WorldTransform worldTransform_{};
	uint32_t textureHandle_ = 0;


};
