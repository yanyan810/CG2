#include "Object3d.h"
#include "Object3dCommon.h"


//Vector3 Normalize(const Vector3& v) {
//	float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
//	if (length == 0.0f) return { 0.0f, 0.0f, 0.0f };
//	return { v.x / length, v.y / length, v.z / length };
//}

static void ApplyAnimation(Model::Skeleton& skeleton, const Animation& animation, float time) {
	for (auto& joint : skeleton.joints) {

		auto it = animation.nodeAnimations.find(joint.name);
		if (it == animation.nodeAnimations.end()) {
			continue;
		}

		const NodeAnimation& na = it->second;

		// “そのジョイントの元値” から始める（カーブが無い成分は維持）
		Vector3 t = joint.transform.translate;
		Quaternion r = joint.transform.rotate;
		Vector3 s = joint.transform.scale;

		if (!na.translate.keyframes.empty()) t = CalculateValue(na.translate.keyframes, time);
		if (!na.rotate.keyframes.empty())    r = CalculateValue(na.rotate.keyframes, time);
		if (!na.scale.keyframes.empty())     s = CalculateValue(na.scale.keyframes, time);

		joint.transform.translate = t;
		joint.transform.rotate = r;
		joint.transform.scale = s;
	}
}

static uint32_t CalcTotalVertexCount(const Model::ModelData& modelData) {
	uint64_t total = 0;
	for (const auto& m : modelData.meshes) {
		total += m.vertices.size();
	}
	return static_cast<uint32_t>(total);
}

void Object3d::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dx) {
	Initialize(object3dCommon, dx, nullptr,nullptr);
}


void Object3d::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dx, SrvManager* srv, SkinningCommon* skinCom) {
	// 初期化処理
	this->object3dCommon = object3dCommon;

	dx_ = dx;

	srvManager_ = srv;

	skinningCommon_ = skinCom;

	transformationMatrixResourceModel= dx->CreateBufferResource(sizeof(TransformationMatrix));
	//書き込むためのアドレスを取得
	transformationMatrixResourceModel->Map(0, nullptr,
		reinterpret_cast<void**>(&transformationMatrixDataModel));
	//単位行列を書き込んでおく
	transformationMatrixDataModel->WVP = Matrix4x4::MakeIdentity4x4();
	transformationMatrixDataModel->World = Matrix4x4::MakeIdentity4x4();

	//平行光源
	directionalLightResource = dx->CreateBufferResource(sizeof(DirectionalLight));
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//初期化
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // ライトの色
	//directionalLightData->direction = Matrix4x4::Normalize(Vector3({ 0.0f, -1.0f, 0.0f }));//ライトの向き
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 0.0f; // ライトの強度


	//Transform変数
	transform = { {1.0f,1.0f,1.0f},
				  {0.0f,0.0f,0.0f},
				  {0.0f,0.0f,0.0f} };
	cameraTransform = { {1.0f,1.0f,1.0f},
						{0.3f,0.0f,0.0f},
						{0.0f,4.0f,-10.0f} };

	this->camera_ = object3dCommon->GetDefaultCamera();

	cameraResource_ = dx_->CreateBufferResource(sizeof(CameraGPU));
	cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));

	pointLightResource_ = dx_->CreateBufferResource(sizeof(PointLight));
	pointLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&pointLightData_));

	// 初期値（とりあえず）
	pointLightData_->color = { 1,1,1,1 };
	pointLightData_->position = { 0, 3, 0 };
	pointLightData_->intensity = 1.0f;
	pointLightData_->radius = 10.0f;
	pointLightData_->decay = 1.0f;

	//スポットライト
	spotLightResource_ = dx_->CreateBufferResource(sizeof(SpotLight));
	spotLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&spotLightData_));

	// スポットライト 初期値
	spotLightData_->color = { 1,1,1,1 };
	spotLightData_->position = { 0, 3, 0 };
	spotLightData_->intensity = 1.0f;
	spotLightData_->direction = { 0, -1, 0 }; // ※正規化しておくのが理想
	spotLightData_->distance = 10.0f;
	spotLightData_->decay = 1.0f;

	// 例：内側30度のcos（ラジアンにしてcos）
	spotLightData_->cosAngle = std::cosf(30.0f * (3.14159265f / 180.0f));

}

void Object3d::Update(float dt){

	// 1) 通常のWorld（Object3dのTransform）
	Matrix4x4 worldMatrixModel = Matrix4x4::MakeAffineMatrix(
		transform.scale, transform.rotate, transform.translate);

	// ===== アニメ再生（SkinnedならSkeletonへ適用）=====
	if (model_ && isPlayAnimation_) {

		const auto& anims = model_->GetAnimations();
		if (!anims.empty()) {

			// 使うアニメ選択（今のコードのまま）
			const Animation* anim = nullptr;
			if (!playingAnimName_.empty()) {
				auto itA = anims.find(playingAnimName_);
				if (itA != anims.end()) anim = &itA->second;
			}
			if (!anim) anim = &anims.begin()->second;

			// time更新（今のコードのまま）
			animationTime_ += dt;
			const float duration = std::max(anim->duration, 0.0001f);
			if (loop_) animationTime_ = std::fmod(animationTime_, duration);
			else       animationTime_ = std::min(animationTime_, duration);

			if (model_->HasSkinning()) {
				// ★Skinned：Skeletonに適用（worldに掛けない）
				if (!poseReady_) {
					poseSkeleton_ = model_->GetSkeleton();
					poseReady_ = true;
				}

				ApplyAnimation(poseSkeleton_, *anim, animationTime_);

				// skeletonSpace 更新（あなたの Multiply は m1*m2 なのでこの式でOK）
				Model::UpdateSkeleton(poseSkeleton_); // ← Modelに静的関数でもOK

				// ★ここで SkinCluster（palette）更新
				if (poseReady_) {
					UpdateSkinCluster_();
				}


			}
			else {
				// ★Rigid：今まで通り world に前掛け（必要なら root ノードだけに絞る）
				// 既存の playingNodeName_ 方式を残すならここに置く
			}
		}
	}

	// 2) ★glTF/FBX/OBJ共通：ModelのRootNode行列を適用（あれば）
	//    これで「glTFが回転してる/スケールが違う」みたいなのが補正される
	if (model_) {
		// Model に GetRootLocalMatrix() を追加した前提
		if (!model_->HasSkinning()) {
			const Matrix4x4& root = model_->GetRootLocalMatrix();

			// wvp = world * vp なので、root を world の “前” に掛ける
			worldMatrixModel = Matrix4x4::Multiply(root, worldMatrixModel);
		}
	}

	// 3) camera
	if (!camera_) {
		camera_ = object3dCommon->GetDefaultCamera();
	}

	// ★ボーン点表示更新（Line/Sphere無し版）
	if (debugDrawBones_ && model_ && model_->HasSkinning() && poseReady_) {

		for (size_t i = 0; i < poseSkeleton_.joints.size() && i < boneMarkers_.size(); ++i) {

			const auto& j = poseSkeleton_.joints[i];

			// SkeletonSpace -> World
			Matrix4x4 jointWorld =
				Matrix4x4::Multiply(j.skeletonSpaceMatrix, worldMatrixModel);

			// 行ベクトル系：平行移動は m[3][0..2]
			Vector3 pos{
				jointWorld.m[3][0],
				jointWorld.m[3][1],
				jointWorld.m[3][2]
			};

			boneMarkers_[i]->SetTranslate(pos);
			boneMarkers_[i]->Update(0.0f);
		}
	}

	Matrix4x4 wvpModel = worldMatrixModel;
	if (camera_) {
		const Matrix4x4& vp = camera_->GetViewProjectionMatrix(); // View*Proj
		wvpModel = Matrix4x4::Multiply(worldMatrixModel, vp);     // World * VP
	}

	transformationMatrixDataModel->WVP = wvpModel;
	transformationMatrixDataModel->World = worldMatrixModel;

	// 4) WorldInverseTranspose
	Matrix4x4 invW = Matrix4x4::Inverse(worldMatrixModel);
	transformationMatrixDataModel->WorldInverseTranspose = Matrix4x4::Transpose(invW);
}



void Object3d::Draw()
{
	if (!model_) { return; }

	if (cameraData_ && camera_) {
		cameraData_->worldPosition = camera_->GetTranslate();
	}

	auto* cmd = dx_->GetCommandList();

	// SRV heap
	ID3D12DescriptorHeap* heaps[] = {
		TextureManager::GetInstance()->GetSrvDescriptorHeap()
	};
	cmd->SetDescriptorHeaps(_countof(heaps), heaps);

	if (model_->HasSkinning()) {

		// ★スキン用 PSO/RootSig
		skinningCommon_->SetGraphicsPipelineState();

		// Transform (Root 1)
		cmd->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceModel->GetGPUVirtualAddress());

		// Lights (Root 4..7)
		cmd->SetGraphicsRootConstantBufferView(4, directionalLightResource->GetGPUVirtualAddress());
		cmd->SetGraphicsRootConstantBufferView(5, cameraResource_->GetGPUVirtualAddress());
		cmd->SetGraphicsRootConstantBufferView(6, pointLightResource_->GetGPUVirtualAddress());
		cmd->SetGraphicsRootConstantBufferView(7, spotLightResource_->GetGPUVirtualAddress());

		// Palette SRV (Root 2, VS t0)
		cmd->SetGraphicsRootDescriptorTable(2, skinCluster_.paletteSrvHandle.second);

		// Draw skinned (textureは Root 3 に入れる)
		model_->DrawSkinned(cmd, skinCluster_);
	}
	else {
		// ★通常モデル用 PSO/RootSig（あなたの Object3dCommon 側で設定してる想定）
		object3dCommon->SetGraphicsPipelineState();

		cmd->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceModel->GetGPUVirtualAddress());
		cmd->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
		cmd->SetGraphicsRootConstantBufferView(4, cameraResource_->GetGPUVirtualAddress());
		cmd->SetGraphicsRootConstantBufferView(5, pointLightResource_->GetGPUVirtualAddress());
		cmd->SetGraphicsRootConstantBufferView(6, spotLightResource_->GetGPUVirtualAddress());

		model_->Draw(cmd);
	}

	// debug bones
	if (debugDrawBones_ && !boneMarkers_.empty()) {
		for (auto& m : boneMarkers_) { m->Draw(); }
	}
}



void Object3d::SetModel(const std::string& filePath) {
	auto* mgr = ModelManager::GetInstance();

	Model* m = mgr->FindModel(filePath);
	if (!m) {
		mgr->LoadModel(filePath);
		m = mgr->FindModel(filePath);
	}
	model_ = m;

	boneMarkers_.clear();

	if (!model_) { return; }

	if (model_->HasSkinning()) {

		// ★先に保証（boneMarkers作成より前）
		assert(srvManager_ && "Object3d: srvManager_ is null. Pass it in Initialize()");

		// ==== SkinCluster 作成 ====
		ID3D12Device* device = dx_->GetDevice();
		ID3D12DescriptorHeap* srvHeap = TextureManager::GetInstance()->GetSrvDescriptorHeap();
		uint32_t descriptorSize =
			device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		skinCluster_ = CreateSkinCluster(
			device,
			model_->GetSkeleton(),
			model_->GetSkinClusterData(),
			model_->GetVertexCount(),
			srvHeap,
			descriptorSize
		);

		// ==== boneMarkers 作成（デバッグ用） ====
		const auto& skel = model_->GetSkeleton();
		boneMarkers_.reserve(skel.joints.size());

		for (size_t i = 0; i < skel.joints.size(); ++i) {
			auto marker = std::make_unique<Object3d>();
			marker->Initialize(object3dCommon, dx_, srvManager_,skinningCommon_);
			marker->SetModel(boneMarkerModel_);
			marker->SetScale({ 0.03f, 0.03f, 0.03f });
			marker->SetRotate({ 0,0,0 });
			marker->SetEnableLighting(0);
			boneMarkers_.push_back(std::move(marker));
		}
	}
}

void Object3d::SetDirection(const Vector3& direction)
{
	if (!directionalLightData) return;

	Vector3 d = direction;
	float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);

	if (len < 1e-6f) {
		d = { 0.0f, -1.0f, -1.0f }; // 0ベクトル保険
		len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
	}

	d.x /= len; d.y /= len; d.z /= len;
	directionalLightData->direction = d;
}

bool Object3d::HasAnimation() const {
	return model_ && !model_->GetAnimations().empty();
}

SkinCluster Object3d::CreateSkinCluster(
	ID3D12Device* device,
	const Model::Skeleton& skeleton,
	const std::map<std::string, Model::JointWeightData>& skinData,
	uint32_t vertexCount,
	ID3D12DescriptorHeap* srvHeap,
	uint32_t descriptorSize)
{
	SkinCluster sc{};

	// ========= palette (StructuredBuffer<WellForGPU>) =========
	sc.paletteResource = dx_->CreateBufferResource(sizeof(WellForGPU) * skeleton.joints.size());

	WellForGPU* mappedPalette = nullptr;
	sc.paletteResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedPalette));
	sc.mappedPalette = { mappedPalette, skeleton.joints.size() };

	// SRV index (TextureManagerのヒープと同じヒープに切る前提)
	assert(srvManager_ && "Object3d::CreateSkinCluster: srvManager_ is null");
	uint32_t srvIndex = srvManager_->Allocate();

	sc.paletteSrvHandle.first = dx_->GetCPUDescriptorHandle(srvHeap, descriptorSize, srvIndex);
	sc.paletteSrvHandle.second = dx_->GetGPUDescriptorHandle(srvHeap, descriptorSize, srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};
	paletteSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	paletteSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	paletteSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	paletteSrvDesc.Buffer.FirstElement = 0;
	paletteSrvDesc.Buffer.NumElements = static_cast<UINT>(skeleton.joints.size());
	paletteSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	paletteSrvDesc.Buffer.StructureByteStride = sizeof(WellForGPU);

	device->CreateShaderResourceView(sc.paletteResource.Get(), &paletteSrvDesc, sc.paletteSrvHandle.first);

	// ========= influence (VertexCount 分の VB) =========
	sc.influenceResource = dx_->CreateBufferResource(sizeof(VertexInfluence) * vertexCount);

	VertexInfluence* mappedInfluence = nullptr;
	sc.influenceResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedInfluence));
	std::memset(mappedInfluence, 0, sizeof(VertexInfluence) * vertexCount);
	sc.mappedInfluence = { mappedInfluence, vertexCount };

	sc.influenceBufferView.BufferLocation = sc.influenceResource->GetGPUVirtualAddress();
	sc.influenceBufferView.SizeInBytes = static_cast<UINT>(sizeof(VertexInfluence) * vertexCount);
	sc.influenceBufferView.StrideInBytes = sizeof(VertexInfluence);

	// ========= inverse bind pose =========
	sc.inverseBindPoseMatrices.resize(skeleton.joints.size());
	std::generate(sc.inverseBindPoseMatrices.begin(), sc.inverseBindPoseMatrices.end(), Matrix4x4::MakeIdentity4x4);

	// ========= fill influences =========
	for (const auto& jw : skinData) {

		auto it = skeleton.jointMap.find(jw.first);
		if (it == skeleton.jointMap.end()) { continue; }

		const int32_t jointIndex = it->second;

		// inverse bind
		sc.inverseBindPoseMatrices[jointIndex] = jw.second.inverseBindPoseMatrix;

		// weights
		for (const auto& vw : jw.second.vertexWeights) {

			// 範囲外防止（安全）
			if (vw.vertexIndex >= vertexCount) { continue; }

			auto& inf = sc.mappedInfluence[vw.vertexIndex];

			for (uint32_t k = 0; k < kNumMaxInfluence; ++k) {
				if (inf.weights[k] == 0.0f) {
					inf.weights[k] = vw.weight;
					inf.jointIndices[k] = jointIndex;
					break;
				}
			}
		}
	}


	OutputDebugStringA(("[Skin] vertexCount=" + std::to_string(vertexCount) + "\n").c_str());
	for (const auto& jw : skinData) {
		for (const auto& vw : jw.second.vertexWeights) {
			if (vw.vertexIndex >= vertexCount) {
				OutputDebugStringA("[Skin] ERROR: weight vertexIndex out of range!\n");
				break;
			}
		}
	}

	// --- debug: max vertex index in skinData ---
	uint32_t maxV = 0;
	uint64_t totalWeights = 0;

	for (const auto& jw : skinData) {
		for (const auto& vw : jw.second.vertexWeights) {
			maxV = (std::max)(maxV, vw.vertexIndex);
			totalWeights++;
		}
	}

	char buf[256];
	std::snprintf(buf, sizeof(buf),
		"[SkinDBG] vertexCount=%u maxVertexIndex=%u totalWeights=%llu\n",
		vertexCount, maxV, (unsigned long long)totalWeights);
	OutputDebugStringA(buf);


	return sc;
}


void Object3d::UpdateSkinCluster_()
{
	// 前提：poseSkeleton_ は UpdateSkeleton 済み
	// 前提：skinCluster_ は CreateSkinCluster 済み（mappedPalette / inverseBindPoseMatrices が揃っている）

	if (!model_ || !model_->HasSkinning()) { return; }

	const auto jointCount = poseSkeleton_.joints.size();
	if (skinCluster_.mappedPalette.size() != jointCount) { return; }
	if (skinCluster_.inverseBindPoseMatrices.size() != jointCount) { return; }

	for (size_t jointIndex = 0; jointIndex < jointCount; ++jointIndex) {

		// T_i = B_i^{-1} * S_i
		// ※あなたの Multiply が「m1*m2」なのでこの順でOK（スライドと一致）
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix =
			Matrix4x4::Multiply(
				skinCluster_.inverseBindPoseMatrices[jointIndex],
				poseSkeleton_.joints[jointIndex].skeletonSpaceMatrix
			);

		// 法線用：InverseTranspose
		Matrix4x4 inv =
			Matrix4x4::Inverse(skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix);

		skinCluster_.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			Matrix4x4::Transpose(inv);
	}
}
