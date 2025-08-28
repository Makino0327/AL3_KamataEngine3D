#include "MapChipField.h"
#include <cassert>
#include <fstream>
#include <map>
#include <sstream>

namespace {
std::unordered_map<std::string, MapChipType> mapChipTable = {
    {"0", MapChipType::kBlank},
    {"1", MapChipType::kBlock},
    {"2", MapChipType::kBlockRed }, // 追加
    {"3", MapChipType::kBlockBlue}, // 追加
};
}

// マップチップデータをリセット
void MapChipField::ResetMapChipData() 
{
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVirtical);
	for (std::vector<MapChipType>& mapChipDataLine : mapChipData_.data) {
		mapChipDataLine.resize(kNumBlockHorizontal);
	}
}

void MapChipField::LoadMapChipCsv(const std::string& filePath) {
	// マップチップデータをリセット
	ResetMapChipData();
	// ファイルを開く
	std::ifstream file;
	file.open(filePath);
	assert(file.is_open());

	// マップチップCSV
	std::stringstream mapChipCsv;
	// ファイルの内容を文字列ストリームにコピー
	mapChipCsv << file.rdbuf();
	// ファイルを閉じる
	file.close();

	// CSVからマップチップデータを読み込む
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		std::string line;
		std::getline(mapChipCsv, line);

		// 1行分の文字列をストリームに変換して解析しやすくなる
		std::istringstream line_stream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			std::string word;
			std::getline(line_stream, word, ',');

			if (mapChipTable.contains(word)) {
				mapChipData_.data[i][j] = mapChipTable[word];
			}
		}
	}
}

MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	if (xIndex >= kNumBlockHorizontal)
		return MapChipType::kBlank;
	if (yIndex >= kNumBlockVirtical)
		return MapChipType::kBlank;
	return mapChipData_.data[yIndex][xIndex];
}


KamataEngine::Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) {
	return KamataEngine::Vector3(kBlockWidth * xIndex, kBlockHeight * (kNumBlockVirtical - 1 - yIndex), 0);
}


IndexSet MapChipField::GetMapChipIndexByPosition(const Vector3& position) {
	Vector3 adjusted = {position.x + (kBlockWidth / 2.0f), position.y + (kBlockHeight / 2.0f), 0.0f};

	IndexSet indexSet;
	indexSet.xIndex = std::min(static_cast<uint32_t>(adjusted.x / kBlockWidth), kNumBlockHorizontal - 1);

	uint32_t reverseY = static_cast<uint32_t>(adjusted.y / kBlockHeight);
	indexSet.yIndex = std::min(kNumBlockVirtical - 1 - reverseY, kNumBlockVirtical - 1);

	return indexSet;
}


Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) {
	Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);

	Rect rect;
	rect.left = center.x - kBlockWidth / 2.0f;
	rect.right = center.x + kBlockWidth / 2.0f;
	rect.bottom = center.y - kBlockHeight / 2.0f;
	rect.top = center.y + kBlockHeight / 2.0f;

	return rect;
}

