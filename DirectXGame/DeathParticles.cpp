#include "DeathParticles.h"
#include <cassert>
#include "Player.h"

void DeathParticles::Initialize(Vector3 playerPosition, KamataEngine::Model* model, KamataEngine::Camera* camera) {
	model_ = model;
	camera_ = camera;

	counter_ = 0.0f;
	isFinished_ = false;
	objectColor_.Initialize(); // カラーバッファ初期化
	color_ = {1, 1, 1, 1};     // 白・不透明スタート


	for (uint32_t i = 0; i < kNumParticles; ++i) {
		worldTransforms_[i].Initialize();
		worldTransforms_[i].translation_ = playerPosition;
		worldTransforms_[i].scale_ = {2.0f, 2.0f, 2.0f};
		worldTransforms_[i].matWorld_ = MakeAffineMatrix(worldTransforms_[i].scale_, worldTransforms_[i].rotation_, worldTransforms_[i].translation_);
		worldTransforms_[i].TransferMatrix();

		lifeTimers_[i] = kDuration; // ここが超重要
	}
}



void DeathParticles::Update() {

	counter_ += 1.0f / 60.0f;
	if (counter_ >= kDuration) {
		counter_ = kDuration;
		isFinished_ = true;
	}

	// 終了フラグが立っていたら以降の処理をスキップ
	if (isFinished_) {
		return;
	}
	for (uint32_t i = 0; i < kNumParticles; ++i) {
		if (lifeTimers_[i] <= 0.0f)
			continue;

		lifeTimers_[i] -= 1.0f / 60.0f;

		// 基本となる右方向ベクトル
		Vector3 velocity = {kSpeed, 0.0f, 0.0f};

		// 回転角を求める
		float angle = kAngleUnit * i;

		// Z軸回りの回転行列を生成
		Matrix4x4 matrixRotation = MakeRotateZMatrix(angle);

		// 回転行列を使ってベクトルを回転させる
		velocity = Transform(velocity, matrixRotation);

		// アルファ値の更新（0.0〜1.0 の範囲に制限）
		float t = counter_ / kDuration;
		color_.w = std::clamp(1.0f - t, 0.0f, 1.0f);

		// 色変更オブジェクトに反映
		objectColor_.SetColor(color_);


		// 移動
		worldTransforms_[i].translation_.x += velocity.x;
		worldTransforms_[i].translation_.y += velocity.y;
		worldTransforms_[i].translation_.z += velocity.z;


		// 行列更新してからVRAM転送
		worldTransforms_[i].matWorld_ = MakeAffineMatrix(worldTransforms_[i].scale_, worldTransforms_[i].rotation_, worldTransforms_[i].translation_);
		worldTransforms_[i].TransferMatrix();
	}
}



void DeathParticles::Draw() {

	 if (isFinished_) {
		return;
	}

	for (size_t i = 0; i < worldTransforms_.size(); ++i) {
		WorldTransform& wt = worldTransforms_[i];
		model_->Draw(wt, *camera_, &objectColor_);

		// デバッグ出力（座標確認用）
		char buffer[256];
		sprintf_s(buffer, "DeathParticle[%zu] Pos: x=%.2f y=%.2f z=%.2f\n", i, wt.translation_.x, wt.translation_.y, wt.translation_.z);
		OutputDebugStringA(buffer);
	}
}
