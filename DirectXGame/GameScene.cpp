#include "GameScene.h"
#include "imgui.h"
#include <Windows.h>
#include <cstdlib>

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
	phase_ = Phase::kCountdown;

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

	// Initialize 内

	srand(static_cast<unsigned int>(time(nullptr)));

	player_ = new Player();
	// モデルとテクスチャは一度だけ作成
	// いろんな位置を用意
	std::vector<Vector3> enemyPositions = {
	  
	};

	enemyModel_ = Model::CreateFromOBJ("dog", true);
	uint32_t enemyTex = TextureManager::Load("./Resources/dog/Atlas_Monsters.png");
	// （PNGを使うなら）
	blockTexRed_ = TextureManager::Load("./Resources/cube/block_red.jpg");
	blockTexBlue_ = TextureManager::Load("./Resources/cube/block_blue.jpg");

	cubeModel_ = Model::CreateFromOBJ("cube", true);

	for (const auto& pos : enemyPositions) {
		Enemy* newEnemy = new Enemy();
		newEnemy->Initialize(enemyModel_, &camera_, pos);
		newEnemy->SetTexture(enemyTex);
		enemies_.push_back(newEnemy);
	}

	// 位置は適当な例
	// ブロックの要素数
	const uint32_t kNumBlockVirtical = 10;
	const uint32_t kNumBlockHorizontal = 20;
	// テクスチャの読み込み
	// ↓ キューブ → 草ブロックOBJに差し替え
	

	skyDomeTexture_ = TextureManager::Load("./Resources/skyDome.png");
	// ★CSVはそのまま（OBJを渡さない！）
	mapChipField_->LoadMapChipCsv("./Resources/blocks.csv");

	// 要素数を設定する
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}
	// 座標をマップチップ番号で指定
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 1);
	playerModel_ = Model::CreateFromOBJ("cube", true);
	playerPosition = {21.0f, 12.0f, 0.0f};
	particleModel_ = Model::CreateFromOBJ("particle", true);
	// 仮の生成処理。後で条件つけて呼び出すようにする


	// ▼カウントダウンOBJの読み込み
	//countModel3_ = Model::CreateFromOBJ("Count3", true);
	//countModel2_ = Model::CreateFromOBJ("Count2", true);
	//countModel1_ = Model::CreateFromOBJ("Count1", true);
	//countModelGO_ = Model::CreateFromOBJ("CountGO", true);

	//countWT_.Initialize();
	//// 初期スケール（Blender実寸に応じて後で調整）
	//countWT_.scale_ = {3.0f, 3.0f, 3.0f};

	player_->Initialize(playerModel_, &camera_, playerPosition); // ← テクスチャを敵に設定
	player_->SetMapChipField(mapChipField_);      
	player_->SetBlocksAreRed(blocksAreRed_); // ★これ忘れずに// プレイヤーにマップチップフィールドを設定
	deathParticles_ = new DeathParticles();
	deathParticles_->Initialize(playerPosition, particleModel_, &camera_);



	// GameClear OBJ 読み込み（Resources/GameClear/GameClear.obj を想定）
	gameClearModel_ = Model::CreateFromOBJ("GameClear", true); // 失敗時は nullptr
	gameClearTex_ = TextureManager::Load("./Resources/GameClear/GameClear.png");
	gameClearWT_.Initialize();

	// とりあえず原点・等倍（回転は後で調整）
	gameClearWT_.scale_ = {6.0f, 6.0f, 1.0f};
	gameClearWT_.rotation_ = {0.0f, 0.0f, 0.0f}; // ※寝て見えたら x を +π/2 に
	gameClearWT_.translation_ = {0.0f, 0.0f, 0.0f};

	// 行列初期転送（毎フレーム更新もします）
	gameClearWT_.matWorld_ = MakeAffineMatrix(gameClearWT_.scale_, gameClearWT_.rotation_, gameClearWT_.translation_);
	gameClearWT_.TransferMatrix();

	// GameOverモデル読み込み
	gameOverModel_ = Model::CreateFromOBJ("GameOver", true);
	gameOverTex_ = TextureManager::Load("./Resources/GameOver/GameOver.png");
	assert(gameOverModel_ && "GameOver obj load failed");

	// 変換の初期化
	gameOverWT_.Initialize();

	// 初期位置・スケール（画面中央付近に大きめ表示）
	gameOverWT_.translation_ = {-18.0f, 5.0f, 0.0f}; // y を高めに（カメラオフセットに合わせて後で調整）
	gameOverWT_.rotation_ = {std::numbers::pi_v<float> / 2.0f, 0.0f, 0.0f};

	// Blender の実寸次第で調整：大き過ぎ/小さ過ぎたらここを変える
	gameOverWT_.scale_ = {6.0f, 6.0f, 6.0f}; // 厚みは薄めでもOK

	// 行列を転送
	gameOverWT_.matWorld_ = MakeAffineMatrix(gameOverWT_.scale_, gameOverWT_.rotation_, gameOverWT_.translation_);
	gameOverWT_.TransferMatrix();

	// Initialize に追加
	cameraController_.SetCamera(&camera_);
	cameraController_.SetTarget(player_);

	cameraController_.Reset();

	// Map 実寸
	const float BW = MapChipField::kBlockWidth;
	const float BH = MapChipField::kBlockHeight;
	const uint32_t NX = MapChipField::kNumBlockHorizontal;
	const uint32_t NY = MapChipField::kNumBlockVirtical;

	// 画面に見せたいタイル枚数（整数がコツ）
	const int visibleTilesX = 32;
	const int visibleTilesY = 9;

	const float halfW = (visibleTilesX * BW) * 0.5f;
	const float halfH = (visibleTilesY * BH) * 0.5f;

	// ── 右端ゴール領域（例：右端1タイルぶんの縦柱）。左手座標系
	const float tileW = MapChipField::kBlockWidth;
	const float tileH = MapChipField::kBlockHeight;
	const uint32_t mapCols = MapChipField::kNumBlockHorizontal;
	const uint32_t mapRows = MapChipField::kNumBlockVirtical;

	// 右端の1列（mapCols-1）をゴールにする。Zは薄い柱で十分
	goalArea_.min = {(mapCols - 1) * tileW - tileW * 0.5f, 0.0f, -0.5f};
	goalArea_.max = {(mapCols - 1) * tileW + tileW * 0.5f, mapRows * tileH, +0.5f};

	// --- 余白設定 ---
	const float marginX = 3.5f * BW; // 左右に「3マスぶん」余裕（好みに応じて調整）
	const float marginY = 1.0f * BH; // 上下のマージン（必要なら）

	// --- 範囲設定 ---
	Rect area{};
	area.left = halfW - BW * 0.5f - marginX;              // 左をさらに左へ
	area.right = (NX * BW) - halfW - BW * 0.5f + marginX; // 右をさらに右へ
	area.bottom = halfH - BH * 0.5f - marginY;
	area.top = (NY * BH) - halfH - BH * 0.5f + marginY;

	cameraController_.SetMovableArea(area);

	skydomeModel_ = Model::CreateFromOBJ("skydome", true);

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Status::FadeIn, 1.0f); // 開始時にフェードイン

	GenerateBlocks();

	// GameScene.cpp の Initialize 内
	seGameClearHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/GameClear.mp3");

	// GameScene::Initialize()
	seGameOverHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/GameOver.mp3");

	seBlockHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/block.wav");

	// どこかのテクスチャ/SE読み込みと同じ並びで
	seDeathHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/death.mp3");

	// 念のため
	playedDeathSE_ = false;

	bgmGameHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/GameBGM.mp3");
}

void GameScene::UpdatePlay(float deltaTime) {
	player_->SetBlocksAreRed(blocksAreRed_);

	// Spaceで赤/青切替：地上にいる時だけ有効
	// ① 地上でSpaceを押したときに切替
	if (Input::GetInstance()->TriggerKey(DIK_SPACE) && player_->IsOnGround()) {
		blocksAreRed_ = !blocksAreRed_;
		Audio::GetInstance()->PlayWave(seBlockHandle_, false); // block.wav
	}

	// ② 2段目ジャンプが発生したフレームでも切替
	if (player_->ConsumeSecondJumpEvent()) {
		blocksAreRed_ = !blocksAreRed_;
		Audio::GetInstance()->PlayWave(seBlockHandle_, false); // block.wav
	}

	if (player_->IsDead()) {

		Audio::GetInstance()->PlayWave(seDeathHandle_, false);

		const Vector3& pos = player_->GetWorldPosition();

		if (!deathParticles_) {
			deathParticles_ = new DeathParticles();
		}
		deathParticles_->Initialize(pos, particleModel_, &camera_);

		phase_ = Phase::kDeath; // 早めに切り替える

		return;
	}

	player_->Update(deltaTime);

	if (player_->ConsumeFirstJumpEvent()) {
		
		// （任意）効果音や演出
		// PlaySE("switch.wav");
		// SpawnSwitchParticles(player_->GetWorldPosition());
	}

	const float BH = MapChipField::kBlockHeight;
	const uint32_t NY = MapChipField::kNumBlockVirtical;

	// 最下段(= NY-1) の任意x（0でOK）のタイル中心Y
	Vector3 bottomCenter = mapChipField_->GetMapChipPositionByIndex(0, NY - 1);
	float mapBottomEdgeY = bottomCenter.y - (BH * 0.5f); // タイル下端

	// キルライン（下へ余裕を持たせる：タイル2枚分くらい）
	const float killOffset = BH * 2.0f;
	float killLineY = mapBottomEdgeY - killOffset;

	// プレイヤーの中心Y（必要なら足元Yで判定しても可）
	float playerCenterY = player_->GetWorldTransform().translation_.y;

	if (playerCenterY < killLineY) {
		player_->OnCollision(); // ← 既存の死亡処理を利用（isDead_=true）
	}

	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}

	cameraController_.Update();

	// === クリア判定：プレイヤーAABB と ゴールAABB が交差したらクリア ===
	{
		AABB p = player_->GetAABB();
		if (IsCollisionAABB(p, goalArea_)) {
			phase_ = Phase::kGameClear;
			// 必要ならプレイヤー停止（自動走行を止めたいとき）
			// player_->SetVelocityX(0); など
			return;
		}
	}

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
			// ここで Play ではなく Countdown に入る
			isCountingDown_ = true;
			countdownTimer_ = 0.0f;
			countdownValue_ = 3;
			phase_ = Phase::kCountdown;
			
		}
		break;

	case Phase::kCountdown: {

		
		// 進行
		countdownTimer_ += deltaTime;
		UpdatePlay(deltaTime);

		// 1秒ごとに 3→2→1 と減少
		if (countdownValue_ > 0 && countdownTimer_ >= countdownInterval_) {
			countdownTimer_ = 0.0f;
			countdownValue_--;
		}

		// GO表示の保持 → プレイへ
		if (countdownValue_ == 0 && countdownTimer_ >= goHoldTime_) {
			phase_ = Phase::kPlay;
		}

		// カメラ行列を最新に
		cameraController_.Update();
		camera_.UpdateMatrix();

		// viewの3行目を前方向として使う（左手系）
		Vector3 camPos = camera_.translation_;
		Vector3 camFwd{camera_.matView.m[2][0], camera_.matView.m[2][1], camera_.matView.m[2][2]};

		//float dist = 6.0f; // near/far の間に収める（3〜10で調整可）
		//countWT_.translation_ = {camPos.x + camFwd.x * dist, camPos.y + camFwd.y * dist + 4.0f - 4.0f, camPos.z + camFwd.z * dist};

		//// 2) 向き（寝てるならXに+π/2、正面調整にY=π）
		//countWT_.rotation_ = {
		//    std::numbers::pi_v<float> / 2.0f, // X
		//    std::numbers::pi_v<float>,        // Y
		//    0.0f};

		//float showT = std::min(countdownTimer_ / 0.2f, 1.0f); // 最初の0.2秒でふわっと
		//float s = std::lerp(1.5f, 2.0f, showT);
		//countWT_.scale_ = {s, s, s};

		//// 転送
		//countWT_.matWorld_ = MakeAffineMatrix(countWT_.scale_, countWT_.rotation_, countWT_.translation_);
		//countWT_.TransferMatrix();
	} break;

	case Phase::kPlay:
		if (!bgmPlaying_) {
			bgmGameId_ = Audio::GetInstance()->PlayWave(bgmGameHandle_, true); // ★IDを保存
			Audio::GetInstance()->SetVolume(bgmGameId_, 0.5f); 
			bgmPlaying_ = true;
		}
		UpdatePlay(deltaTime);
		for (Scenery* t : trees_) {
			t->Update();
		}

		for (auto* g : grasses_) {
			if (g) {
				g->Update();
			}
		}


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

	// ───────── GameClear の表示（カメラ前固定）─────────
	case Phase::kGameClear: {
		
		camera_.UpdateMatrix();


		if (bgmPlaying_) {
			Audio::GetInstance()->StopWave(bgmGameId_); // ★ハンドルではなくID
			bgmPlaying_ = false;
			bgmGameId_ = 0;
		}
		if (!seGameClearPlayed_) {
			seGameClearId_ = Audio::GetInstance()->PlayWave(seGameClearHandle_, false);
			seGameClearPlayed_ = true;
		}
		// 1) カメラ位置から “少し手前(-Z)” & “少し上(+Y)” に固定配置
		// const float distZ = 6.0f;   // 手前距離（3〜10で調整）
		// const float offsetY = 4.0f; // ちょい上
		// const float offsetX = 0.0f; // 必要なら横ズラし

		// viewの3行目を前方向として使う（左手系）
		Vector3 camPos = camera_.translation_;
		Vector3 camFwd{camera_.matView.m[2][0], camera_.matView.m[2][1], camera_.matView.m[2][2]};

		float dist = 6.0f; // near/far の間に収める（3〜10で調整可）
		gameClearWT_.translation_ = {camPos.x + camFwd.x * dist - 2.5f, camPos.y + camFwd.y * dist + 4.0f - 4.0f, camPos.z + camFwd.z * dist};

		// 2) 向き（寝てるならXに+π/2、正面調整にY=π）
		gameClearWT_.rotation_ = {
		    std::numbers::pi_v<float> / 2.0f, // X
		    std::numbers::pi_v<float>,        // Y
		    0.0f};

		// 3) 大きさ
		gameClearWT_.scale_ = {1.0f, 1.0f, 1.0f};

		// 4) 行列更新＆転送（毎フレーム）
		gameClearWT_.matWorld_ = MakeAffineMatrix(gameClearWT_.scale_, gameClearWT_.rotation_, gameClearWT_.translation_);
		gameClearWT_.TransferMatrix();

		// デバッグ出力
		DebugText::GetInstance()->ConsolePrintf(
		    "[CLEAR] drawn=%d cam(%.2f,%.2f,%.2f) pos(%.2f,%.2f,%.2f)\n", gameClearModel_ ? 1 : 0, camera_.translation_.x, camera_.translation_.y, camera_.translation_.z, gameClearWT_.translation_.x,
		    gameClearWT_.translation_.y, gameClearWT_.translation_.z);

		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			finished_ = true;
		}
	} break;

	case Phase::kDeath:

		if (bgmPlaying_) {
			Audio::GetInstance()->StopWave(bgmGameId_); // ★ハンドルではなくID
			bgmPlaying_ = false;
			bgmGameId_ = 0;
		}

		if (!playedDeathSE_) { // ★ガード
			Audio::GetInstance()->PlayWave(seDeathHandle_, false);
			playedDeathSE_ = true;
		}
		UpdateDeath();

		if (deathParticles_ && deathParticles_->IsFinished()) {
			// GameOverへ
			phase_ = Phase::kGameOver;
			gameOverAnimT_ = 0.0f; // アニメ開始
		}
		break;

	case Phase::kGameOver: {
		
		if (!seGameOverPlayed_) {
			seGameOverId_ = Audio::GetInstance()->PlayWave(seGameOverHandle_, false);
			seGameOverPlayed_ = true;
		}

		// カメラ更新（固定でもOK）
		camera_.UpdateMatrix();

		// SPACE でタイトルへ
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			finished_ = true; // タイトルへ戻る（シーンマネージャ側でハンドル）
		}

	} break;

	case Phase::kFadeOut:

		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true; // シーン終了
		}
		break;
	}
}

void GameScene::Draw() {

	skydome_->Draw(skydomeModel_, camera_, skyDomeTexture_);
	if (!player_->IsDead()) {
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

	for (Scenery* t : trees_) {
		t->Draw();
	}

	for (auto* g : grasses_) {
		if (g) {
			g->Draw();
		}
	}

	for (uint32_t i = 0; i < worldTransformBlocks_.size(); ++i) {
		for (uint32_t j = 0; j < worldTransformBlocks_[i].size(); ++j) {
			KamataEngine::WorldTransform* wt = worldTransformBlocks_[i][j];
			if (!wt)
				continue;

			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);

			KamataEngine::Model* drawModel = nullptr;
			uint32_t tex = 0;

			switch (type) {
			case MapChipType::kBlock:
				drawModel = model_;
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
			default:
				break;
			}
			if (!drawModel)
				continue;

			// デフォルトのスケールをコピー
			Vector3 scale = wt->scale_;

			// 無効な赤・青は縮めるだけ
			if (type == MapChipType::kBlockRed && !blocksAreRed_) {
				scale = {0.2f, 0.2f, 0.2f};
			}
			if (type == MapChipType::kBlockBlue && blocksAreRed_) {
				scale = {0.2f, 0.2f, 0.2f};
			}

			// 行列更新
			Matrix4x4 world = MakeAffineMatrix(scale, wt->rotation_, wt->translation_);
			wt->matWorld_ = world;
			wt->TransferMatrix();

			drawModel->Draw(*wt, camera_, tex);
		}
	}

	if (phase_ == Phase::kCountdown) {
		// どのモデルを出すか選択
		KamataEngine::Model* m = nullptr;
		if (countdownValue_ >= 3)
			m = countModel3_;
		else if (countdownValue_ == 2)
			m = countModel2_;
		else if (countdownValue_ == 1)
			m = countModel1_;
		else /* 0 */
			m = countModelGO_;

		if (m) {
			m->Draw(countWT_, camera_);
		}
	}

	if (phase_ == Phase::kGameClear) {
		if (gameClearModel_) {
			gameClearModel_->Draw(gameClearWT_, camera_, gameClearTex_);
		}
		// 確認ログ（ImGuiなしでもOK）
		DebugText::GetInstance()->ConsolePrintf(
		    "[CLEAR] drawn=%d pos(%.2f, %.2f, %.2f)\n", gameClearModel_ ? 1 : 0, gameClearWT_.translation_.x, gameClearWT_.translation_.y, gameClearWT_.translation_.z);
	}

	if (phase_ == Phase::kGameOver) {
		
		camera_.UpdateMatrix();

		// viewの3行目を前方向として使う（左手系）
		Vector3 camPos = camera_.translation_;
		Vector3 camFwd{camera_.matView.m[2][0], camera_.matView.m[2][1], camera_.matView.m[2][2]};

		float dist = 6.0f; // near/far の間に収める（3〜10で調整可）
		gameOverWT_.translation_ = {camPos.x + camFwd.x * dist - 2.5f, camPos.y + camFwd.y * dist + 4.0f - 4.0f, camPos.z + camFwd.z * dist};

		// まず回転ゼロで確認（寝て見えない対策）
		gameOverWT_.rotation_ = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float>, 0.0f};

		// とにかくデカくして確認
		gameOverWT_.scale_ = {1.0f, 1.0f, 1.0f};

		gameOverWT_.matWorld_ = MakeAffineMatrix(gameOverWT_.scale_, gameOverWT_.rotation_, gameOverWT_.translation_);
		gameOverWT_.TransferMatrix();

		// ★ 本命OBJと同じWTで、草ブロックも描く（必ず見えるはず）
		// model_->Draw(gameOverWT_, camera_, block_); // プロキシ
		gameOverModel_->Draw(gameOverWT_, camera_, gameOverTex_); // 本命
	}
	// 最後にフェード
	if (phase_ == Phase::kFadeIn || phase_ == Phase::kFadeOut) {
		fade_->Draw();
	}
}

GameScene::~GameScene() {
	
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

	for (auto* g : grasses_) {
		delete g;
	}
	grasses_.clear();

	delete grassModel_;
	grassModel_ = nullptr;

	delete cubeModel_;
	cubeModel_ = nullptr;

	// ワールドトランスフォーム開放
	for (std::vector<KamataEngine::WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (KamataEngine::WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}

	for (Scenery* t : trees_) {
		delete t;
	}
	trees_.clear();

	delete treeModel_;
	treeModel_ = nullptr;

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

	// 例（GameScene::GenerateBlocks の該当箇所）
	for (uint32_t i = 0; i < MapChipField::kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < MapChipField::kNumBlockHorizontal; ++j) {
			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);
			if (type != MapChipType::kBlank) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}


