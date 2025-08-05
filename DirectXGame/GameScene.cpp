#include "GameScene.h"
#include "imgui.h"
#include <Windows.h>

using namespace KamataEngine;


void GameScene::CheckAllCollisions() {

#pragma region 自キャラと敵キャラの当たり判定
	{
		// 判定対象は2つ必要
		AABB aabb1, aabb2;

		// 自キャラのAABB
		aabb1 = player_->GetAABB();

		// 敵キャラとの総当たり判定
		for (Enemy* enemy : enemies_) {
			// 敵のAABB
			aabb2 = enemy->GetAABB();

			// AABB同士の交差判定
			if (IsCollisionAABB(aabb1, aabb2)) {
				player_->OnCollision();
				enemy->OnCollision(player_);
			}
		}
	}

#pragma endregion
}

void GameScene::Initialize() { 
	// 初期状態をプレイフェーズに
	phase_ = Phase::kPlay;

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
	for (int32_t i = 0; i < 3; ++i) {
		Enemy* newEnemy = new Enemy();
		enemyModel_ = Model::CreateFromOBJ("dog", true);
		// 各体ごとに異なる座標に配置（例: x方向に2.0fずつ離して配置）
		Vector3 enemyPosition = {20.0f + i * 2.0f, 2.0f, 0.0f};

		// 初期化
		newEnemy->Initialize(enemyModel_, &camera_, enemyPosition);
		uint32_t enemyTex = TextureManager::Load("./Resources/dog/Atlas_Monsters.png");
		newEnemy->SetTexture(enemyTex);

		// リストに追加
		enemies_.push_back(newEnemy);
	}

	 // 位置は適当な例
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
	playerModel_ = Model::CreateFromOBJ("cat", true);
	playerPosition = {2.0f, 2.0f, 0.0f};
	particleModel_ = Model::CreateFromOBJ("particle", true);
	// 仮の生成処理。後で条件つけて呼び出すようにする
	


	player_->Initialize(playerModel_, &camera_, playerPosition);            // ← テクスチャを敵に設定
	player_->SetMapChipField(mapChipField_); // プレイヤーにマップチップフィールドを設定
	deathParticles_ = new DeathParticles();
	deathParticles_->Initialize(playerPosition, particleModel_, &camera_);
	// Initialize に追加
	cameraController_.SetCamera(&camera_);
	cameraController_.SetTarget(player_);

	cameraController_.Reset();

	Rect area{};

	cameraController_.SetMovableArea(area);

	skydomeModel_ = Model::CreateFromOBJ("skydome", true);

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Status::FadeIn, 1.0f); // 開始時にフェードイン


	GenerateBlocks();
	
}

void GameScene::UpdatePlay(float deltaTime) {

	if (player_->IsDead()) {
		const Vector3& pos = player_->GetWorldPosition();

		if (!deathParticles_) {
			deathParticles_ = new DeathParticles();
		}
		deathParticles_->Initialize(pos, particleModel_, &camera_);
		phase_ = Phase::kDeath; // 早めに切り替える

		return;
	}




	player_->Update(deltaTime); 

	 for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	 }

	 cameraController_.Update();
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

	 #ifdef _DEBUG
	 if (Input::GetInstance()->TriggerKey(DIK_T)) {
		 isDebugCameraActive_ = true;
	 }
#endif
	

	for (std::vector<KamataEngine::WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (KamataEngine::WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;

			// アフィン行列の作成
			Matrix4x4 worldMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = worldMatrix;
			// 定数バッファに転送する
			worldTransformBlock->TransferMatrix();
		}
	}
	

	CheckAllCollisions();

	
}

void GameScene::UpdateDeath() {
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}
	if (deathParticles_ && deathParticles_->isActive_) {
		deathParticles_->Update();
	}

	 cameraController_.Update();
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

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_T)) {
		isDebugCameraActive_ = true;
	}
#endif

	for (std::vector<KamataEngine::WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (KamataEngine::WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)
				continue;

			// アフィン行列の作成
			Matrix4x4 worldMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
			worldTransformBlock->matWorld_ = worldMatrix;
			// 定数バッファに転送する
			worldTransformBlock->TransferMatrix();
		}
	}
	
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kPlay:
		// プレイ中の切り替え条件（プレイヤー死亡など）をここに書く
		break;

	case Phase::kDeath:
		// 今は何も書かなくてよい（演出終了後に書くことになる）
		break;
	}
}



void GameScene::Update() {
	float deltaTime = 1.0f / 60.0f;

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kPlay;
		}
		break;

	case Phase::kPlay:
		UpdatePlay(deltaTime);

		if (player_->IsDead()) {
			// 死亡時にパーティクル初期化をここでやる！
			const Vector3& pos = player_->GetWorldPosition();
			if (!deathParticles_) {
				deathParticles_ = new DeathParticles();
			}
			deathParticles_->Initialize(pos, particleModel_, &camera_);
			deathParticles_->isActive_ = true;

			// フェードアウトではなく、死亡フェーズへ移行
			phase_ = Phase::kDeath;
		}
		break;

	case Phase::kDeath:
		UpdateDeath();

		// パーティクルが終了したらフェードアウトを開始
		if (deathParticles_ && deathParticles_->IsFinished()) {
			fade_->Start(Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true; // シーン終了
		}
		break;
	}

	// 死亡パーティクル終了チェック（必要に応じて phase_ == kDeath の中に入れてもOK）
	if (deathParticles_ && deathParticles_->IsFinished()) {
		finished_ = true;
	}
}


void GameScene::Draw() { 
	skydome_->Draw(skydomeModel_, camera_);
	if (!player_->IsDead())
	{
		player_->Draw();

	} else {
		// 描画処理
		if (deathParticles_) {
			deathParticles_->Draw();
		}
	}
	
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Draw();
		}
	}


	for (std::vector<KamataEngine::WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (KamataEngine::WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock)continue;
			model_->Draw(*worldTransformBlock, camera_, block_);
		}
	}

	 // 最後にフェード
	if (phase_ == Phase::kFadeIn || phase_ == Phase::kFadeOut) {
		fade_->Draw();
	}
	
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
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	delete enemyModel_;

	delete deathParticles_;
	deathParticles_ = nullptr;

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