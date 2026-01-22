#pragma once
#include "ModelCommon.h"
#include "MathStruct.h"
#include "TextureManager.h"
#include <format>
#include <filesystem>
#include <fstream>
#include "Animation.h"
#include <unordered_map>
#include <algorithm>
#include <map>      // std::map
#include "SkinningTypes.h"

class Model
{

public:

	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};



	static_assert(offsetof(Model::VertexData, position) == 0);
	static_assert(offsetof(Model::VertexData, texcoord) == 16);
	static_assert(offsetof(Model::VertexData, normal) == 24);

	struct MaterialData { std::string textureFilePath; };

	struct Joint {
		QuaternionTransform transform;   // bind pose
		Matrix4x4 localMatrix;
		Matrix4x4 skeletonSpaceMatrix;
		std::string name;

		std::vector<int32_t> children;
		int32_t index;//自身のインデックス
		std::optional<int32_t> parent;
	};

	struct Skeleton {
		int32_t root;
		std::unordered_map<std::string, int32_t> jointMap;
		std::vector<Joint> joints;
	};

	//ノード
	struct Node {

		QuaternionTransform transform;
		Matrix4x4 localMatrix = Matrix4x4::MakeIdentity4x4();
		std::string name;
		std::vector<uint32_t> meshIndices; // aiNode::mMeshes を保持（scene->mMeshes の index）
		std::vector<Node> children;
	};

	struct MeshData {
		std::vector<VertexData> vertices; // 読み込み直後のデータ（CPU側）
		uint32_t materialIndex = 0;

		// GPUバッファに詰めた後の範囲
		uint32_t startVertex = 0;
		uint32_t vertexCount = 0;

		// ★追加：IndexBuffer 内での範囲
		uint32_t startIndex = 0;
		uint32_t indexCount = 0;
	};


	struct VertexWeightData {

		float weight;
		uint32_t vertexIndex;

	};

	struct JointWeightData {

		Matrix4x4 inverseBindPoseMatrix;
		std::vector<VertexWeightData> vertexWeights;

	};


	struct ModelData {
		std::map<std::string, JointWeightData> skinClusterData;
		std::vector<MaterialData> materials;
		std::vector<MeshData> meshes;

		Node rootNode;

		bool hasSkinning = false;

		std::vector<uint32_t> indices;

		// ★追加：アニメーション（Assimpから読めたもの）
		std::unordered_map<std::string, Animation> animations;
	};


	struct Material {
		Vector4  color;           // 16
		int32_t  enableLighting;  // 4
		float    pad0[3];         // 12  -> ここで16byte揃う

		Matrix4x4 uvTransform;    // 64

		float    shininess;       // 4
		float    pad1[3];         // 12  -> ここで16byte揃う
	};
	static_assert(sizeof(Material) % 16 == 0, "Material must be 16-byte aligned");

public:

	void Initialize(ModelCommon* modelCommon,
		const std::string& directoryPath,
		const std::string& filename);

	void Draw(ID3D12GraphicsCommandList* cmd);
	//パーティクル用
	void Draw(ID3D12GraphicsCommandList* cmd, uint32_t instanceCount);

	void DrawSkinned(ID3D12GraphicsCommandList* cmd, const SkinCluster& sc);


	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

	static ModelData    LoadAssimpFile(const std::string& fullPath);

	Vector4& GetMaterialColor()
	{
		static Vector4 dummy{ 1,1,1,1 };
		if (!materialData_) {
			OutputDebugStringA("[Model] GetMaterialColor returns dummy (materialData_ is null)\n");
			return dummy;
		}
		return materialData_->color;
	}

	void SetMaterialColor(const Vector4& c)
	{
		if (!materialData_) {
			OutputDebugStringA("[Model] SetMaterialColor skipped (materialData_ is null)\n");
			return;
		}
		materialData_->color = c;
	}

	Material* GetMaterial() { return materialData_; }

	const Matrix4x4& GetRootLocalMatrix() const;

	//modeldataを取得
	const std::unordered_map<std::string, Animation>& GetAnimations() const {
		return modelData_.animations;
	}

	bool HasSkinning() const { return modelData_.hasSkinning; }

	const Skeleton& GetSkeleton() const { return skeleton_; }

	static void UpdateSkeleton(Skeleton& skeleton);

	// 頂点数（Influence配列サイズ用）
	uint32_t GetVertexCount() const {
		uint32_t total = 0;
		for (const auto& m : modelData_.meshes) { total += m.vertexCount; }
		// もし vertexCount をまだ埋めてないなら meshes[i].vertices.size() にする
		if (total == 0) {
			for (const auto& m : modelData_.meshes) { total += (uint32_t)m.vertices.size(); }
		}
		return total;
	}

	// skinClusterData（jointName -> weights/invBind）
	const std::map<std::string, JointWeightData>& GetSkinClusterData() const {
		return modelData_.skinClusterData;
	}

private:
	static Skeleton CreateSkeleton(const Node& rootNode);

	static int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);



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

	Skeleton skeleton_;

	// Indexデータ（バッファ）★追加
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	uint32_t* indexData_ = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};


};

