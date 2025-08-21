#pragma once
#define NOMINMAX
#include <Windows.h>
#include "KamataEngine.h"
#include "Vector.h"
#include <algorithm>

enum class MapChipType
{
	kBlank, // 空白
	kBlock, // ブロック
};

struct MapChipData
{
	std::vector<std::vector<MapChipType>> data;
};

struct IndexSet
{
	uint32_t xIndex;
	uint32_t yIndex;
};

// 範囲情報

class MapChipField {
public:
	// 1ブロックのサイズ
	static inline const float kBlockWidth = 2.0f;
	static inline const float kBlockHeight = 2.0f;
	// ブロックの個数
	static inline const uint32_t kNumBlockVirtical = 20;
	static inline const uint32_t kNumBlockHorizontal = 200;

	MapChipData mapChipData_;

public:
	void ResetMapChipData();
	void LoadMapChipCsv(const std::string& filePath);
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);
	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);
	IndexSet GetMapChipIndexByPosition(const Vector3& position);
	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);
};