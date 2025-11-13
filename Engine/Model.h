#pragma once
#include "ModelCommon.h"
#include "Vector3.h"
#include "Matrix4x4.h"
#include "TextureManager.h"
#include <format>
#include <filesystem>
#include <fstream>


class Model
{

public:

	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	struct MaterialData {
		std::string textureFilePath; // テクスチャファイルのパス
		uint32_t textureIndex = 0;
	};

	struct ModelData {
		std::vector<VertexData> vertices; // 頂点データ
		MaterialData material;
	};

	struct Material {
		Vector4 color; // 色
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

public:

	void Initialize(ModelCommon* modelCommon,
		const std::string& directoryPath,
		const std::string& filename);

	void Draw(ID3D12GraphicsCommandList* cmd);

	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);


private:

	ModelCommon* modelCommon_;

	ModelData modelData_;

	// 頂点データ（バッファ）
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;   // 頂点リソース
	VertexData* vertexData_ = nullptr;              // 頂点データのCPU側ポインタ
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};             // 頂点バッファビュー

	//マテリアル用のリソースを作る。今回はcolor1つ分のサイズを用意する
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	//マテリアルにデータを書き込む
	Material* materialData_ = nullptr;

};

