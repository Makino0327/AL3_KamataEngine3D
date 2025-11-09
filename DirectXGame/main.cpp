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

Scene currentScene = Scene::kGame;
Scene scene = Scene::kUnknown;

void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene && titleScene->IsFinished()) {
			// もうTitle使わないなら、この分岐自体を消してOK
			// （使い続けるならここでGameScene遷移を書く）
			scene = Scene::kGame;
			delete titleScene;
			titleScene = nullptr;
			gameScene = new GameScene();
			gameScene->Initialize();
		}
		break;

	case Scene::kGame:
		if (gameScene && gameScene->IsFinished()) {
			// ★Titleに戻さず、GameSceneを作り直してリスタート
			delete gameScene;
			gameScene = nullptr;
			gameScene = new GameScene();
			gameScene->Initialize();
			// scene はずっと Scene::kGame のまま
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



// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"LE2B_21_マキノ_ハルト_AL3");

	using namespace KamataEngine;	

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	scene = Scene::kGame;
	gameScene = new GameScene();
	gameScene->Initialize();

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

	delete titleScene;
	titleScene = nullptr;
	delete gameScene;
	gameScene = nullptr;



	return 0;
}
