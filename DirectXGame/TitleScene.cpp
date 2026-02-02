#include "TitleScene.h"


void TitleScene::Initialize() {
	const float kFadeTime = 1.0f;

	camera_.Initialize();

	// ====== 既存どおり ======
	playerModel_ = Model::Create();
	titleTextModel_ = Model::Create();
	player_ = new Player();
	playerModel_ = Model::CreateFromOBJ("grassBlock", true);
	titleTextModel_ = Model::CreateFromOBJ("doubutuen", true);

	player_->Initialize(playerModel_, &camera_, {200, 0, 0});

	// スカイドームの生成
	skydome_ = new Skydome();
	// skydomeの初期化
	skydome_->Initialize();

	skydomeModel_ = Model::CreateFromOBJ("skydome", true);

	skyDomeTexture_ = TextureManager::Load("./Resources/skyDome.png");
	skyDomeTexture1_ = TextureManager::Load("./Resources/skyDome1.png");


	titleTex = TextureManager::Load("./Resources/doubutuen/doubutuen.png");

	titleTextTransform_.rotation_.x = std::numbers::pi_v<float> / 2.0f;
	titleTextTransform_.rotation_.y = std::numbers::pi_v<float> ;
	titleTextTransform_.Initialize();
	titleTextTransform_.scale_ = {7.0f, 7.0f, 7.0f};
	titleTextTransform_.translation_ = {180.0f, 5.0f, 0.0f};
	titleTextTransform_.matWorld_ = MakeAffineMatrix(titleTextTransform_.scale_, titleTextTransform_.rotation_, titleTextTransform_.translation_);
	titleTextTransform_.TransferMatrix();

	grassTex_ = TextureManager::Load("./Resources/grass/grass.png");
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Status::FadeIn, kFadeTime);

	cameraController_.SetCamera(&camera_);
	cameraController_.Reset();
	Rect area{};
	cameraController_.SetMovableArea(area);

	// ====== ★ここから追加（位置は一切いじらず、背景ステージだけ描画準備） ======
	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipCsv("./Resources/stage1.csv");

	// モデル・テクスチャ
	grassModel_ = Model::CreateFromOBJ("grassBlock", true); // 既存の草ブロック
	cubeModel_ = Model::CreateFromOBJ("cube", true);        // 赤青は cube.obj

	texGrass_ = TextureManager::Load("./Resources/grassBlock/grassBlock.png");
	texRed_ = TextureManager::Load("./Resources/cube/block_red.jpg");
	texBlue_ = TextureManager::Load("./Resources/cube/block_blue.jpg");

	GenerateBlocksForTitle_(); // WT を作るだけ。位置は MapChipField に従う

	// BGM 読み込み（拡張子に注意）
	titleBGMHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/TitleBGM.mp3");

	// 再生（ループあり、-1 で無限ループ、trueで重複再生回避）
	titleBGMPlayingId_ = Audio::GetInstance()->PlayWave(titleBGMHandle_, true);

	// 例: Resources/Audio/decide.wav を置いておく
	seDecideHandle_ = Audio::GetInstance()->LoadWave("./Resources/Audio/kettei.mp3");

	plane_ = Model::CreateFromOBJ("plane", true); // 1x1 の板メッシュ
	texHowto_ = TextureManager::Load("./Resources/howto_play.png");

	spaceModel_ = Model::CreateFromOBJ("Space", true);

	// ★ステージサムネ（仮で同じでもOK）
	stageTex_[0] = TextureManager::Load("./Resources/stage1.png");
	stageTex_[1] = TextureManager::Load("./Resources/stage2.png");
	stageTex_[2] = TextureManager::Load("./Resources/stage3.png");

	// ★WT初期化（位置は後でUpdateで演出させるので、ここではInitializeだけでもOK）
	for (int i = 0; i < 3; ++i) {
		stageWT_[i].Initialize();
	}
	stageCursor_ = 0;
	stageSelectT_ = 0.0f;
	stageSelectAppearing_ = false;
}

void TitleScene::GenerateBlocksForTitle_() {
	// 要素数をタイトル用にもつくる
	worldTransformBlocks_.clear();
	worldTransformBlocks_.resize(MapChipField::kNumBlockVirtical);
	for (auto& row : worldTransformBlocks_) {
		row.resize(MapChipField::kNumBlockHorizontal, nullptr);
	}

	// TitleScene.cpp の GenerateBlocksForTitle_()
	for (uint32_t y = 0; y < MapChipField::kNumBlockVirtical; ++y) {
		for (uint32_t x = 0; x < MapChipField::kNumBlockHorizontal; ++x) {
			MapChipType t = mapChipField_->GetMapChipTypeByIndex(x, y);
			if (t == MapChipType::kBlank)
				continue;

			auto* wt = new WorldTransform();
			wt->Initialize();

			// ここだけ変更：取得したタイル座標にオフセットを足す
			Vector3 base = mapChipField_->GetMapChipPositionByIndex(x, y);
			wt->translation_ = {base.x + stageOffset_.x, base.y + stageOffset_.y, base.z + stageOffset_.z};

			wt->scale_ = {1.0f, 1.0f, 1.0f};
			wt->rotation_ = {0.0f, 0.0f, 0.0f};
			wt->matWorld_ = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
			wt->TransferMatrix();

			worldTransformBlocks_[y][x] = wt;
		}
	}
}

void TitleScene::Update() {
	// ====== 既存どおり（位置は元コードのまま） ======
	WorldTransform& wt = player_->GetWorldTransform();
	wt.scale_ = {4.0f, 4.0f, 4.0f};
	wt.rotation_.y += 0.01f;
	wt.translation_.y = -10.0f;
	wt.matWorld_ = MakeAffineMatrix(wt.scale_, wt.rotation_, wt.translation_);
	wt.TransferMatrix();

	uiWT_.Initialize();
	uiWT_.translation_ = {200.0f, 0.0f, -35.0f};
	uiWT_.scale_ = {6.2f, 6.2f, 6.2f};                      // 画像サイズに相当
	uiWT_.rotation_ = {std::numbers::pi_v<float> / 2, std::numbers::pi_v<float> ,0}; // メッシュがXY平面なら調整
	uiWT_.matWorld_ = MakeAffineMatrix(uiWT_.scale_, uiWT_.rotation_, uiWT_.translation_);
	uiWT_.TransferMatrix();

	

	spaceWT_.rotation_.x = std::numbers::pi_v<float> / 2.0f;
	spaceWT_.rotation_.y = std::numbers::pi_v<float>;
	spaceWT_.Initialize();
	spaceWT_.scale_ = {3.0f, 3.0f, 3.0f};
	spaceWT_.translation_ = {200.0f, 0.0f, 0.0f};
	spaceWT_.matWorld_ = MakeAffineMatrix(spaceWT_.scale_, spaceWT_.rotation_, spaceWT_.translation_);
	spaceWT_.TransferMatrix();

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		if (Input::GetInstance()->PushKey(DIK_SPACE)) {
			if (!sePlayed_) {
				seDecideId_ = Audio::GetInstance()->PlayWave(seDecideHandle_, false); // ループなし
				// もし音量APIがあるなら例：
				// Audio::GetInstance()->SetVolume(seDecideId_, 0.8f);
				sePlayed_ = true;
			}
			phase_ = Phase::kHowToPlay;


		}
		break;

		case Phase::kStageSelect: {
		// ------------------------------
		// StageSelect：右→中→左 の順で置いていく配置（ぐちゃぐちゃ防止）
		// ★totalTime方式（stageSelectT_ は 0..totalTime の「秒」）
		// ------------------------------

		const float dt = 1.0f / 60.0f;

		// ------------------------------
		// 最終（並び）位置
		// ------------------------------
		const Vector3 base = {200.0f, 0.0f, -20.0f};
		const float spacing = 8.0f;
		const float scaleBase =3.0f;
		const float raiseSelected = 1.0f;
		const float pushFrontZ = 2.0f;

		// ------------------------------
		// 開始（置く前の“待機”位置）
		// ※右から出てくる感じ：右側＆奥
		// ------------------------------
		const Vector3 start = {base.x + 18.0f, base.y - 0.5f, base.z - 10.0f};

		// ------------------------------
		// 右→中→左 の順に出すためのディレイ
		// i=2(右) が最速、i=0(左) が最遅
		// ★ここを大きくしても止まらない
		// ------------------------------
		const float delayStep = 0.30f; // 1枚ごとの遅れ（秒）
		const float moveSpan = 0.30f;  // 1枚が動き切る時間（秒）

		// 右→中→左 で最後のカードが終わる総時間
		const float totalTime = delayStep * 2.0f + moveSpan;

		// 演出速度（1.0=実時間、2.0=2倍速）
		const float appearSpeed = 1.0f;

		// ------------------------------
		// 演出タイマー（0..totalTime）
		// ------------------------------
		if (stageSelectAppearing_) {
			stageSelectT_ += dt * appearSpeed;

			if (stageSelectT_ >= totalTime) {
				stageSelectT_ = totalTime;
				stageSelectAppearing_ = false;
			}
		}

		// ------------------------------
		// 補間関数
		// ------------------------------
		auto Smooth01 = [](float x) {
			x = std::clamp(x, 0.0f, 1.0f);
			return x * x * (3.0f - 2.0f * x); // smoothstep
		};

		for (int i = 0; i < 3; ++i) {

			// ------------------------------
			// このカード専用の 0..1 進行度（右→中→左）
			// ------------------------------
			const int order = (2 - i); // i=2→0, i=1→1, i=0→2
			const float delay = delayStep * order;

			// stageSelectT_ は「秒」なので、各カードで遅れて動き始める
			float local = (stageSelectT_ - delay) / moveSpan;
			float e = Smooth01(local);

			// ------------------------------
			// 最終位置（並び）
			// ------------------------------
			Vector3 target = {base.x + (i - 1) * spacing, base.y, base.z};

			// ------------------------------
			// 位置：start → target（カードごとに順番）
			// ------------------------------
			Vector3 pos;
			pos.x = start.x + (target.x - start.x) * e;
			pos.y = start.y + (target.y - start.y) * e;
			pos.z = start.z + (target.z - start.z) * e;

			// ------------------------------
			// 回転：最初は少し斜め → 揃う（置いてる感）
			// ------------------------------
			Vector3 rotStart = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float>, 0.35f};
			Vector3 rotEnd = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float>, 0.0f};

			Vector3 rot;
			rot.x = rotStart.x + (rotEnd.x - rotStart.x) * e;
			rot.y = rotStart.y + (rotEnd.y - rotStart.y) * e;
			rot.z = rotStart.z + (rotEnd.z - rotStart.z) * e;

			// ------------------------------
			// 選択中の強調：少し前＆少し上（演出完了後）
			// 左手座標系の運用に合わせて「手前=Zをマイナス寄り」
			// ------------------------------
			if (!stageSelectAppearing_ && i == stageCursor_) {
				pos.y += raiseSelected;
				pos.z -= pushFrontZ;
			}

			// ------------------------------
			// WT反映
			// ------------------------------
			stageWT_[i].scale_ = {scaleBase, scaleBase, scaleBase};
			stageWT_[i].rotation_ = rot;
			stageWT_[i].translation_ = pos;

			stageWT_[i].matWorld_ = MakeAffineMatrix(stageWT_[i].scale_, stageWT_[i].rotation_, stageWT_[i].translation_);
			stageWT_[i].TransferMatrix();
		}

		// ------------------------------
		// 操作（演出中は無効）
		// ※このブロックを for ループの直後に置く
		// ------------------------------
		if (!stageSelectAppearing_) {

			// 左
			if (Input::GetInstance()->TriggerKey(DIK_A) || Input::GetInstance()->TriggerKey(DIK_LEFT)) {
				stageCursor_ = (stageCursor_ + 2) % 3;
				Audio::GetInstance()->PlayWave(seDecideHandle_, false); // 任意：カーソル音
			}

			// 右
			if (Input::GetInstance()->TriggerKey(DIK_D) || Input::GetInstance()->TriggerKey(DIK_RIGHT)) {
				stageCursor_ = (stageCursor_ + 1) % 3;
				Audio::GetInstance()->PlayWave(seDecideHandle_, false); // 任意：カーソル音
			}

			// 決定
			if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
				selectedStage_ = stageCursor_;
				fade_->Start(Status::FadeOut, 1.0f);
				phase_ = Phase::kFadeOut;
			}
		}


	} break;


		case Phase::kHowToPlay:
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			seDecideId_ = Audio::GetInstance()->PlayWave(seDecideHandle_, false);

			phase_ = Phase::kStageSelect;
			stageSelectT_ = 0.0f;
			stageSelectAppearing_ = true;

			// SE重複防止フラグを使ってるならここで戻す（必要なら）
			// sePlayed_ = false;
		}

		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			Audio::GetInstance()->StopWave(titleBGMPlayingId_); 
			finished_ = true;
		}
		break;
	}

	// ★タイトル画面だけ、カメラを少し右へ
	camera_.translation_.x = 200.0f; // ← 右に寄せたい量（好みで調整）
	camera_.UpdateMatrix();
}

void TitleScene::Draw() {
	

	
	if (phase_ == Phase::kHowToPlay)
	{
		plane_->Draw(uiWT_, camera_, texHowto_);
		skydome_->Draw(skydomeModel_, camera_, skyDomeTexture1_);
	} else if (phase_ == Phase::kStageSelect || phase_ == Phase::kFadeOut) {
		for (int i = 0; i < 3; ++i) {
			plane_->Draw(stageWT_[i], camera_, stageTex_[i]);
		}
		skydome_->Draw(skydomeModel_, camera_, skyDomeTexture_);

		// ★背景ステージ（位置は MapChipField 準拠、カメラは Initialize のまま）
		for (uint32_t y = 0; y < worldTransformBlocks_.size(); ++y) {
			for (uint32_t x = 0; x < worldTransformBlocks_[y].size(); ++x) {
				WorldTransform* wt = worldTransformBlocks_[y][x];
				if (!wt)
					continue;

				MapChipType t = mapChipField_->GetMapChipTypeByIndex(x, y);

				Model* m = nullptr;
				uint32_t tex = 0;
				if (t == MapChipType::kBlock) { // 草
					m = grassModel_;
					tex = texGrass_;
				} else if (t == MapChipType::kBlockRed) { // 赤
					m = cubeModel_;
					tex = texRed_;
				} else if (t == MapChipType::kBlockBlue) { // 青
					m = cubeModel_;
					tex = texBlue_;
				}
				if (m) {
					m->Draw(*wt, camera_, tex);
				}
			}
		}
	} else
	{
		spaceModel_->Draw(spaceWT_, camera_);
		skydome_->Draw(skydomeModel_, camera_, skyDomeTexture_);
		// ★背景ステージ（位置は MapChipField 準拠、カメラは Initialize のまま）
		for (uint32_t y = 0; y < worldTransformBlocks_.size(); ++y) {
			for (uint32_t x = 0; x < worldTransformBlocks_[y].size(); ++x) {
				WorldTransform* wt = worldTransformBlocks_[y][x];
				if (!wt)
					continue;

				MapChipType t = mapChipField_->GetMapChipTypeByIndex(x, y);

				Model* m = nullptr;
				uint32_t tex = 0;
				if (t == MapChipType::kBlock) { // 草
					m = grassModel_;
					tex = texGrass_;
				} else if (t == MapChipType::kBlockRed) { // 赤
					m = cubeModel_;
					tex = texRed_;
				} else if (t == MapChipType::kBlockBlue) { // 青
					m = cubeModel_;
					tex = texBlue_;
				}
				if (m) {
					m->Draw(*wt, camera_, tex);
				}
			}
		}
		/*if (treeModel_) {
			treeModel_->Draw(treeTransform_, camera_);
		}
		if (grassModel1_) {
			grassModel1_->Draw(grassTransform_, camera_, grassTex_);
		}*/

		// 既存どおり
		if (titleTextModel_) {
			titleTextModel_->Draw(titleTextTransform_, camera_, titleTex);
		}
		if (playerModel_) {
			player_->Draw();
		}
		
	}
	if (phase_ == Phase::kFadeIn || phase_ == Phase::kFadeOut) {
		fade_->Draw();
	}

	
}

TitleScene::~TitleScene() {
	// 既存
	delete fade_;

	// ★追加：タイトル用に確保したものを解放
	for (auto& row : worldTransformBlocks_) {
		for (auto* wt : row) {
			delete wt;
		}
	}
	worldTransformBlocks_.clear();
	delete skydome_;
	delete mapChipField_;
	mapChipField_ = nullptr;
	// モデルはエンジン側の共有管理なら削除不要。個別所有なら↓
	// delete grassModel_; delete cubeModel_;
}
