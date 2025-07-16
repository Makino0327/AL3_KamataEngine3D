#include "GameScene.h"
#include "imgui.h"
#include <Windows.h>

using namespace KamataEngine;

void GameScene::Initialize() { 
	// デバックカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);
	// カメラの初期化
	camera_.Initialize();
	// 3Dモデルデータの生成
	model_ = Model::Create();
	playerModel_ = Model::Create();
	// スカイドームの生成
	skydome_ = new Skydome();
	// skydomeの初期化
	skydome_->Initialize();
	// マップチップフィールドの生成
	mapChipField_ = new MapChipField();
	
	player_ = new Player();
	// ブロックの要素数
	const uint32_t kNumBlockVirtical = 10;
	const uint32_t kNumBlockHorizontal = 20;
	// テクスチャの読み込み
	block_ = TextureManager::Load("./Resources/cube/cube.jpg");
	mapChipField_->LoadMapChipCsv("./Resources/blocks.csv");
	// 要素数を設定する
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i)
	{
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}
	// 座標をマップチップ番号で指定
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 1);
	playerModel_ = Model::CreateFromOBJ("cube", true);
	playerPosition = {2.0f, 2.0f, 0.0f};
	player_->Initialize(playerModel_, &camera_, playerPosition);

	player_->SetMapChipField(mapChipField_); // プレイヤーにマップチップフィールドを設定

	// Initialize に追加
	cameraController_.SetCamera(&camera_);
	cameraController_.SetTarget(player_);
	cameraController_.Reset();

	Rect area{};

	cameraController_.SetMovableArea(area);

	skydomeModel_ = Model::CreateFromOBJ("skydome", true);
	GenerateBlocks();
	
}

void GameScene::Update() { 
	 float deltaTime = 1.0f / 60.0f;
	player_->Update(deltaTime);
	 cameraController_.Update();


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
	player_->Draw();
	Vector3 pos = player_->GetPosition();

	char buffer[256];
	sprintf_s(buffer, "Player Pos: x=%.2f y=%.2f z=%.2f\n", pos.x, pos.y, pos.z);

	// Visual Studio の「出力」ウィンドウに表示される
	OutputDebugStringA(buffer);

	for (std::vector<KamataEngine::WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (KamataEngine::WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)continue;
			model_->Draw(*worldTransformBlock, camera_, block_);
		}
	}
	skydome_->Draw(skydomeModel_, camera_);
}

GameScene::~GameScene()
{ 
	// デバックカメラの開放
	delete debugCamera_;
	// 3Dモデルデータの開放
	delete model_;
	// スカイドームの開放
	delete skydome_;
	delete skydomeModel_;
	// マップチップフィールドの開放
	delete mapChipField_;
	delete playerModel_;


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

void GameScene::GenerateBlocks() {
	// 要素数
	uint32_t numBlockVirtical = mapChipField_->kNumBlockVirtical;
	uint32_t numBlockHorizontal = mapChipField_->kNumBlockHorizontal;

	// 要素数を変更する
	// 列数を設定
	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		// 行数を設定
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	// ブロックの生成
	for (uint32_t i = 0; i < MapChipField::kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < MapChipField::kNumBlockHorizontal; ++j) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock)
			{
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}