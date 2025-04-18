#include "GameScene.h"

using namespace KamataEngine;

void GameScene::Initialize()
{
	textureHandle_ = TextureManager::Load("./Resources/uvChecker.png"); 
	model_ = Model::Create();
	worldTransform_.Initialize();
	camera_->Initialize();

	player_ = new Player();
	player_->Initialize(model_,textureHandle_,camera_);
}

void GameScene::Update() { 
	player_->Update(); 
}

void GameScene::Draw() {
	player_->Draw(); 
}

GameScene::~GameScene() {
	delete model_; 
	delete player_;
}