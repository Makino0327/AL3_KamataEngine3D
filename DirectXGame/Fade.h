#pragma once
#define NOMINMAX
#include <Windows.h>
#include "KamataEngine.h"
#include <vector>
#include <cmath>
#include "Skydome.h"
#include "MapChipField.h"
#include "Player.h"
#include "Vector.h"
#include "CameraController.h"
#include "Enemy.h" 
#include "DeathParticles.h"
#include <algorithm>

enum class Status {
	None,
	FadeIn,
	FadeOut,
};
class Fade
{
public:
	KamataEngine::Sprite* sprite_ = nullptr;
	Status status_ = Status::None;

	float duration_ = 0.0f;
	float counter_ = 0.0f;

public:
	void Initialize();
	void Update();
	void Draw();

	void Start(Status status, float duration);

	void Stop();

	bool IsFinished() const;
};
