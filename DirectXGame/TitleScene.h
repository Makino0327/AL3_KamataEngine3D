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
#include "Fade.h"

enum class Phase {
	kFadeIn, // フェードイン中
	kCountdown,
	kPlay,   // ゲームプレイ
	kDeath,  // デス演出中
	kMain,
	kHowToPlay,
	kGameClear,
	kGameOver, 
	kFadeOut, // フェードアウト中
};



class TitleScene 
{
private:
	KamataEngine::Model* titleTextModel_ = nullptr; 
	CameraController cameraController_;
	KamataEngine::Model* playerModel_ = nullptr;
	KamataEngine::WorldTransform playerTransform_;
	KamataEngine::Camera camera_;
	Player* player_;
	WorldTransform titleTextTransform_{};
	bool finished_ = false;
	Fade* fade_ = nullptr;
	Phase phase_ = Phase::kFadeIn;

	 KamataEngine::Model* treeModel_ = nullptr;
	KamataEngine::Model* grassModel1_ = nullptr;
	 uint32_t grassTex_ = 0;
	KamataEngine::WorldTransform treeTransform_;
	KamataEngine::WorldTransform grassTransform_;

	 // ★追加：ステージ用
	MapChipField* mapChipField_ = nullptr;

	// 草・赤・青の各ブロック描画に使う
	Model* grassModel_ = nullptr; // grassBlock.obj
	Model* cubeModel_ = nullptr;  // cube.obj
	uint32_t texGrass_ = 0;
	uint32_t texRed_ = 0;
	uint32_t texBlue_ = 0;

	uint32_t titleTex = 0;

	// タイル配置ごとの WT
	std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;

	 KamataEngine::Vector3 stageOffset_ = {-28.0f, -12.0f, -23.0f};
	Skydome* skydome_ = nullptr;
	KamataEngine::Model* skydomeModel_ = nullptr;
	uint32_t skyDomeTexture_ = 0; 
	uint32_t skyDomeTexture1_ = 0; 

	uint32_t titleBGMHandle_ = 0;
	int titleBGMPlayingId_ = -1;

	uint32_t seDecideHandle_ = 0; // 決定音のハンドル
	int seDecideId_ = -1;         // 再生ID（必要なら停止用）
	bool sePlayed_ = false;       // 二重再生防止

	KamataEngine::Model* plane_ = nullptr;
	KamataEngine::Camera uiCam_;
	KamataEngine::WorldTransform uiWT_;
	uint32_t texHowto_ = 0;

	KamataEngine::Model* spaceModel_ = nullptr;
	KamataEngine::WorldTransform spaceWT_;

public:
	void Initialize();
	void Update();
	void Draw();
	~TitleScene();
	bool IsFinished() const { return finished_; }
	void GenerateBlocksForTitle_();
};
