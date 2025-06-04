#include "Skydome.h"


void Skydome::Initialize() {
	worldTransform_.Initialize(); 
}


void Skydome::Draw(KamataEngine::Model* model, const KamataEngine::Camera& camera)
{
	model->Draw(worldTransform_, camera);
}
