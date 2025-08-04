#pragma once  
#include "KamataEngine.h"  
#include "Vector.h"  
#include <array>  
#define _USE_MATH_DEFINES  
#include <cmath>  
#include <math.h> // 追加: M_PI を定義するために必要  

class Player; // 前方宣言（インクルードループ対策）  

static inline const uint32_t kNumParticles = 8;  
static inline const float kDuration = 1.0f; // パーティクルの寿命（秒）  
static inline const float kSpeed = 0.3f;    // 移動速度  
static inline const float kAngleUnit = 2.0f * float(M_PI) / float(kNumParticles);  

class DeathParticles  
{  
private:  
   KamataEngine::Model* model_ = nullptr;  
   KamataEngine::Camera* camera_ = nullptr;  
   std::array<KamataEngine::WorldTransform, kNumParticles> worldTransforms_;  
   std::array<float, kNumParticles> lifeTimers_;  
   // 終了フラグ（描画しないかの判定に使う）
   bool isFinished_ = false;

   // 経過時間カウンタ（秒）
   float counter_ = 0.0f;

   ObjectColor objectColor_; // 色変更用（KamataEngine特有）
   Vector4 color_;           // 実際の色データ（RGBA）


public:  
   // 初期化（Playerの位置を基準に）  
   void Initialize(Vector3 playerPosition, KamataEngine::Model* model, KamataEngine::Camera* camera);  

   // 更新  
   void Update();  

   // 描画  
   void Draw();  
};
