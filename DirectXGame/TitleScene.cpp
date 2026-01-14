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
	mapChipField_->LoadMapChipCsv("./Resources/blocks.csv");

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

		case Phase::kHowToPlay:
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			    seDecideId_ = Audio::GetInstance()->PlayWave(seDecideHandle_, false); 
			fade_->Start(Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
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
	

	
	if (phase_ == Phase::kHowToPlay || phase_ == Phase::kFadeOut)
	{
		plane_->Draw(uiWT_, camera_, texHowto_);
		skydome_->Draw(skydomeModel_, camera_, skyDomeTexture1_);
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
