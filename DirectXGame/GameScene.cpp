#include "GameScene.h"
#include "imgui.h"

#define NOMINMAX
#include <Windows.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

// ※ time(nullptr) を使うなら <ctime> を入れる
#include <ctime>

using namespace KamataEngine;

bool IsAABBOverlap(const AABB& a, const AABB& b) { return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z); }

namespace {

// -------------------------
// 定数（見た目/演出）
// -------------------------
constexpr float kFixedDeltaTime = 1.0f / 60.0f;

// HPバー描画用
constexpr Vector2 kHpFrameSize = {240.0f, 16.0f};
constexpr float kHpFramePadding = 2.0f;

// 被弾演出
constexpr float kHpShakeTime = 0.25f;
constexpr float kHpFlashTime = 0.15f;
constexpr float kHpDelayTime = 0.25f;
constexpr float kHpDamageSpeed = 1.5f;

// デバッグカメラ切替
constexpr int kDebugToggleKey = DIK_T;

// 画面に見せたいタイル枚数（カメラ可動範囲用）
constexpr int kVisibleTilesX = 32;
constexpr int kVisibleTilesY = 9;

// 画面余白（カメラ可動範囲用）
constexpr float kMarginXMul = 3.5f; // 3.5マス
constexpr float kMarginYMul = 1.0f; // 1マス

// キルライン（マップ最下端から何枚下に落ちたら死ぬ）
constexpr float kKillOffsetTiles = 2.0f;

inline float Clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

} // namespace

// =========================
// 当たり判定（Player vs Enemies）
// =========================
void GameScene::CheckAllCollisions() {

	if (!player_) {
		return;
	}

	const AABB playerAabb = player_->GetAABB();

	for (Enemy* enemy : enemies_) {
		if (!enemy) {
			continue;
		}

		// ★追加：死亡中・死亡済みは無視
		if (enemy->IsDead() || enemy->IsDying()) {
			continue;
		}

		const AABB enemyAabb = enemy->GetAABB();
		if (IsCollisionAABB(playerAabb, enemyAabb)) {
			player_->OnCollision();
			enemy->OnCollision(player_);
		}
	}
	// =========================
	// 当たり判定（Enemy vs Enemy）
	// =========================
	for (int iter = 0; iter < 2; ++iter) { // ★2回解決で安定
		for (auto itA = enemies_.begin(); itA != enemies_.end(); ++itA) {
			Enemy* a = *itA;
			if (!a)
				continue;
			if (a->IsDead() || a->IsDying())
				continue;

			auto itB = std::next(itA);
			for (; itB != enemies_.end(); ++itB) {
				Enemy* b = *itB;
				if (!b)
					continue;
				if (b->IsDead() || b->IsDying())
					continue;

				if (!a->CanHitEnemy() || !b->CanHitEnemy())
					continue;

				AABB aa = a->GetAABB();
				AABB bb = b->GetAABB();

				// ★接触もOKにする
				if (!IsAABBOverlap(aa, bb))
					continue;

				// ---- X方向の食い込み量（小さい方が実際の押し戻し量）----
				float pen1 = aa.max.x - bb.min.x; // aが左側っぽい時の食い込み
				float pen2 = bb.max.x - aa.min.x; // bが左側っぽい時の食い込み
				float penetration = std::min(pen1, pen2);

				// ほぼゼロなら何もしない（ノイズ対策）
				if (penetration <= 0.0f)
					continue;

				const float eps = 0.02f;
				float push = (penetration * 0.5f) + eps;

				// 中心で左右判定して、互いに押し戻す
				float acx = (aa.min.x + aa.max.x) * 0.5f;
				float bcx = (bb.min.x + bb.max.x) * 0.5f;

				if (acx < bcx) {
					a->Nudge({-push, 0.0f, 0.0f});
					b->Nudge({+push, 0.0f, 0.0f});
				} else {
					a->Nudge({+push, 0.0f, 0.0f});
					b->Nudge({-push, 0.0f, 0.0f});
				}

				// ★反転（壁と同じ）
				a->OnCollision(player_);
				b->OnCollision(player_);

				a->StartEnemyHitCooldown(0.12f);
				b->StartEnemyHitCooldown(0.12f);
			}
		}
	}
}

// =========================
// 初期化
// =========================
void GameScene::Initialize() {

	// 初期状態
	phase_ = Phase::kCountdown;

	// --- 乱数 ---
	srand(static_cast<unsigned int>(time(nullptr)));

	// --- DebugCamera ---
	debugCamera_ = new DebugCamera(1280, 720);

	// --- Camera ---
	camera_.Initialize();

	// --- Models ---
	model_ = Model::Create();

	// プレイヤー/敵/ブロック用
	playerModel_ = Model::CreateFromOBJ("grassBlock", true);
	enemyModel_ = Model::CreateFromOBJ("grassBlock", true);
	cubeModel_ = Model::CreateFromOBJ("cube", true);
	grassModel_ = Model::CreateFromOBJ("grassBlock", true);
	bulletModel_ = Model::CreateFromOBJ("bullet", true);

	// パーティクル用
	particleModel_ = Model::CreateFromOBJ("particle", true);

	// --- Texture ---
	skyDomeTexture_ = TextureManager::Load("./Resources/skyDome.png");

	// 敵のテクスチャ（今は仮でGameClearを使ってる状態を尊重）
	enemyTex = TextureManager::Load("./Resources/enemy.png");

	blockTexGrass_ = TextureManager::Load("./Resources/grassBlock/grassBlock.png");

	blockTexW_ = TextureManager::Load("./Resources/wBlock.png");

	// 赤青ブロック
	blockTexRed_ = TextureManager::Load("./Resources/cube/block_red.jpg");
	blockTexBlue_ = TextureManager::Load("./Resources/cube/block_blue.jpg");

	// --- Skydome ---
	skydome_ = new Skydome();
	skydome_->Initialize();
	skydomeModel_ = Model::CreateFromOBJ("skydome", true);

	// --- MapChip ---
	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipCsv("./Resources/blocks.csv");

	// --- Player ---
	player_ = new Player();

	// ※あなたのコードの「playerPositionを固定値にする」を尊重
	Vector3 playerPosition = {8.0f, 12.0f, 0.0f};

	player_->Initialize(playerModel_, &camera_, playerPosition);
	player_->SetMapChipField(mapChipField_);
	player_->SetBlocksAreRed(blocksAreRed_);
	player_->SetBulletModel(bulletModel_);

	// --- DeathParticles ---
	deathParticles_ = new DeathParticles();
	deathParticles_->Initialize(playerPosition, particleModel_, &camera_);

	// --- GameClear ---
	gameClearModel_ = Model::CreateFromOBJ("GameClear", true);
	gameClearTex_ = TextureManager::Load("./Resources/GameClear/GameClear.png");
	gameClearWT_.Initialize();
	gameClearWT_.scale_ = {6.0f, 6.0f, 1.0f};
	gameClearWT_.rotation_ = {0.0f, 0.0f, 0.0f};
	gameClearWT_.translation_ = {0.0f, 0.0f, 0.0f};
	gameClearWT_.matWorld_ = MakeAffineMatrix(gameClearWT_.scale_, gameClearWT_.rotation_, gameClearWT_.translation_);
	gameClearWT_.TransferMatrix();

	// --- GameOver ---
	gameOverModel_ = Model::CreateFromOBJ("GameOver", true);
	gameOverTex_ = TextureManager::Load("./Resources/GameOver/GameOver.png");
	assert(gameOverModel_ && "GameOver obj load failed");

	gameOverWT_.Initialize();
	gameOverWT_.translation_ = {-18.0f, 5.0f, 0.0f};
	gameOverWT_.rotation_ = {std::numbers::pi_v<float> / 2.0f, 0.0f, 0.0f};
	gameOverWT_.scale_ = {6.0f, 6.0f, 6.0f};
	gameOverWT_.matWorld_ = MakeAffineMatrix(gameOverWT_.scale_, gameOverWT_.rotation_, gameOverWT_.translation_);
	gameOverWT_.TransferMatrix();

	// --- CameraController ---
	cameraController_.SetCamera(&camera_);
	cameraController_.SetTarget(player_);
	cameraController_.Reset();

	// 可動範囲（左手座標系：MapChipFieldの定数を使用）
	const float BW = MapChipField::kBlockWidth;
	const float BH = MapChipField::kBlockHeight;
	const uint32_t NX = MapChipField::kNumBlockHorizontal;
	const uint32_t NY = MapChipField::kNumBlockVirtical;

	const float halfW = (kVisibleTilesX * BW) * 0.5f;
	const float halfH = (kVisibleTilesY * BH) * 0.5f;

	const float marginX = kMarginXMul * BW;
	const float marginY = kMarginYMul * BH;

	Rect area{};
	area.left = halfW - BW * 0.5f - marginX;
	area.right = (NX * BW) - halfW - BW * 0.5f + marginX;
	area.bottom = halfH - BH * 0.5f - marginY;
	area.top = (NY * BH) - halfH - BH * 0.5f + marginY;
	cameraController_.SetMovableArea(area);

	// --- Fade ---
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Status::FadeIn, 1.0f);

	// --- Goal ---
	goalModel_ = Model::CreateFromOBJ("door", true);
	goalTex_ = TextureManager::Load("./Resources/door/door.png");
	goalWT_.Initialize();

	const float tileW = MapChipField::kBlockWidth;
	const float tileH = MapChipField::kBlockHeight;

	hasGoal_ = false;
	for (uint32_t y = 0; y < MapChipField::kNumBlockVirtical; ++y) {
		for (uint32_t x = 0; x < MapChipField::kNumBlockHorizontal; ++x) {
			if (mapChipField_->GetMapChipTypeByIndex(x, y) == MapChipType::kGoal) {
				Vector3 center = mapChipField_->GetMapChipPositionByIndex(x, y);

				goalWT_.translation_ = center;
				goalWT_.rotation_ = {0.0f, 0.0f, 0.0f};
				goalWT_.scale_ = {1.0f, 1.0f, 1.0f};
				goalWT_.matWorld_ = MakeAffineMatrix(goalWT_.scale_, goalWT_.rotation_, goalWT_.translation_);
				goalWT_.TransferMatrix();

				goalArea_.min = {center.x - tileW * 0.5f, center.y - tileH * 0.5f, -0.5f};
				goalArea_.max = {center.x + tileW * 0.5f, center.y + tileH * 0.5f, +0.5f};

				hasGoal_ = true;
				break;
			}
		}
		if (hasGoal_) {
			break;
		}
	}

	// フォールバック
	if (!hasGoal_) {
		const uint32_t mapCols = MapChipField::kNumBlockHorizontal;
		const uint32_t mapRows = MapChipField::kNumBlockVirtical;

		goalArea_.min = {(mapCols - 1) * tileW - tileW * 0.5f, 0.0f, -0.5f};
		goalArea_.max = {(mapCols - 1) * tileW + tileW * 0.5f, mapRows * tileH, +0.5f};

		Vector3 colCenter = {(mapCols - 1) * tileW, (mapRows * tileH) * 0.5f, 0.0f};
		goalWT_.translation_ = colCenter;
		goalWT_.matWorld_ = MakeAffineMatrix(goalWT_.scale_, goalWT_.rotation_, goalWT_.translation_);
		goalWT_.TransferMatrix();
	}

	SpawnEnemyGridByBlocks(30, 0, -18);
	SpawnEnemyGridByBlocks(16, 0, -19);
	SpawnEnemyGridByBlocks(13, 0, -19);
	SpawnEnemyGridByBlocks(18, 0, -19);
	SpawnEnemyGridByBlocks(40, 0, -17);
	SpawnEnemyGridByBlocks(46, 0, -14);
	SpawnEnemyGridByBlocks(50, 0, -17);

	// --- Blocks生成 ---
	GenerateBlocks();

	// --- HPバー初期化（ここが超重要：最初に出ない原因を潰す） ---
	hpBarTex_ = TextureManager::Load("./Resources/white1x1.png");

	// 3枚とも “同じ1x1” を使い、色で枠/緑/赤を表現する
	hpFrame_ = Sprite::Create(hpBarTex_, hpBarPos_);
	hpFill_ = Sprite::Create(hpBarTex_, hpBarPos_);
	hpDamage_ = Sprite::Create(hpBarTex_, hpBarPos_);

	// 初期HP状態
	prevHp_ = player_->GetHP();
	hpRate_ = Clamp01(prevHp_ / float(player_->GetMaxHP()));
	hpDamageRate_ = hpRate_;

	hpShakeTimer_ = 0.0f;
	hpFlashTimer_ = 0.0f;
	hpDamageDelay_ = 0.0f;

	// GameScene.cpp Initialize() の最後の方でOK

	pauseOverlayTex_ = TextureManager::Load("./Resources/white1x1.png");

	pauseOverlaySprite_.reset(KamataEngine::Sprite::Create(pauseOverlayTex_, {0.0f, 0.0f}));

	// 画面全体を覆うサイズにする（1280x720は自分のウィンドウサイズに合わせて）
	pauseOverlaySprite_->SetSize({1280.0f, 720.0f});

	// 左上基準にしたい場合（Spriteが中心基準なら不要。基準が違うなら調整）
	pauseOverlaySprite_->SetAnchorPoint({0.0f, 0.0f});

	pauseTexHandle_ = TextureManager::Load("./Resources/pouse.png");

	pauseSprite_ = Sprite::Create(
	    pauseTexHandle_, {640.0f, 150.0f} // 画面中央
	);

	// 中央揃え
	pauseSprite_->SetAnchorPoint({0.5f, 0.5f});

	// ゲームに戻る
	pauseBackGame_ = Sprite::Create(TextureManager::Load("./Resources/backtoGame.png"), {470.0f, 350.0f}, {0.5f, 0.5f});

	// タイトルに戻る
	pauseBackTitle_ = Sprite::Create(TextureManager::Load("./Resources/backtoTitle.png"), {470.0f, 430.0f}, {0.5f, 0.5f});
	prevMap_.resize(MapChipField::kNumBlockVirtical);
	for (auto& line : prevMap_) {
		line.resize(MapChipField::kNumBlockHorizontal, MapChipType::kBlank);
	}
	for (uint32_t y = 0; y < MapChipField::kNumBlockVirtical; ++y) {
		for (uint32_t x = 0; x < MapChipField::kNumBlockHorizontal; ++x) {
			prevMap_[y][x] = mapChipField_->GetMapChipTypeByIndex(x, y);
		}
	}

	// --- BGM（Titleの真似） ---
	gameBGMHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/GameBGM.mp3");

	// ループあり（true）
	gameBGMPlayingId_ = Audio::GetInstance()->PlayWave(gameBGMHandle_, true);
	gameBGMStarted_ = true;

	gameClearBGMHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/GameClear.mp3");
	gameOverBGMHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/GameOver.mp3");

	playedGameOverBGM_ = false;
	playedClearBGM_ = false;

	deathHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/death.mp3");
	deathStarted_ = false;
	seDecideHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/kettei.mp3");
}

// =========================
// Play中 更新
// =========================
void GameScene::UpdatePlay(float deltaTime) {

	// ブロックスイッチ状態をPlayerへ
	player_->SetBlocksAreRed(blocksAreRed_);

	for (auto* e : enemies_) {
		if (e) {
			e->SetBlocksAreRed(blocksAreRed_);
		}
	}

	// ---------------------------------
	// 赤/青切替（地上Space or 2段ジャンプ）
	// ---------------------------------
	if (Input::GetInstance()->TriggerKey(DIK_SPACE) && player_->IsOnGround()) {
		blocksAreRed_ = !blocksAreRed_;
	}
	if (player_->ConsumeSecondJumpEvent()) {
		blocksAreRed_ = !blocksAreRed_;
	}

	// ---------------------------------
	// 死亡チェック（早期リターン）
	// ---------------------------------
	if (player_->IsDead()) {
		const Vector3& pos = player_->GetWorldPosition();

		if (!deathParticles_) {
			deathParticles_ = new DeathParticles();
		}
		deathParticles_->Initialize(pos, particleModel_, &camera_);
		deathParticles_->isActive_ = true;

		phase_ = Phase::kDeath;
		return;
	}

	// ---------------------------------
	// Player更新
	// ---------------------------------
	player_->Update(deltaTime);

	player_->UpdateBullets(deltaTime, enemies_);

	// ★チャージ破壊ブロックの破片演出
	{
		auto broken = player_->ConsumeBrokenChargeBlocks();
		for (const auto& idx : broken) {
			// 見た目はとりあえず草ブロックでOK（今のDrawと合わせる）
			SpawnBlockBreakEffect(idx.xIndex, idx.yIndex, blockTexGrass_, grassModel_);
		}
	}

	// ---------------------------------
	// 落下死（キルライン）
	// ---------------------------------
	{
		const float kDeathY = -2.0f;

		if (player_->GetPosition().y < kDeathY) {
			player_->KillByFall(); // ★即死

			// ★ここで Death 演出開始して即 return（これが重要）
			const Vector3 pos = player_->GetWorldPosition();

			if (!deathParticles_) {
				deathParticles_ = new DeathParticles();
			}
			deathParticles_->Initialize(pos, particleModel_, &camera_);
			deathParticles_->isActive_ = true;

			phase_ = Phase::kDeath;
			return;
		}
	}

	// ---------------------------------
	// Enemy更新（重複ループは1回だけ）
	// ---------------------------------
	/*if (enemyA_) {
	    enemyA_->Update();
	}
	if (enemyB_) {
	    enemyB_->Update();
	}*/

	for (auto* e : enemies_) {
		if (e) {
			e->Update();
		}
	}

	// ---------------------------------
	// カメラ更新
	// ---------------------------------
	cameraController_.Update();

	// ---------------------------------
	// クリア判定（PlayerAABB vs GoalAABB）
	// ---------------------------------
	{
		AABB p = player_->GetAABB();
		if (IsCollisionAABB(p, goalArea_)) {
			phase_ = Phase::kGameClear;
			return;
		}
	}

	// ---------------------------------
	// デバッグカメラ切替
	// ---------------------------------
#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(kDebugToggleKey)) {
		isDebugCameraActive_ = true;
	}
#endif

	// カメラ行列更新
	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
	}

	// ---------------------------------
	// ブロック行列転送（必要なら）
	// ---------------------------------
	for (auto& line : worldTransformBlocks_) {
		for (auto* wt : line) {
			if (!wt) {
				continue;
			}
			wt->matWorld_ = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
			wt->TransferMatrix();
		}
	}

	// ---------------------------------
	// 当たり判定
	// ---------------------------------

	if (player_->IsAttacking()) {
		const AABB atk = player_->GetAttackAABB();

		for (Enemy* e : enemies_) {
			if (!e)
				continue;

			// ★追加：死に演出中/死んでる敵には当てない
			if (e->IsDead() || e->IsDying())
				continue;

			if (IsAABBOverlap(atk, e->GetAABB())) {
				e->OnHit(1, player_->GetWorldPosition());
			}
		}
	}

	CheckAllCollisions();
	UpdateBlockBreakPieces(deltaTime);

	// ---------------------------------
	// HP演出更新（被弾判定は “HPが減った瞬間”）
	// ---------------------------------
	const int hpNow = player_->GetHP();

	if (hpNow < prevHp_) {
		hpShakeTimer_ = kHpShakeTime;
		hpFlashTimer_ = kHpFlashTime;
		hpDamageDelay_ = kHpDelayTime;
	}
	prevHp_ = hpNow;

	hpRate_ = Clamp01(hpNow / float(player_->GetMaxHP()));

	hpShakeTimer_ = std::max(0.0f, hpShakeTimer_ - deltaTime);
	hpFlashTimer_ = std::max(0.0f, hpFlashTimer_ - deltaTime);

	if (hpDamageDelay_ > 0.0f) {
		hpDamageDelay_ = std::max(0.0f, hpDamageDelay_ - deltaTime);
	} else {
		hpDamageRate_ = std::max(hpRate_, hpDamageRate_ - kHpDamageSpeed * deltaTime);
	}

	for (auto it = enemies_.begin(); it != enemies_.end();) {
		Enemy* e = *it;
		if (e && e->IsDead()) {
			delete e;
			it = enemies_.erase(it);
		} else {
			++it;
		}
	}

	// ★「5→0」になった場所を探して破片を出す
	for (uint32_t y = 0; y < MapChipField::kNumBlockVirtical; ++y) {
		for (uint32_t x = 0; x < MapChipField::kNumBlockHorizontal; ++x) {
			MapChipType now = mapChipField_->GetMapChipTypeByIndex(x, y);
			MapChipType prev = prevMap_[y][x];

			if (prev == MapChipType::kChargeBreakable && now == MapChipType::kBlank) {
				SpawnBlockBreakEffect(x, y, blockTexW_, cubeModel_);
			}

			prevMap_[y][x] = now;
		}
	}
}

// =========================
// Death演出 更新
// =========================
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

#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(kDebugToggleKey)) {
		isDebugCameraActive_ = true;
	}
#endif

	if (isDebugCameraActive_) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetCamera().matView;
		camera_.matProjection = debugCamera_->GetCamera().matProjection;
		camera_.TransferMatrix();
	} else {
		camera_.UpdateMatrix();
	}

	for (auto& line : worldTransformBlocks_) {
		for (auto* wt : line) {
			if (!wt) {
				continue;
			}
			wt->matWorld_ = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
			wt->TransferMatrix();
		}
	}
}

// =========================
// Phase切替（今は空でもOK）
// =========================
void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kPlay:
		break;
	case Phase::kDeath:
		break;
	default:
		break;
	}
}

// =========================
// Update（Phase別）
// =========================
void GameScene::Update() {

	const float deltaTime = kFixedDeltaTime;

	// TABでポーズ切り替え
	if (Input::GetInstance()->TriggerKey(DIK_TAB)) {
		isPaused_ = !isPaused_;
		pauseCursor_ = 0;
	}

	// ポーズ中はゲーム更新止める（とりあえず）
	if (isPaused_) {

		// 上下操作
		if (Input::GetInstance()->TriggerKey(DIK_W) || Input::GetInstance()->TriggerKey(DIK_UP)) {
			pauseCursor_ = (pauseCursor_ + 1) % 2;
		}
		if (Input::GetInstance()->TriggerKey(DIK_S) || Input::GetInstance()->TriggerKey(DIK_DOWN)) {
			pauseCursor_ = (pauseCursor_ + 1) % 2;
		}

		// 決定
		// 決定
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			if (pauseCursor_ == 0) {
				if (!sePlayed_) {
					seDecideId_ = Audio::GetInstance()->PlayWave(seDecideHandle_, false); // ループなし
					// もし音量APIがあるなら例：
					// Audio::GetInstance()->SetVolume(seDecideId_, 0.8f);
					sePlayed_ = true;
				}
				// ゲームに戻る
				isPaused_ = false;
				sePlayed_ = false;
			} else {
				if (gameBGMPlayingId_ != -1) {
					if (!sePlayed_) {
						seDecideId_ = Audio::GetInstance()->PlayWave(seDecideHandle_, false); // ループなし
						// もし音量APIがあるなら例：
						// Audio::GetInstance()->SetVolume(seDecideId_, 0.8f);
						sePlayed_ = true;
					}
					Audio::GetInstance()->StopWave(gameBGMPlayingId_);
					gameBGMPlayingId_ = -1;
					gameBGMStarted_ = false;
				}

				// タイトルに戻る
				finished_ = true;
				nextScene_ = NextScene::kTitle;
			}
		}

		return;
	}

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			isCountingDown_ = true;
			countdownTimer_ = 0.0f;
			countdownValue_ = 3;
			phase_ = Phase::kCountdown;
		}
		break;

	case Phase::kCountdown:
		countdownTimer_ += deltaTime;

		// カウント中でもゲームは動かす（あなたの仕様を尊重）
		UpdatePlay(deltaTime);

		if (countdownValue_ > 0 && countdownTimer_ >= countdownInterval_) {
			countdownTimer_ = 0.0f;
			countdownValue_--;
		}

		if (countdownValue_ == 0 && countdownTimer_ >= goHoldTime_) {
			phase_ = Phase::kPlay;
		}

		// カメラ更新
		cameraController_.Update();
		camera_.UpdateMatrix();
		break;

	case Phase::kPlay:
		UpdatePlay(deltaTime);

		for (Scenery* t : trees_) {
			if (t) {
				t->Update();
			}
		}
		for (auto* g : grasses_) {
			if (g) {
				g->Update();
			}
		}
		break;

	case Phase::kGameClear: {

		if (gameBGMPlayingId_ != -1) {
			Audio::GetInstance()->StopWave(gameBGMPlayingId_);
			gameBGMPlayingId_ = -1;
			gameBGMStarted_ = false;
		}

		if (!playedClearBGM_) {

			gameClearBGMPlayingId_ = Audio::GetInstance()->PlayWave(gameClearBGMHandle_, false);
			playedClearBGM_ = true;
		}

		camera_.UpdateMatrix();

		// 表示位置計算（今のままでOK）
		Vector3 camPos = camera_.translation_;
		Vector3 camFwd{camera_.matView.m[2][0], camera_.matView.m[2][1], camera_.matView.m[2][2]};

		float dist = 6.0f;
		gameClearWT_.translation_ = {camPos.x + camFwd.x * dist - 2.5f, camPos.y + camFwd.y * dist, camPos.z + camFwd.z * dist};

		gameClearWT_.rotation_ = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float>, 0.0f};

		gameClearWT_.scale_ = {1.0f, 1.0f, 1.0f};
		gameClearWT_.matWorld_ = MakeAffineMatrix(gameClearWT_.scale_, gameClearWT_.rotation_, gameClearWT_.translation_);
		gameClearWT_.TransferMatrix();

		// ★ここが重要
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			finished_ = true;
			nextScene_ = NextScene::kTitle; // ← タイトルに戻る指定
		}
	} break;

	case Phase::kDeath:
		if (gameBGMPlayingId_ != -1) {
			Audio::GetInstance()->StopWave(gameBGMPlayingId_);
			gameBGMPlayingId_ = -1;
			gameBGMStarted_ = false;
		}

		if (!deathStarted_) {

			deathPlayingId_ = Audio::GetInstance()->PlayWave(deathHandle_, false);
			deathStarted_ = true;
		}

		UpdateDeath();
		if (deathParticles_ && deathParticles_->IsFinished()) {
			phase_ = Phase::kGameOver;
			gameOverAnimT_ = 0.0f;
		}
		break;

	case Phase::kGameOver:
		camera_.UpdateMatrix();
		if (!playedGameOverBGM_) {
			gameOverBGMPlayingId_ = Audio::GetInstance()->PlayWave(gameOverBGMHandle_, false);
			playedGameOverBGM_ = true;
		}
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			finished_ = true;
		}

		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;

	default:
		break;
	}
}

// =========================
// Draw
// =========================
void GameScene::Draw() {

	// --- Skydome ---
	skydome_->Draw(skydomeModel_, camera_, skyDomeTexture_);

	// --- Player or DeathParticles ---
	if (!player_->IsDead()) {
		player_->Draw();
		player_->DrawBullets();
	} else {
		if (deathParticles_) {
			deathParticles_->Draw();
		}
	}

	// --- Enemies ---
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Draw();
		}
	}
	// if (enemyA_) {
	//	enemyA_->Draw();
	// }
	// if (enemyB_) {
	//	enemyB_->Draw();
	// } // ※2回描画してたのを修正

	// --- Goal ---
	if (goalModel_) {
		goalModel_->Draw(goalWT_, camera_, goalTex_);
	}

	// --- Scenery ---
	for (Scenery* t : trees_) {
		if (t) {
			t->Draw();
		}
	}
	for (auto* g : grasses_) {
		if (g) {
			g->Draw();
		}
	}

	// --- Blocks ---
	for (uint32_t i = 0; i < worldTransformBlocks_.size(); ++i) {
		for (uint32_t j = 0; j < worldTransformBlocks_[i].size(); ++j) {

			WorldTransform* wt = worldTransformBlocks_[i][j];
			if (!wt) {
				continue;
			}

			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);

			Model* drawModel = nullptr;
			uint32_t tex = 0;

			switch (type) {
			case MapChipType::kBlock:
				drawModel = grassModel_;
				tex = blockTexGrass_;
				break;
			case MapChipType::kBlockRed:
				drawModel = cubeModel_;
				tex = blockTexRed_;
				break;
			case MapChipType::kBlockBlue:
				drawModel = cubeModel_;
				tex = blockTexBlue_;
				break;
			case MapChipType::kChargeBreakable:
				drawModel = cubeModel_; // とりあえず草ブロック見た目でOK（あとで専用テクスチャでも可）
				tex = blockTexW_;
				break;

			default:
				break;
			}
			if (!drawModel) {
				continue;
			}

			Vector3 scale = wt->scale_;

			// 無効な赤青は縮める
			if (type == MapChipType::kBlockRed && !blocksAreRed_) {
				scale = {0.2f, 0.2f, 0.2f};
			}
			if (type == MapChipType::kBlockBlue && blocksAreRed_) {
				scale = {0.2f, 0.2f, 0.2f};
			}

			wt->matWorld_ = MakeAffineMatrix(scale, wt->rotation_, wt->translation_);
			wt->TransferMatrix();

			drawModel->Draw(*wt, camera_, tex);
		}
	}
	// --- Block Break Pieces ---
	for (auto& p : breakPieces_) {
		if (!p.alive) {
			continue;
		}
		if (!p.model) {
			continue;
		}
		p.model->Draw(p.wt, camera_, p.tex);
	}

	// --- Countdown（OBJを使ってるならここで） ---
	if (phase_ == Phase::kCountdown) {
		Model* m = nullptr;
		if (countdownValue_ >= 3) {
			m = countModel3_;
		} else if (countdownValue_ == 2) {
			m = countModel2_;
		} else if (countdownValue_ == 1) {
			m = countModel1_;
		} else {
			m = countModelGO_;
		}

		if (m) {
			m->Draw(countWT_, camera_);
		}
	}

	// --- GameClear ---
	if (phase_ == Phase::kGameClear) {
		if (gameClearModel_) {
			gameClearModel_->Draw(gameClearWT_, camera_, gameClearTex_);
		}
	}

	// --- GameOver ---
	if (phase_ == Phase::kGameOver) {

		camera_.UpdateMatrix();

		Vector3 camPos = camera_.translation_;
		Vector3 camFwd{camera_.matView.m[2][0], camera_.matView.m[2][1], camera_.matView.m[2][2]};

		float dist = 6.0f;
		gameOverWT_.translation_ = {camPos.x + camFwd.x * dist - 2.5f, camPos.y + camFwd.y * dist + 0.0f, camPos.z + camFwd.z * dist};

		gameOverWT_.rotation_ = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float>, 0.0f};

		gameOverWT_.scale_ = {1.0f, 1.0f, 1.0f};

		gameOverWT_.matWorld_ = MakeAffineMatrix(gameOverWT_.scale_, gameOverWT_.rotation_, gameOverWT_.translation_);
		gameOverWT_.TransferMatrix();

		gameOverModel_->Draw(gameOverWT_, camera_, gameOverTex_);
	}

	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());

	// =========================
	// HPバー描画（Play/Countdown中、かつ生存中）
	// =========================
	if (!player_->IsDead() && (phase_ == Phase::kPlay || phase_ == Phase::kCountdown)) {

		// hpFrame_/hpFill_/hpDamage_ が生成されている前提（Initializeで作ってる）
		if (hpFrame_ && hpFill_ && hpDamage_) {

			float shakeX = 0.0f;
			if (hpShakeTimer_ > 0.0f) {
				float t = hpShakeTimer_ * 60.0f;
				shakeX = std::sin(t * 12.0f) * 6.0f;
			}

			Vector2 basePos = {hpBarPos_.x + shakeX, hpBarPos_.y};
			Vector2 frameSize = kHpFrameSize;

			float innerW = frameSize.x - (kHpFramePadding * 2.0f);
			float innerH = frameSize.y - (kHpFramePadding * 2.0f);
			Vector2 innerPos = {basePos.x + kHpFramePadding, basePos.y + kHpFramePadding};

			// 枠（白）
			hpFrame_->SetPosition(basePos);
			hpFrame_->SetSize(frameSize);
			hpFrame_->SetColor({1, 1, 1, 1});
			hpFrame_->Draw();

			// 赤残像（遅れて縮む）
			hpDamage_->SetPosition(innerPos);
			hpDamage_->SetSize({innerW * hpDamageRate_, innerH});
			hpDamage_->SetColor({0.8f, 0.1f, 0.1f, 1});
			hpDamage_->Draw();

			// 緑本体（フラッシュ）
			hpFill_->SetPosition(innerPos);
			hpFill_->SetSize({innerW * hpRate_, innerH});

			if (hpFlashTimer_ > 0.0f) {
				hpFill_->SetColor({1, 1, 1, 1});
			} else {
				hpFill_->SetColor({0.1f, 0.9f, 0.1f, 1});
			}
			hpFill_->Draw();
		}
	}

	if (isPaused_) {
		// 半透明の黒にする（SetColor がある場合）
		// 0.0f=黒、alpha=0.5f くらい
		pauseOverlaySprite_->SetColor({0.0f, 0.0f, 0.0f, 0.8f});
		pauseOverlaySprite_->Draw();
		pauseSprite_->Draw();

		// 色制御
		if (pauseCursor_ == 0) {
			pauseBackGame_->SetColor({1, 1, 1, 1});
			pauseBackTitle_->SetColor({1, 1, 1, 0.4f});
		} else {
			pauseBackGame_->SetColor({1, 1, 1, 0.4f});
			pauseBackTitle_->SetColor({1, 1, 1, 1});
		}

		pauseBackGame_->Draw();
		pauseBackTitle_->Draw();
	}

	Sprite::PostDraw();

	// --- Fade ---
	if (phase_ == Phase::kFadeIn || phase_ == Phase::kFadeOut) {
		fade_->Draw();
	}
}

// =========================
// デストラクタ
// =========================
GameScene::~GameScene() {

	delete debugCamera_;
	debugCamera_ = nullptr;

	delete model_;
	model_ = nullptr;
	delete playerModel_;
	playerModel_ = nullptr;
	delete enemyModel_;
	enemyModel_ = nullptr;
	delete cubeModel_;
	cubeModel_ = nullptr;
	delete particleModel_;
	particleModel_ = nullptr;

	delete skydome_;
	skydome_ = nullptr;
	delete skydomeModel_;
	skydomeModel_ = nullptr;

	delete mapChipField_;
	mapChipField_ = nullptr;

	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();

	delete deathParticles_;
	deathParticles_ = nullptr;

	for (auto* g : grasses_) {
		delete g;
	}
	grasses_.clear();

	delete grassModel_;
	grassModel_ = nullptr;

	for (auto& line : worldTransformBlocks_) {
		for (auto* wt : line) {
			delete wt;
		}
	}
	worldTransformBlocks_.clear();

	for (Scenery* t : trees_) {
		delete t;
	}
	trees_.clear();
	delete treeModel_;
	treeModel_ = nullptr;

	// HPバー（3枚）
	delete hpFrame_;
	hpFrame_ = nullptr;
	delete hpFill_;
	hpFill_ = nullptr;
	delete hpDamage_;
	hpDamage_ = nullptr;

	// ※hpBar_ はもう使ってないなら、メンバ自体削除推奨
	delete hpBar_;
	hpBar_ = nullptr;

	// fade
	delete fade_;
	fade_ = nullptr;
	// bullet model
	delete bulletModel_;
	bulletModel_ = nullptr;
}

// =========================
// ブロック生成
// =========================
void GameScene::GenerateBlocks() {

	const uint32_t numBlockVirtical = MapChipField::kNumBlockVirtical;
	const uint32_t numBlockHorizontal = MapChipField::kNumBlockHorizontal;

	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {

			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			if (type == MapChipType::kBlank) {
				continue;
			}

			WorldTransform* wt = new WorldTransform();
			wt->Initialize();
			wt->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);

			worldTransformBlocks_[i][j] = wt;
		}
	}
}

// =========================
// 敵スポーン
// =========================
Enemy* GameScene::SpawnEnemyGrid(uint32_t gx, uint32_t gy, float yOffset) {

	Vector3 p = mapChipField_->GetMapChipPositionByIndex(gx, gy);
	p.y += yOffset;

	// Z方向補正（あなたのコードを尊重）
	Vector3 camFwd{camera_.matView.m[2][0], camera_.matView.m[2][1], camera_.matView.m[2][2]};
	p.z += (camFwd.z < 0.0f) ? -0.5f : +0.5f;

	auto* e = new Enemy();
	e->Initialize(enemyModel_, &camera_, p);
	e->SetTexture(enemyTex);
	e->SetMapChipField(mapChipField_);
	e->SetBlocksAreRed(blocksAreRed_);

	enemies_.push_back(e);
	return e;
}

Enemy* GameScene::SpawnEnemyAt(const Vector3& pos) {

	auto* e = new Enemy();
	e->Initialize(enemyModel_, &camera_, pos);
	e->SetTexture(enemyTex);

	e->SetMapChipField(mapChipField_);
	e->SetBlocksAreRed(blocksAreRed_);

	enemies_.push_back(e);
	return e;
}

Enemy* GameScene::SpawnEnemyGridByBlocks(uint32_t gx, uint32_t gy, int yBlocksOffset) {

	const float yOffset = static_cast<float>(yBlocksOffset) * MapChipField::kBlockHeight;
	return SpawnEnemyGrid(gx, gy, yOffset);
}

void GameScene::SpawnBlockBreakEffect(uint32_t x, uint32_t y, uint32_t tex, Model* model) {

	Vector3 center = mapChipField_->GetMapChipPositionByIndex(x, y);

	const float pieceScale = 0.25f;
	const float side = 2.5f;
	const float up = 6.5f;
	const float life = 2.0f;

	const Vector3 offsets[4] = {
	    {-0.20f, +0.20f, 0.0f},
	    {+0.20f, +0.20f, 0.0f},
	    {-0.20f, -0.20f, 0.0f},
	    {+0.20f, -0.20f, 0.0f},
	};

	const Vector3 vels[4] = {
	    {-side, +up,         0.0f},
	    {+side, +up,         0.0f},
	    {-side, +up * 0.75f, 0.0f},
	    {+side, +up * 0.75f, 0.0f},
	};

	const Vector3 rotVels[4] = {
	    {0.0f, 0.0f, +6.0f},
	    {0.0f, 0.0f, -6.0f},
	    {0.0f, 0.0f, +8.0f},
	    {0.0f, 0.0f, -8.0f},
	};

	for (int i = 0; i < 4; ++i) {

		// ★ ここが大事：コピーしない
		breakPieces_.emplace_back();
		BreakPiece& p = breakPieces_.back();

		p.alive = true;
		p.life = life;
		p.tex = tex;
		p.model = model;

		p.wt.Initialize();
		p.wt.translation_ = {center.x + offsets[i].x, center.y + offsets[i].y, center.z + offsets[i].z};
		p.wt.rotation_ = {0, 0, 0};
		p.wt.scale_ = {pieceScale, pieceScale, pieceScale};

		p.wt.translation_ = {center.x + offsets[i].x, center.y + offsets[i].y, center.z + offsets[i].z};
		p.wt.translation_.z -= 0.05f; // ★ちょい手前（左手座標系のまま）

		p.vel = vels[i];
		p.rotVel = rotVels[i];

		p.wt.matWorld_ = MakeAffineMatrix(p.wt.scale_, p.wt.rotation_, p.wt.translation_);
		p.wt.TransferMatrix();
	}
}

void GameScene::UpdateBlockBreakPieces(float dt) {

	const float g = 18.0f;

	for (auto& p : breakPieces_) {
		if (!p.alive) {
			continue;
		}

		p.life -= dt;
		if (p.life <= 0.0f) {
			p.alive = false;
			continue;
		}

		p.vel.y -= g * dt;

		p.wt.translation_.x += p.vel.x * dt;
		p.wt.translation_.y += p.vel.y * dt;
		p.wt.translation_.z += p.vel.z * dt;

		p.wt.rotation_.x += p.rotVel.x * dt;
		p.wt.rotation_.y += p.rotVel.y * dt;
		p.wt.rotation_.z += p.rotVel.z * dt;

		p.wt.matWorld_ = MakeAffineMatrix(p.wt.scale_, p.wt.rotation_, p.wt.translation_);
		p.wt.TransferMatrix();
	}

	// ★ erase/remove_if はしない（WorldTransformがコピー不可だから）
}

void GameScene::BreakChargeBlock(uint32_t x, uint32_t y) {
	if (mapChipField_->GetMapChipTypeByIndex(x, y) != MapChipType::kChargeBreakable) {
		return;
	}

	// ①破片生成（見た目は grass でOK）
	SpawnBlockBreakEffect(x, y, blockTexW_, cubeModel_);

	// ②マップを空にする（これが超重要：ブロック本体が残ると破片が見えないことが多い）
	mapChipField_->SetMapChipTypeByIndex(x, y, MapChipType::kBlank);
}
