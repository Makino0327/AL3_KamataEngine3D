#include <Windows.h>
#include <KamataEngine.h>
#include "GameScene.h"
#include "TitleScene.h"

GameScene* gameScene = nullptr;
TitleScene* titleScene = nullptr;

enum class Scene {
	kUnknown = 0,
	kTitle,
	kGame,
};

Scene currentScene = Scene::kTitle;
Scene scene = Scene::kUnknown;

void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene->IsFinished()) {
			// シーン変更
			scene = Scene::kGame;

			// 旧シーンの解放
			delete titleScene;
			titleScene = nullptr;

			// 新シーンの生成と初期化
			gameScene = new GameScene();
			gameScene->Initialize();
		}
		break;

	case Scene::kGame:
		if (gameScene->IsFinished()) {
			// シーン変更
			scene = Scene::kTitle;

			// 旧シーンの解放
			delete gameScene;
			gameScene = nullptr;

			// 新シーンの生成と初期化
			titleScene = new TitleScene();
			titleScene->Initialize();
		}
		break;

	}
}

void UpdateScene()
{
	switch (scene) {
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
		break;
	}
}

void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	}
}


// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"LE2B_21_マキノ_ハルト_AL3");

	using namespace KamataEngine;	

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	scene = Scene::kTitle;
	titleScene = new TitleScene;
	titleScene->Initialize();

	while (true) {
		if (KamataEngine::Update()){
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

	delete gameScene;


	return 0;
}
