#pragma once

// =========================================
// Windows / STL
// =========================================
#define NOMINMAX // Windows の min/max マクロ無効化（Windows.h より前）
#include <Windows.h>

#include <algorithm> // std::min / std::max / std::clamp など
#include <cmath>     // std::fabs / std::sin / std::cos など
#include <cstdint>   // uint32_t を安全に使う（環境によっては必須）
#include <list>      // std::list（※ enemies_ / trees_ / grasses_ で使ってる）
#include <vector>    // std::vector
#include <deque>


// =========================================
// Project / Engine
// =========================================
#include "KamataEngine.h"

// 依存ヘッダ（そのまま・順番だけ整理）
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "Fade.h"
#include "MapChipField.h"
#include "Player.h"
#include "Scenery.h"
#include "Skydome.h"
#include "TitleScene.h"
#include "Vector.h"

enum class NextScene {
	kNone = 0,
	kTitle,
	kRestart,
};

// =========================================
// GameScene
// =========================================
class GameScene {
public:
	// -----------------------------
	// Core (Model / Camera / World)
	// -----------------------------
	KamataEngine::Model* model_ = nullptr;        // 3Dモデルデータ
	KamataEngine::Camera camera_;                 // カメラ
	KamataEngine::WorldTransform worldTransform_; // ワールドトランスフォーム

	// -----------------------------
	// Blocks / Map
	// -----------------------------
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_; // ブロックWT
	uint32_t block_ = 0;                                                           // ブロック用テクスチャ等（用途名はそのまま）
	MapChipField* mapChipField_;                                                   // マップチップフィールド

	// -----------------------------
	// Sky / Debug Camera
	// -----------------------------
	uint32_t skyDomeTexture_ = 0;                      // スカイドームのテクスチャハンドル
	bool isDebugCameraActive_ = false;                 // デバックカメラ有効
	KamataEngine::DebugCamera* debugCamera_ = nullptr; // デバックカメラ
	Skydome* skydome_ = nullptr;                       // スカイドーム
	KamataEngine::Model* skydomeModel_ = nullptr;      // スカイドームモデル

	// -----------------------------
	// Camera Controller
	// -----------------------------
	CameraController cameraController_; // カメラ制御

	// -----------------------------
	// Particles
	// -----------------------------
	DeathParticles* deathParticles_ = nullptr;     // 死亡パーティクル
	KamataEngine::Model* particleModel_ = nullptr; // パーティクル用モデル

	// -----------------------------
	// GameOver (draw/anime helpers)
	// -----------------------------
	bool goDrawnThisFrame_ = false; // 今フレーム GameOver を描いたか
	bool goUseProxyCube_ = false;   // 代替：ブロックで描くトグル
	Vector3 goLastPos_{};           // 最後に描いた位置

	float goAnimT_ = 0.0f; // GameOver用アニメT
	Vector3 goBasePos_{};  // 基準位置
	bool goDrop_ = false;  // 落下演出フラグ

	// -----------------------------
	// Player
	// -----------------------------
	Player* player_;                             // プレイヤー
	KamataEngine::Model* playerModel_ = nullptr; // プレイヤーモデル

	// -----------------------------
	// Enemies
	// -----------------------------
	std::list<Enemy*> enemies_;                 // Enemyのポインタ
	KamataEngine::Model* enemyModel_ = nullptr; // Enemyモデル
	uint32_t enemyTex = 0;                      // Enemyテクスチャ

	// -----------------------------
	// Scene State / Phase
	// -----------------------------
	bool finished_ = false;
	Phase phase_ = Phase::kFadeIn; // ※ Phase がどこで定義されるかはそのまま

	// -----------------------------
	// Fade
	// -----------------------------
	Fade* fade_ = nullptr; // フェード用

	// -----------------------------
	// GameOver / GameClear (Models)
	// -----------------------------
	KamataEngine::Model* gameOverModel_ = nullptr;
	KamataEngine::WorldTransform gameOverWT_;
	float gameOverAnimT_ = 0.0f; // 登場アニメ用

	AABB goalArea_{}; // ゴール判定領域（型はそのまま）

	KamataEngine::Model* gameClearModel_ = nullptr;
	KamataEngine::WorldTransform gameClearWT_{};

	// -----------------------------
	// Countdown
	// -----------------------------
	KamataEngine::Model* countModel3_ = nullptr;
	KamataEngine::Model* countModel2_ = nullptr;
	KamataEngine::Model* countModel1_ = nullptr;
	KamataEngine::Model* countModelGO_ = nullptr;

	KamataEngine::WorldTransform countWT_;
	bool isCountingDown_ = false;

	float countdownTimer_ = 0.0f;
	int countdownValue_ = 3;               // 3→2→1→0(=GO表示)
	const float countdownInterval_ = 1.0f; // 各数字を出す秒数
	const float goHoldTime_ = 0.8f;        // GOを出しておく秒数

	// -----------------------------
	// Scenery (Trees / Grass)
	// -----------------------------
	KamataEngine::Model* treeModel_ = nullptr;
	std::list<Scenery*> trees_; // 背景：木

	KamataEngine::Model* grassModel_ = nullptr;
	uint32_t grassTex_ = 0;
	std::list<Scenery*> grasses_; // 背景：草

	// -----------------------------
	// Block Textures / Toggle
	// -----------------------------
	uint32_t blockTexRed_ = 0;
	uint32_t blockTexBlue_ = 0;
	uint32_t blockTexGrass_ = 0;
	uint32_t blockTexW_ = 0;
	KamataEngine::Model* cubeModel_ = nullptr;

	bool blocksAreRed_ = true; // true=赤 / false=青

	// -----------------------------
	// UI Textures
	// -----------------------------
	uint32_t gameOverTex_ = 0;
	uint32_t gameClearTex_ = 0;

	// -----------------------------
	// HP Bar UI
	// -----------------------------
	int prevHp_ = 0;
	Sprite* hpBar_ = nullptr;
	Vector2 hpBarPos_ = {40.0f, 620.0f};
	Vector2 hpBarMaxSize_ = {240.0f, 16.0f};
	uint32_t hpBarTex_ = 0;

	// 演出
	float hpShakeTimer_ = 0.0f;
	float hpFlashTimer_ = 0.0f;

	float hpDamageDelay_ = 0.0f;
	float hpDamageRate_ = 1.0f;
	float hpRate_ = 1.0f;

	// 枠・緑・赤残像
	Sprite* hpFrame_ = nullptr;
	Sprite* hpFill_ = nullptr;
	Sprite* hpDamage_ = nullptr;

	// -----------------------------
	// Goal
	// -----------------------------
	bool hasGoal_ = false;
	KamataEngine::Model* goalModel_ = nullptr;
	KamataEngine::WorldTransform goalWT_;
	uint32_t goalTex_ = 0;

	KamataEngine::Model* bulletModel_ = nullptr;

	// ポーズ
	bool isPaused_ = false;

	// 画面暗くする用
	uint32_t pauseOverlayTex_ = 0;
	std::unique_ptr<KamataEngine::Sprite> pauseOverlaySprite_;

	Sprite* pauseSprite_ = nullptr; // pause.png
	uint32_t pauseTexHandle_ = 0;

	int pauseCursor_ = 0; // 0:ゲームに戻る / 1:タイトルに戻る

	Sprite* pauseBackGame_ = nullptr;
	Sprite* pauseBackTitle_ = nullptr;

	// -----------------------------
	struct BreakPiece {
		WorldTransform wt;
		Vector3 vel;       // 移動速度
		Vector3 rotVel;    // 回転速度
		float life = 0.0f; // 残り寿命
		uint32_t tex = 0;
		Model* model = nullptr;
		bool alive = false;
	};

	std::deque<BreakPiece> breakPieces_;
	static constexpr int kBreakPiecesPerBlock = 4;
	std::vector<IndexSet> brokenChargeBlocks_;

	std::vector<std::vector<MapChipType>> prevMap_; // 前フレームのマップ状態

	// GameScene.h（private に追加）

	uint32_t gameBGMHandle_ = 0;
	int gameBGMPlayingId_ = -1;
	bool gameBGMStarted_ = false; // 二重再生防止
	uint32_t gameClearBGMHandle_ = 0;
	int gameClearBGMPlayingId_ = -1;

	uint32_t gameOverBGMHandle_ = 0;
	int gameOverBGMPlayingId_ = -1;

	bool playedGameOverBGM_ = false;
	bool playedClearBGM_ = false;

	uint32_t deathHandle_ = 0;
	int deathPlayingId_ = -1;
	bool deathStarted_ = false; // 二重再生防止

	uint32_t seDecideHandle_ = 0; // 決定音のハンドル
	int seDecideId_ = -1;         // 再生ID（必要なら停止用）
	bool sePlayed_ = false;       // 二重再生防止

	KamataEngine::WorldTransform wireMarkerWT_;
	bool wireMarkerActive_ = false;


public:
	// -----------------------------
	// Lifecycle
	// -----------------------------
	void Initialize();
	void Update();
	void Draw();
	~GameScene();

	// -----------------------------
	// Helpers
	// Helpers
	// -----------------------------
	void GenerateBlocks();
	void CheckAllCollisions();
	void UpdatePlay(float deltaTime);
	void UpdateDeath();

	void ChangePhase();
	bool IsFinished() const { return finished_; }

	NextScene GetNextScene() const { return nextScene_; }

	NextScene nextScene_ = NextScene::kNone;

	// -----------------------------
	// Enemy Spawns
	// -----------------------------
	Enemy* SpawnEnemyGrid(uint32_t gx, uint32_t gy, float yOffset);
	Enemy* SpawnEnemyAt(const Vector3& pos);
	Enemy* SpawnEnemyGridByBlocks(uint32_t gx, uint32_t gy, int yBlocksOffset = 0);
	void SpawnBlockBreakEffect(uint32_t x, uint32_t y, uint32_t tex, Model* model);
	void UpdateBlockBreakPieces(float dt);
	void BreakChargeBlock(uint32_t x, uint32_t y);
	std::vector<IndexSet> ConsumeBrokenChargeBlocks();
};
