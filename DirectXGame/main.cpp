#include "GameScene.h"
#include "TitleScene.h"
#include <KamataEngine.h>
#include <Windows.h>

GameScene* gameScene = nullptr;
TitleScene* titleScene = nullptr;

enum class Scene {
	kUnknown = 0,
	kTitle,
	kGame,
};

Scene scene = Scene::kTitle;

void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene && titleScene->IsFinished()) {
			scene = Scene::kGame;

			// ★修正手順1：削除する前に、必要な情報（ステージ番号）を一時保存する
			int nextStageIndex = titleScene->GetSelectedStage();

			// ★修正手順2：用済みになったので削除する
			delete titleScene;
			titleScene = nullptr;

			// ★修正手順3：新しいシーンを作って、保存しておいた情報を渡す
			gameScene = new GameScene();
			gameScene->SetStageIndex(nextStageIndex); // 変数から渡す
			gameScene->Initialize();
		}
		break;

	case Scene::kGame:
		if (gameScene && gameScene->IsFinished()) {

			// ★修正：削除前に現在のステージを退避
			int keepStageIndex = gameScene->GetStageIndex();
			NextScene next = gameScene->GetNextScene();

			delete gameScene;
			gameScene = nullptr;

			if (next == NextScene::kTitle) {
				scene = Scene::kTitle;
				titleScene = new TitleScene();
				titleScene->Initialize();
			} else {
				// ★Restart（またはそれ以外） → 同じステージで再生成
				scene = Scene::kGame;
				gameScene = new GameScene();
				gameScene->SetStageIndex(keepStageIndex); // ★ここが本命
				gameScene->Initialize();
			}
		}
		break;

	}
}

void UpdateScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene)
			titleScene->Update();
		break;
	case Scene::kGame:
		if (gameScene)
			gameScene->Update();
		break;
	}
}

void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene)
			titleScene->Draw();
		break;
	case Scene::kGame:
		if (gameScene)
			gameScene->Draw();
		break;
	}
}

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	KamataEngine::Initialize(L"LE2B_23_マキノ_ハルト_壁を打ち抜け");

	using namespace KamataEngine;

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// ✅ 最初はタイトルを生成
	scene = Scene::kTitle;
	titleScene = new TitleScene();
	titleScene->Initialize();

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		ChangeScene();
		UpdateScene();

		dxCommon->PreDraw();
		Model::PreDraw(dxCommon->GetCommandList());

		DrawScene();

		Model::PostDraw();
		dxCommon->PostDraw();
	}

	KamataEngine::Finalize();

	delete titleScene;
	titleScene = nullptr;
	delete gameScene;
	gameScene = nullptr;

	return 0;
}
