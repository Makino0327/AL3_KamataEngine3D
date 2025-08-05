#include "TitleScene.h"


void TitleScene::Initialize() {
	// フェード時間（秒単位）を定数として定義
	const float kFadeTime = 1.0f;
	


	camera_.Initialize();
	// タイトルテクスチャの読み込み（または描画文字）
	playerModel_ = Model::Create();
	titleTextModel_ = Model::Create();
	player_ = new Player();
	playerModel_ = Model::CreateFromOBJ("cat", true);
	
	titleTextModel_ = Model::CreateFromOBJ("doubutuen", true); // Resources/どうぶつえん/どうぶつえん.obj がある前提

	player_->Initialize(playerModel_, &camera_, {0,0,0}); 

	titleTextTransform_.rotation_.x = std::numbers::pi_v<float> /2.0f;

	// Transformの初期化
	titleTextTransform_.Initialize();
	titleTextTransform_.scale_ = {6.0f, 6.0f, 6.0f};       // 文字が小さいなら拡大
	titleTextTransform_.translation_ = {-18.0f, 5.0f, 0.0f}; // 表示位置調整（Yを少し上）

	titleTextTransform_.matWorld_ = MakeAffineMatrix(titleTextTransform_.scale_, titleTextTransform_.rotation_, titleTextTransform_.translation_);
	titleTextTransform_.TransferMatrix();

	fade_ = new Fade();
	fade_->Initialize();

		// フェードインを開始
	fade_->Start(Status::FadeIn, kFadeTime);

	cameraController_.SetCamera(&camera_);
	
	cameraController_.Reset();
	Rect area{};

	cameraController_.SetMovableArea(area);
}

void TitleScene::Update() {
	WorldTransform& wt = player_->GetWorldTransform();
	wt.scale_ = {4.0f, 4.0f, 4.0f};
	wt.rotation_.y += 0.01f;
	wt.translation_.y = -10.0f;
	wt.matWorld_ = MakeAffineMatrix(wt.scale_, wt.rotation_, wt.translation_);
	wt.TransferMatrix();
	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		

		if (Input::GetInstance()->PushKey(DIK_SPACE)) {
			fade_->Start(Status::FadeOut, 1.0f);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}
}



void TitleScene::Draw() {
	if (titleTextModel_) {
		titleTextModel_->Draw(titleTextTransform_, camera_);
	}

	if (playerModel_) {
		player_->Draw();
	}

	if (phase_ == Phase::kFadeIn || phase_ == Phase::kFadeOut) {
		fade_->Draw();
	}
}


TitleScene::~TitleScene()
{ delete fade_; }