#include "GameScene.h"

using namespace KamataEngine;

void GameScene::Initialize() { 
	// デバックカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);
	// カメラの初期化
	camera_.Initialize();
	// 3Dモデルデータの生成
	model_ = Model::Create();
	// ブロックの要素数
	const uint32_t kNumBlockVirtical = 10;
	const uint32_t kNumBlockHorizontal = 20;
	// ブロック一個分の横幅
	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;
	// テクスチャの読み込み
	block_ = TextureManager::Load("./Resources/cube/cube.jpg");

	// 要素数を設定する
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i)
	{
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}

	// キューブの生成
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i)
	{
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j)
		{
			if ((i + j) % 2 == 1) {
				worldTransformBlocks_[i][j] = nullptr;
				continue;
			}

			worldTransformBlocks_[i][j] = new KamataEngine::WorldTransform();
			worldTransformBlocks_[i][j]->Initialize();
			worldTransformBlocks_[i][j]->translation_.x = kBlockWidth * j;
			worldTransformBlocks_[i][j]->translation_.y = kBlockHeight*i;
		}
	}
}

void GameScene::Update() { 
	for (std::vector<KamataEngine::WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (KamataEngine::WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)continue;

			// アフィン行列の作成
			Matrix4x4 worldMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = worldMatrix;
			// 定数バッファに転送する
			worldTransformBlock->TransferMatrix();
		}
	}
	debugCamera_->Update();

	#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_T))
	{
		isDebugCameraActive_ = true;
	}
	#endif

	// カメラの処理
    if (isDebugCameraActive_) {
        // デバッグカメラの更新（キーボードやマウスで移動・回転など）
        debugCamera_->Update();

		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;

        // ビュープロジェクション行列の転送
        camera_.TransferMatrix();
    } else {
        // 通常カメラのビュープロジェクション行列を更新・転送
        camera_.UpdateMatrix();
    }
}

void GameScene::Draw() { 
	for (std::vector<KamataEngine::WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (KamataEngine::WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)continue;
			model_->Draw(*worldTransformBlock, camera_, block_);
		}
	}
}

GameScene::~GameScene()
{ 
	// デバックカメラの開放
	delete debugCamera_;
	// 3Dモデルデータの開放
	delete model_;
	// ワールドトランスフォーム開放
	for (std::vector<KamataEngine::WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_)
	{
		for (KamataEngine::WorldTransform* worldTransformBlock : worldTransformBlockLine)
		{
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();
}