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


void Object3d::Initialize(Object3dCommon* object3dCommon, DirectXCommon* dx) {
	// 初期化処理
	this->object3dCommon = object3dCommon;

	dx_ = dx;

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



void Object3d::Draw() {

	if (cameraData_ && camera_) {
		cameraData_->worldPosition = camera_->GetTranslate();
	}

	auto* cmd = dx_->GetCommandList();

	// ★ SRVヒープを必ずセット（これがないとテクスチャが読めない）
	ID3D12DescriptorHeap* heaps[] = {
		TextureManager::GetInstance()->GetSrvDescriptorHeap()
	};
	cmd->SetDescriptorHeaps(_countof(heaps), heaps);

	// Transform（WVP/World）
	cmd->SetGraphicsRootConstantBufferView(
		1, transformationMatrixResourceModel->GetGPUVirtualAddress());

	// DirectionalLight
	cmd->SetGraphicsRootConstantBufferView(
		3, directionalLightResource->GetGPUVirtualAddress());

	cmd->SetGraphicsRootConstantBufferView(
		4, cameraResource_->GetGPUVirtualAddress());

	cmd->SetGraphicsRootConstantBufferView(5, pointLightResource_->GetGPUVirtualAddress());

	cmd->SetGraphicsRootConstantBufferView(6, spotLightResource_->GetGPUVirtualAddress());

	// Model
	if (model_) {
		model_->Draw(cmd);
	}

	if (debugDrawBones_ && !boneMarkers_.empty()) {
		for (auto& m : boneMarkers_) {
			m->Draw();
		}
	}
}



void Object3d::SetModel(const std::string& filePath) {
	//モデルを検索してセットする
	auto* mgr = ModelManager::GetInstance();

	// まず探す
	Model* m = mgr->FindModel(filePath);

	// なければロードして再取得
	if (!m) {
		mgr->LoadModel(filePath);
		m = mgr->FindModel(filePath);
	}
	
	model_ = m;

	boneMarkers_.clear();

	if (model_ && model_->HasSkinning()) {
		const auto& skel = model_->GetSkeleton();

		boneMarkers_.reserve(skel.joints.size());
		for (size_t i = 0; i < skel.joints.size(); ++i) {

			auto marker = std::make_unique<Object3d>();
			marker->Initialize(object3dCommon, dx_);
			marker->SetModel(boneMarkerModel_);

			marker->SetScale({ 0.03f, 0.03f, 0.03f }); // 小さく
			marker->SetRotate({ 0,0,0 });

			// ライトいらないなら切る（見やすい）
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

