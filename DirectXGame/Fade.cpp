#include "Fade.h"

void Fade::Initialize() {
	// スプライト生成（テクスチャID = 0 の透明テクスチャを使う場合など）
	// フェード用に透明テクスチャを使って画面全体に広がる黒スプライトを作る場合
	sprite_ = Sprite::Create(
	    0,                             // テクスチャID（0 = 透明）
	    Vector2(0.0f, 0.0f),           // 表示位置（画面左上）
	    Vector4(0, 0, 1280.0f, 720.0f) // サイズ（画面全体）
	);

	// 画面サイズを指定（1280x720など、環境に合わせて）
	sprite_->SetSize(Vector2(1280.0f, 720.0f));

	// 黒色・完全不透明（RGBA）
	sprite_->SetColor(Vector4(0, 0, 0, 1));
}

void Fade::Update() {
	if (status_ == Status::None) {
		return;
	}

	// フレーム分の秒数を加算
	counter_ += 1.0f / 60.0f;
	if (counter_ > duration_) {
		counter_ = duration_;
	}

	// アルファ値をステータスごとに計算
	float alpha = 0.0f;
	switch (status_) {
	case Status::FadeIn:
		alpha = 1.0f - std::clamp(counter_ / duration_, 0.0f, 1.0f);
		break;
	case Status::FadeOut:
		alpha = std::clamp(counter_ / duration_, 0.0f, 1.0f);
		break;
	default:
		break;
	}

	// アルファ適用
	sprite_->SetColor(Vector4(0, 0, 0, alpha));
}


void Fade::Draw() {

	if (status_ == Status::None) {
		return; // フェードがない場合は描画しない
	}

	if (sprite_) {
		Sprite::PreDraw(KamataEngine::DirectXCommon::GetInstance()->GetCommandList()); // パイプライン切り替え
		sprite_->Draw();                                                               // スプライト描画
		Sprite::PostDraw();                                                            // パイプライン戻す
	}
}

void Fade::Start(Status status, float duration) {
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;
}

void Fade::Stop() {
	// フェードを停止する場合は、ステータスをNoneに設定
	status_ = Status::None;
	
}

bool Fade::IsFinished() const { 
	switch (status_) 
	{
	case Status::FadeIn:
	case Status::FadeOut:
		if (counter_ >= duration_) {
			return true; // フェードが完了
		} else
		{
			return false; // フェードがまだ進行中
		}
	}
	return true; // Noneの場合は完了とみなす
}