#pragma once
#include "KamataEngine.h"
#include <vector>
#include <cmath>
#include "Skydome.h"
#include "MapChipField.h"
#include "Player.h"
#include "Vector.h"
#include "CameraController.h"
#include "Enemy.h" 
#include "DeathParticles.h"

class GameScene {
public:
	// 3Dモデルデータ
	KamataEngine::Model* model_ = nullptr;
	// カメラ
	KamataEngine::Camera camera_;
	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;
	// ワールドトランスフォーム
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;
	// テクスチャハンドル
	uint32_t block_ = 0;
	// デバックカメラ有効
	bool isDebugCameraActive_ = false;
	// デバックカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
	// スカイドーム
	Skydome *skydome_=nullptr;
	KamataEngine::Model* skydomeModel_ = nullptr;
	// マップチップフィールド
	MapChipField* mapChipField_;
	CameraController cameraController_;

	DeathParticles* deathParticles_ = nullptr;


	// プレイヤー
	Player* player_;
	KamataEngine::Model* playerModel_ = nullptr;
	KamataEngine::Model* particleModel_ = nullptr; // パーティクル用のモデル
	std::list<Enemy*> enemies_; // Enemyのポインタ
	KamataEngine::Model* enemyModel_ = nullptr; // または Model::CreateFromOBJ("enemy", true) など


	public:

	void Initialize();

	void Update();

	void Draw();

	~GameScene();

	void GenerateBlocks();

	// 全ての当たり判定を行う
	void CheckAllCollisions();
};
