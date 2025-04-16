#include <Windows.h>
#include <KamataEngine.h>
#include "GameScene.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	KamataEngine::Initialize(L"LE2B_21_マキノ_ハルト_AL3");

	using namespace KamataEngine;	

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();
	GameScene* gameScene = new GameScene();

	gameScene->Initialize();

	while (true) {
		if (KamataEngine::Update())
		{
			break;
		}

		imguiManager->Begin();

		gameScene->Update();

		imguiManager->End();

		dxCommon->PreDraw();

		gameScene->Draw();

		AxisIndicator::GetInstance()->Draw();

		imguiManager->Draw();

		dxCommon->PostDraw();
	}

	KamataEngine::Finalize();

	delete gameScene;
	gameScene = nullptr;

	return 0;
}
