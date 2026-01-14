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

			delete titleScene;
			titleScene = nullptr;

			gameScene = new GameScene();
			gameScene->Initialize();
		}
		break;

	case Scene::kGame:
		if (gameScene && gameScene->IsFinished()) {

			if (gameScene->GetNextScene() == NextScene::kTitle) {

				delete gameScene;
				gameScene = nullptr;

				scene = Scene::kTitle;
				titleScene = new TitleScene();
				titleScene->Initialize();

			} else {
				delete gameScene;
				gameScene = nullptr;

				scene = Scene::kGame;
				gameScene = new GameScene();
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
