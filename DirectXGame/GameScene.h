#pragma once
#define NOMINMAX // ← これを *Windows.h より前* に
#include <Windows.h>
#include <algorithm> // ← std::min / std::max はここ

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
#include "Fade.h"
#include "TitleScene.h"
#include "Scenery.h"


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

	uint32_t skyDomeTexture_ = 0; // スカイドームのテクスチャハンドル
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

	// GameScene.h
	bool goDrawnThisFrame_ = false; // 今フレーム GameOver を描いたか
	bool goUseProxyCube_ = false;   // 代替：ブロックで描くトグル
	Vector3 goLastPos_{};

	// GameScene.h（GameOver用アニメ用）
	float goAnimT_ = 0.0f;
	Vector3 goBasePos_;
	bool goDrop_ = false;
	// プレイヤー
	Player* player_;
	KamataEngine::Model* playerModel_ = nullptr;
	KamataEngine::Model* particleModel_ = nullptr; // パーティクル用のモデル
	std::list<Enemy*> enemies_; // Enemyのポインタ
	KamataEngine::Model* enemyModel_ = nullptr; // または Model::CreateFromOBJ("enemy", true) など


	bool finished_ = false;

	Phase phase_ = Phase::kFadeIn;

	Fade* fade_ = nullptr; // フェード用のオブジェクト

	// 追加するメンバ
	KamataEngine::Model* gameOverModel_ = nullptr;
	KamataEngine::WorldTransform gameOverWT_;
	float gameOverAnimT_ = 0.0f; // 簡単な登場アニメ用

	AABB goalArea_{};
	// GameClear 表示用
	KamataEngine::Model* gameClearModel_ = nullptr;
	KamataEngine::WorldTransform gameClearWT_{};

	// ▼カウントダウン用
	KamataEngine::Model* countModel3_ = nullptr;
	KamataEngine::Model* countModel2_ = nullptr;
	KamataEngine::Model* countModel1_ = nullptr;
	KamataEngine::Model* countModelGO_ = nullptr;

	KamataEngine::WorldTransform countWT_;
	// GameScene のメンバに追加
	bool isCountingDown_ = false;

	float countdownTimer_ = 0.0f;
	int countdownValue_ = 3;               // 3→2→1→0(=GO表示)
	const float countdownInterval_ = 1.0f; // 各数字を出す秒数
	const float goHoldTime_ = 0.8f;        // GOを出しておく秒数

    KamataEngine::Model* treeModel_ = nullptr;

	// 背景用オブジェクトは list を使う
	std::list<Scenery*> trees_;

	 // --- grass 用 ---
	KamataEngine::Model* grassModel_ = nullptr;
	uint32_t grassTex_ = 0;
	std::list<Scenery*> grasses_; // ← 木と同じ扱い（list）

	// テクスチャハンドル（TextureManager::Load の戻り）
	uint32_t blockTexRed_ = 0;
	uint32_t blockTexBlue_ = 0;
	KamataEngine::Model* cubeModel_ = nullptr;
	uint32_t blockTexGrass_ = 0; 
	// ブロック全体の色状態（true = 赤, false = 青）
	bool blocksAreRed_ = true;

	uint32_t seGameClearHandle_ = 0;
	int seGameClearId_ = -1;
	bool seGameClearPlayed_ = false;

	uint32_t seGameOverHandle_ = 0;
	int seGameOverId_ = -1;
	bool seGameOverPlayed_ = false;

	uint32_t seBlockHandle_ = 0;
	
	uint32_t seDeathHandle_ = 0; // death.mp3
	bool playedDeathSE_ = false;

	 uint32_t bgmGameHandle_ = 0; // ゲームBGM
	uint32_t bgmGameId_ = 0;    
	bool bgmPlaying_ = false;

	uint32_t gameOverTex_ = 0;
	uint32_t gameClearTex_ = 0;
	
	public:

	void Initialize();

	void Update();

	void Draw();

	~GameScene();

	void GenerateBlocks();

	// 全ての当たり判定を行う
	void CheckAllCollisions();

	void UpdatePlay(float deltaTime);

	void UpdateDeath();

	void ChangePhase();
	bool IsFinished() const { return finished_; }


};
