#include "GameScene.h"
using namespace KamataEngine;

void GameScene::Initialize() {
	textureHandle_ = TextureManager::Load("tex1.png");
	sprite_ = Sprite::Create(textureHandle_, {100, 50});
}

void GameScene::Update() {
	Vector2 position = sprite_->GetPosition();
	position.x += 2.0f;
	position.y += 1.0f;

	sprite_->SetPosition(position);

}

void GameScene::Draw() {
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	Sprite::PreDraw(dxCommon->GetCommandList());

	sprite_->Draw();

	Sprite::PostDraw();
}

GameScene::~GameScene() 
{
	delete sprite_;
}