#include <Windows.h>
#include <KamataEngine.h>
#include "GameScene.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"LE2B_21_マキノ_ハルト_AL3");

	using namespace KamataEngine;	

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	GameScene* gameScene = new GameScene();

	while (true) {
		if (KamataEngine::Update()){
			break;
		}

		gameScene->Initialize();

		gameScene->Update();

		gameScene->Draw();

		dxCommon->PreDraw();

		dxCommon->PostDraw();
	}

	KamataEngine::Finalize();

	delete gameScene;
	gameScene = nullptr;

	return 0;
}
