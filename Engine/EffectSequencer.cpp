#include "EffectSequencer.h"
#include "Object3d.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Camera.h"
#include "ModelParticleManager.h"
#include "TrailManager.h"
#include "TrailInstance.h"
#include "ModelManager.h"
#include "GameApp.h"
#include <algorithm>
#include <unordered_map>

#ifdef USE_IMGUI
#include <imgui.h>
#endif

namespace {
	ProjectileProfile::Piece MakeLegacyProjectilePiece(const ProjectileProfile& projectile)
	{
		ProjectileProfile::Piece piece;
		piece.name = "Bullet 1";
		piece.modelPath = projectile.modelPath;
		piece.scale = projectile.scale;
		piece.rotationSpeed = projectile.rotationSpeed;
		return piece;
	}

	std::vector<ProjectileProfile::Piece> GetProjectilePieces(const ProjectileProfile& projectile)
	{
		return projectile.pieces.empty()
			? std::vector<ProjectileProfile::Piece>{ MakeLegacyProjectilePiece(projectile) }
			: projectile.pieces;
	}

	void ApplyProjectilePieceSettings(Object3d& object, const ProjectileProfile::Piece& piece)
	{
		object.SetScale(piece.scale);
		object.SetMaterialColor(piece.materialColor);
		object.SetEnableLighting(piece.enableLighting ? 2 : 0);
		object.SetDirection(piece.lightDir);
		object.SetLightColor(piece.lightColor);
		object.SetIntensity(piece.lightIntensity);
	}
}

nlohmann::json ProjectileProfile::Piece::ToJson() const
{
	return nlohmann::json{
		{"name", name},
		{"modelPath", modelPath},
		{"offset", {offset.x, offset.y, offset.z}},
		{"rot", {rot.x, rot.y, rot.z}},
		{"scale", {scale.x, scale.y, scale.z}},
		{"rotationSpeed", {rotationSpeed.x, rotationSpeed.y, rotationSpeed.z}},
		{"materialColor", {materialColor.x, materialColor.y, materialColor.z, materialColor.w}},
		{"enableLighting", enableLighting},
		{"lightDir", {lightDir.x, lightDir.y, lightDir.z}},
		{"lightColor", {lightColor.x, lightColor.y, lightColor.z, lightColor.w}},
		{"lightIntensity", lightIntensity},
		{"usePostEffect", usePostEffect},
		{"postEffect", {
			{"threshold", postEffect.threshold},
			{"intensity", postEffect.intensity},
			{"vignetteIntensity", postEffect.vignetteIntensity},
			{"vignetteScale", postEffect.vignetteScale},
			{"chromAbAmount", postEffect.chromAbAmount},
			{"distortionAmount", postEffect.distortionAmount},
			{"noiseIntensity", postEffect.noiseIntensity},
			{"scanlineIntensity", postEffect.scanlineIntensity},
			{"scanlineFrequency", postEffect.scanlineFrequency},
			{"glitchAmount", postEffect.glitchAmount},
			{"dissolveAmount", postEffect.dissolveAmount},
			{"curvature", postEffect.curvature},
			{"borderSharp", postEffect.borderSharp}
		}}
	};
}

void ProjectileProfile::Piece::FromJson(const nlohmann::json& j)
{
	name = j.value("name", name);
	modelPath = j.value("modelPath", modelPath);
	if (j.contains("offset")) { offset = { j["offset"][0], j["offset"][1], j["offset"][2] }; }
	if (j.contains("rot")) { rot = { j["rot"][0], j["rot"][1], j["rot"][2] }; }
	if (j.contains("scale")) { scale = { j["scale"][0], j["scale"][1], j["scale"][2] }; }
	if (j.contains("rotationSpeed")) {
		rotationSpeed = { j["rotationSpeed"][0], j["rotationSpeed"][1], j["rotationSpeed"][2] };
	}
	if (j.contains("materialColor")) {
		materialColor = { j["materialColor"][0], j["materialColor"][1], j["materialColor"][2], j["materialColor"][3] };
	}
	enableLighting = j.value("enableLighting", enableLighting);
	if (j.contains("lightDir")) { lightDir = { j["lightDir"][0], j["lightDir"][1], j["lightDir"][2] }; }
	if (j.contains("lightColor")) {
		lightColor = { j["lightColor"][0], j["lightColor"][1], j["lightColor"][2], j["lightColor"][3] };
	}
	lightIntensity = j.value("lightIntensity", lightIntensity);
	usePostEffect = j.value("usePostEffect", usePostEffect);
	if (j.contains("postEffect")) {
		const auto& pe = j["postEffect"];
		if (pe.contains("threshold")) postEffect.threshold = pe["threshold"];
		if (pe.contains("intensity")) postEffect.intensity = pe["intensity"];
		if (pe.contains("vignetteIntensity")) postEffect.vignetteIntensity = pe["vignetteIntensity"];
		if (pe.contains("vignetteScale")) postEffect.vignetteScale = pe["vignetteScale"];
		if (pe.contains("chromAbAmount")) postEffect.chromAbAmount = pe["chromAbAmount"];
		if (pe.contains("distortionAmount")) postEffect.distortionAmount = pe["distortionAmount"];
		if (pe.contains("noiseIntensity")) postEffect.noiseIntensity = pe["noiseIntensity"];
		if (pe.contains("scanlineIntensity")) postEffect.scanlineIntensity = pe["scanlineIntensity"];
		if (pe.contains("scanlineFrequency")) postEffect.scanlineFrequency = pe["scanlineFrequency"];
		if (pe.contains("glitchAmount")) postEffect.glitchAmount = pe["glitchAmount"];
		if (pe.contains("dissolveAmount")) postEffect.dissolveAmount = pe["dissolveAmount"];
		if (pe.contains("curvature")) postEffect.curvature = pe["curvature"];
		if (pe.contains("borderSharp")) postEffect.borderSharp = pe["borderSharp"];
	}
}

nlohmann::json ProjectileProfile::ToJson() const
{
	nlohmann::json j{
		{"modelPath", modelPath},
		{"scale", {scale.x, scale.y, scale.z}},
		{"rotationSpeed", {rotationSpeed.x, rotationSpeed.y, rotationSpeed.z}}
	};
	j["pieces"] = nlohmann::json::array();
	for (const auto& piece : pieces) {
		j["pieces"].push_back(piece.ToJson());
	}
	return j;
}

void ProjectileProfile::FromJson(const nlohmann::json& j)
{
	modelPath = j.value("modelPath", modelPath);
	if (j.contains("scale")) {
		scale = { j["scale"][0], j["scale"][1], j["scale"][2] };
	}
	if (j.contains("rotationSpeed")) {
		rotationSpeed = { j["rotationSpeed"][0], j["rotationSpeed"][1], j["rotationSpeed"][2] };
	}
	pieces.clear();
	if (j.contains("pieces") && j["pieces"].is_array()) {
		for (const auto& item : j["pieces"]) {
			Piece piece = MakeLegacyProjectilePiece(*this);
			piece.FromJson(item);
			pieces.push_back(piece);
		}
	}
}

// =============================================
// 繝倥Ν繝代・髢｢謨ｰ
// =============================================

Vector3 EffectSequencer::LerpVec3(const Vector3& a, const Vector3& b, float t) {
	return {
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t
	};
}

// =============================================
// 蛻晄悄蛹・
// =============================================

void EffectSequencer::Initialize(
	Object3dCommon* objCommon,
	DirectXCommon* dx,
	Camera* camera,
	ModelParticleManager* particleMgr,
	TrailManager* trailMgr)
{
	objCommon_ = objCommon;
	dx_ = dx;
	camera_ = camera;
	particleMgr_ = particleMgr;
	trailMgr_ = trailMgr;

	state_ = State::Idle;

	// 繧ｨ繝・ぅ繧ｿ繝ｼ逕ｨ縺ｮ蛻晄悄繝励Ο繝輔ぃ繧､繝ｫ繧偵そ繝・ヨ
	editingProfile_ = EffectProfile{};
}

// =============================================
// 逋ｺ蟆・
// =============================================

void EffectSequencer::Fire(const EffectProfile& profile, const Vector3& startPos, const Vector3& targetPos) {
	// 譌｢縺ｫ螳溯｡御ｸｭ縺ｪ繧牙・縺ｫ繝ｪ繧ｻ繝・ヨ
	if (state_ != State::Idle && state_ != State::Finished) {
		Reset();
	}

	profile_ = profile;
	startPos_ = startPos;
	targetPos_ = targetPos;
	currentPos_ = startPos;
	elapsedTime_ = 0.0f;
	projectileRotation_ = { 0.0f, 0.0f, 0.0f };

	state_ = State::Firing;
}

// =============================================
// 豈弱ヵ繝ｬ繝ｼ繝譖ｴ譁ｰ
// =============================================

void EffectSequencer::Update(float dt) {
	switch (state_) {
	case State::Idle:
	case State::Finished:
		// 菴輔ｂ縺励↑縺・
		break;
	case State::Firing:
		UpdateFiring(dt);
		break;
	case State::Flying:
		UpdateFlying(dt);
		break;
	case State::Hit:
		UpdateHit(dt);
		break;
	}
}

// =============================================
// 繧ｹ繝・・繝亥挨蜃ｦ逅・
// =============================================

void EffectSequencer::UpdateFiring(float dt) {
	// --- 蠑ｾ繧ｪ繝悶ず繧ｧ繧ｯ繝医・逕滓・ ---
	projectile_.reset();
	projectilePieces_.clear();
	projectilePieceRotations_.clear();
	activeProjectilePieces_ = GetProjectilePieces(profile_.projectile);
	projectilePieces_.reserve(activeProjectilePieces_.size());
	projectilePieceRotations_.reserve(activeProjectilePieces_.size());

	for (const auto& piece : activeProjectilePieces_) {
		auto object = std::make_unique<Object3d>();
		object->Initialize(objCommon_, dx_);
		object->SetModel(piece.modelPath);
		object->SetTranslate({
			startPos_.x + piece.offset.x,
			startPos_.y + piece.offset.y,
			startPos_.z + piece.offset.z
			});
		object->SetRotate(piece.rot);
		object->SetCamera(camera_);
		ApplyProjectilePieceSettings(*object, piece);
		object->Update(0.0f);
		projectilePieces_.push_back(std::move(object));
		projectilePieceRotations_.push_back(piece.rot);
	}

	// --- 霆瑚ｷ｡縺ｮ逕滓・ ---
	if (profile_.enableTrail && trailMgr_) {
		trail_ = trailMgr_->CreateInstance();
		if (trail_) {
			trail_->SetIsPermanent(false);
			trail_->SetActive(true);

			TrailConfig trailConfig;
			trailConfig.startColor = profile_.trail.startColor;
			trailConfig.endColor = profile_.trail.endColor;
			trailConfig.maxPoints = profile_.trail.maxPoints;
			trailConfig.interpolationSteps = profile_.trail.interpolationSteps;
			trailConfig.lifetime = profile_.trail.lifetime;
			trail_->SetConfig(trailConfig);
		}
	}

	// 蜊ｳ蠎ｧ縺ｫFlying縺ｸ驕ｷ遘ｻ
	state_ = State::Flying;

	// 1繝輔Ξ繝ｼ繝逶ｮ縺ｮUpdate繧ょｮ溯｡・
	UpdateFlying(dt);
}

void EffectSequencer::UpdateFlying(float dt) {
	elapsedTime_ += dt;

	// 豁｣隕丞喧譎る俣 t (0.0 ~ 1.0)
	float duration = (std::max)(profile_.duration, 0.001f);
	float t = elapsedTime_ / duration;
	t = (std::min)(t, 1.0f);

	// EaseInOut・・moothStep・峨↓繧医ｋ貊代ｉ縺九↑陬憺俣
	float eased = t * t * (3.0f - 2.0f * t);

	// 蠑ｾ縺ｮ菴咲ｽｮ繧呈峩譁ｰ
	currentPos_ = LerpVec3(startPos_, targetPos_, eased);

	// 蠑ｾ縺ｮ蝗櫁ｻ｢
	projectileRotation_.x += profile_.projectile.rotationSpeed.x * dt;
	projectileRotation_.y += profile_.projectile.rotationSpeed.y * dt;
	projectileRotation_.z += profile_.projectile.rotationSpeed.z * dt;

	// Object3d 縺ｮ譖ｴ譁ｰ
	for (size_t i = 0; i < projectilePieces_.size() && i < activeProjectilePieces_.size(); ++i) {
		auto& object = projectilePieces_[i];
		const auto& piece = activeProjectilePieces_[i];
		if (!object) {
			continue;
		}
		projectilePieceRotations_[i].x += piece.rotationSpeed.x * dt;
		projectilePieceRotations_[i].y += piece.rotationSpeed.y * dt;
		projectilePieceRotations_[i].z += piece.rotationSpeed.z * dt;
		object->SetTranslate({
			currentPos_.x + piece.offset.x,
			currentPos_.y + piece.offset.y,
			currentPos_.z + piece.offset.z
			});
		object->SetRotate(projectilePieceRotations_[i]);
		ApplyProjectilePieceSettings(*object, piece);
		object->Update(dt);
	}

	// 鬟帷ｿ斐ヱ繝ｼ繝・ぅ繧ｯ繝ｫ縺ｮ逋ｺ逕・
	if (particleMgr_ && !profile_.flyParticle.empty()) {
		particleMgr_->Emit(profile_.flyParticle, currentPos_, profile_.flyParticleCount);
	}

	// 霆瑚ｷ｡縺ｮ譖ｴ譁ｰ・医が繝輔そ繝・ヨ莉倥″・・
	if (trail_ && trail_->IsActive()) {
		Vector3 tipPos = {
			currentPos_.x + profile_.trail.tipOffset.x,
			currentPos_.y + profile_.trail.tipOffset.y,
			currentPos_.z + profile_.trail.tipOffset.z
		};
		Vector3 basePos = {
			currentPos_.x + profile_.trail.baseOffset.x,
			currentPos_.y + profile_.trail.baseOffset.y,
			currentPos_.z + profile_.trail.baseOffset.z
		};

		TrailConfig trailConfig;
		trailConfig.startColor = profile_.trail.startColor;
		trailConfig.endColor = profile_.trail.endColor;
		trailConfig.maxPoints = profile_.trail.maxPoints;
		trailConfig.interpolationSteps = profile_.trail.interpolationSteps;
		trailConfig.lifetime = profile_.trail.lifetime;
		trail_->Update(dt, tipPos, basePos, trailConfig);
	}

	// 繧ｿ繝ｼ繧ｲ繝・ヨ蛻ｰ驕泌愛螳・
	if (t >= 1.0f) {
		// 竊・Hit 繧ｹ繝・・繝医∈
		state_ = State::Hit;
		elapsedTime_ = 0.0f; // 繝偵ャ繝医ち繧､繝槭・繧偵Μ繧ｻ繝・ヨ

		// 蠑ｾ繧帝撼陦ｨ遉ｺ
		for (auto& object : projectilePieces_) {
			if (!object) {
				continue;
			}
			object->SetScale({ 0.0f, 0.0f, 0.0f });
			object->Update(0.0f);
		}

		// 霆瑚ｷ｡繧貞●豁｢・域ｶ亥喧繝｢繝ｼ繝会ｼ・
		if (trail_) {
			trail_->SetActive(false);
		}

		// 繝偵ャ繝医ヱ繝ｼ繝・ぅ繧ｯ繝ｫ縺ｮ逋ｺ逕・
		if (particleMgr_ && !profile_.hitParticle.empty()) {
			particleMgr_->Emit(profile_.hitParticle, targetPos_, profile_.hitParticleCount);
		}

		// 繧ｳ繝ｼ繝ｫ繝舌ャ繧ｯ蜻ｼ縺ｳ蜃ｺ縺・
		if (onHitCallback_) {
			onHitCallback_();
		}
	}
}

void EffectSequencer::UpdateHit(float dt) {
	elapsedTime_ += dt;

	// 霆瑚ｷ｡縺ｮ豸亥喧繧｢繝九Γ繝ｼ繧ｷ繝ｧ繝ｳ
	if (trail_) {
		// TrailInstance 縺ｯ SetActive(false) 蠕後↓閾ｪ蜍輔〒豸亥喧縺輔ｌ繧・
		// 霑ｽ蜉縺ｮ豸亥喧縺ｯ荳崎ｦ・ｼ・railManager::Update 縺悟・逅・☆繧具ｼ・
	}

	// 繝偵ャ繝域ｼ泌・邨ゆｺ・愛螳・
	if (elapsedTime_ >= profile_.hitDuration) {
		state_ = State::Finished;

		// 繧ｯ繝ｪ繝ｼ繝ｳ繧｢繝・・
		projectile_.reset();
		projectilePieces_.clear();
		projectilePieceRotations_.clear();
		activeProjectilePieces_.clear();
		trail_ = nullptr; // TrailManager縺梧園譛峨＠縺ｦ縺・ｋ縺ｮ縺ｧ隗｣謾ｾ縺ｯ縺励↑縺・
	}
}

// =============================================
// 謠冗判
// =============================================

void EffectSequencer::Draw() {
	if (state_ != State::Flying) {
		return;
	}
	for (size_t i = 0; i < projectilePieces_.size() && i < activeProjectilePieces_.size(); ++i) {
		if (!projectilePieces_[i] || activeProjectilePieces_[i].usePostEffect) {
			continue;
		}
		projectilePieces_[i]->Draw();
	}
}

void EffectSequencer::DrawPostEffect(GameApp& app) {
	if (state_ != State::Flying) {
		return;
	}
	for (size_t i = 0; i < projectilePieces_.size() && i < activeProjectilePieces_.size(); ++i) {
		if (!projectilePieces_[i] || !activeProjectilePieces_[i].usePostEffect) {
			continue;
		}
		app.ObjectPost()->SetParam(activeProjectilePieces_[i].postEffect);
		app.BeginObjectPostEffect();
		projectilePieces_[i]->Draw();
		app.EndObjectPostEffect();
		app.ObjCom()->SetGraphicsPipelineState();
	}
}

// =============================================
// 繝ｪ繧ｻ繝・ヨ
// =============================================

void EffectSequencer::Reset() {
	state_ = State::Idle;
	elapsedTime_ = 0.0f;
	projectile_.reset();
	projectilePieces_.clear();
	projectilePieceRotations_.clear();
	activeProjectilePieces_.clear();

	if (trail_) {
		trail_->SetActive(false);
		trail_ = nullptr;
	}

	currentPos_ = {};
	projectileRotation_ = {};
}

// =============================================
// JSON 菫晏ｭ・隱ｭ縺ｿ霎ｼ縺ｿ
// =============================================

void EffectSequencer::SaveProfile(const std::string& path, const EffectProfile& profile) {
	std::ofstream file("resources/effect/" + path);
	if (file.is_open()) {
		nlohmann::json j = profile.ToJson();
		file << std::setw(4) << j << std::endl;
	}
}

bool EffectSequencer::LoadProfile(const std::string& path, EffectProfile& profile) {
	std::ifstream file("resources/effect/" + path);
	if (file.is_open()) {
		nlohmann::json j;
		file >> j;
		profile.FromJson(j);
		return true;
	}
	return false;
}

bool EffectSequencer::LoadProfileCached(const std::string& path, EffectProfile& profile) {
	static std::unordered_map<std::string, EffectProfile> profileCache;
	auto cached = profileCache.find(path);
	if (cached != profileCache.end()) {
		profile = cached->second;
		return true;
	}

	if (!LoadProfile(path, profile)) {
		return false;
	}

	profileCache[path] = profile;
	return true;
}

// =============================================
// ImGui 繧ｨ繝・ぅ繧ｿ繝ｼ
// =============================================

#ifdef USE_IMGUI
void EffectSequencer::DrawImGuiEditor(const Vector3& defaultStartPos, const Vector3& defaultTargetPos) {
	ImGui::Begin("Attack Effect Editor");

	// --- 繝・く繧ｹ繝医ヰ繝・ヵ繧｡・・tatic縺ｧ菫晄戟・・---
	static char modelBuf[256] = "";
	static char flyBuf[128] = "";
	static char hitBuf[128] = "";
	static bool needsSync = true;

	// 蛻晏屓 or Load蠕後↓繝舌ャ繝輔ぃ繧貞酔譛・
	if (needsSync) {
		snprintf(modelBuf, sizeof(modelBuf), "%s", editingProfile_.projectile.modelPath.c_str());
		snprintf(flyBuf, sizeof(flyBuf), "%s", editingProfile_.flyParticle.c_str());
		snprintf(hitBuf, sizeof(hitBuf), "%s", editingProfile_.hitParticle.c_str());
		needsSync = false;
	}

	// --- 繧ｹ繝・・繝郁｡ｨ遉ｺ ---
	const char* stateNames[] = { "Idle", "Firing", "Flying", "Hit", "Finished" };
	ImGui::Text("State: %s", stateNames[static_cast<int>(state_)]);
	ImGui::Separator();

	// --- 繝励Ο繝輔ぃ繧､繝ｫ邱ｨ髮・---
	if (ImGui::CollapsingHeader("Projectile", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::InputText("Model Path", modelBuf, sizeof(modelBuf))) {
			editingProfile_.projectile.modelPath = modelBuf;
		}
		ImGui::DragFloat3("Scale", &editingProfile_.projectile.scale.x, 0.01f, 0.01f, 10.0f);
		ImGui::DragFloat3("Rotation Speed", &editingProfile_.projectile.rotationSpeed.x, 0.1f);

		ImGui::Separator();
		ImGui::Text("Placed Bullets");
		if (editingProfile_.projectile.pieces.empty()) {
			if (ImGui::Button("Create From Legacy Bullet")) {
				editingProfile_.projectile.pieces.push_back(MakeLegacyProjectilePiece(editingProfile_.projectile));
			}
		} else {
			if (ImGui::Button("Add Bullet Piece")) {
				auto piece = editingProfile_.projectile.pieces.back();
				piece.name = "Bullet " + std::to_string(editingProfile_.projectile.pieces.size() + 1);
				piece.offset.x += 0.35f;
				editingProfile_.projectile.pieces.push_back(piece);
			}
		}

		for (int i = 0; i < static_cast<int>(editingProfile_.projectile.pieces.size()); ++i) {
			auto& piece = editingProfile_.projectile.pieces[i];
			std::string label = piece.name + "##ProjectilePiece" + std::to_string(i);
			if (!ImGui::TreeNode(label.c_str())) {
				continue;
			}

			char nameBuf[128];
			strcpy_s(nameBuf, piece.name.c_str());
			if (ImGui::InputText(("Name##Piece" + std::to_string(i)).c_str(), nameBuf, sizeof(nameBuf))) {
				piece.name = nameBuf;
			}

			char pieceModelBuf[256];
			strcpy_s(pieceModelBuf, piece.modelPath.c_str());
			if (ImGui::InputText(("Model##Piece" + std::to_string(i)).c_str(), pieceModelBuf, sizeof(pieceModelBuf))) {
				piece.modelPath = pieceModelBuf;
			}

			ImGui::DragFloat3(("Offset##Piece" + std::to_string(i)).c_str(), &piece.offset.x, 0.01f);
			ImGui::DragFloat3(("Rotation##Piece" + std::to_string(i)).c_str(), &piece.rot.x, 0.01f);
			ImGui::DragFloat3(("Scale##Piece" + std::to_string(i)).c_str(), &piece.scale.x, 0.01f, 0.01f, 10.0f);
			ImGui::DragFloat3(("Rotation Speed##Piece" + std::to_string(i)).c_str(), &piece.rotationSpeed.x, 0.1f);
			ImGui::ColorEdit4(("Material Color##Piece" + std::to_string(i)).c_str(), &piece.materialColor.x);

			ImGui::Checkbox(("Enable Lighting##Piece" + std::to_string(i)).c_str(), &piece.enableLighting);
			if (piece.enableLighting) {
				ImGui::DragFloat3(("Light Dir##Piece" + std::to_string(i)).c_str(), &piece.lightDir.x, 0.01f);
				ImGui::ColorEdit4(("Light Color##Piece" + std::to_string(i)).c_str(), &piece.lightColor.x);
				ImGui::DragFloat(("Light Intensity##Piece" + std::to_string(i)).c_str(), &piece.lightIntensity, 0.05f, 0.0f, 20.0f);
			}

			ImGui::Checkbox(("Use PostEffect##Piece" + std::to_string(i)).c_str(), &piece.usePostEffect);
			if (piece.usePostEffect) {
				ImGui::DragFloat(("Bloom Threshold##Piece" + std::to_string(i)).c_str(), &piece.postEffect.threshold, 0.01f, 0.0f, 3.0f);
				ImGui::DragFloat(("Bloom Intensity##Piece" + std::to_string(i)).c_str(), &piece.postEffect.intensity, 0.01f, 0.0f, 10.0f);
				ImGui::DragFloat(("ChromAb##Piece" + std::to_string(i)).c_str(), &piece.postEffect.chromAbAmount, 0.0005f, 0.0f, 0.05f);
				ImGui::DragFloat(("Distortion##Piece" + std::to_string(i)).c_str(), &piece.postEffect.distortionAmount, 0.001f, 0.0f, 0.2f);
				ImGui::DragFloat(("Noise##Piece" + std::to_string(i)).c_str(), &piece.postEffect.noiseIntensity, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat(("Glitch##Piece" + std::to_string(i)).c_str(), &piece.postEffect.glitchAmount, 0.001f, 0.0f, 0.2f);
				ImGui::DragFloat(("Dissolve##Piece" + std::to_string(i)).c_str(), &piece.postEffect.dissolveAmount, 0.01f, -1.0f, 1.0f);
			}

			if (editingProfile_.projectile.pieces.size() > 1 && ImGui::Button(("Delete Piece##" + std::to_string(i)).c_str())) {
				editingProfile_.projectile.pieces.erase(editingProfile_.projectile.pieces.begin() + i);
				ImGui::TreePop();
				break;
			}

			ImGui::TreePop();
		}
	}

	if (ImGui::CollapsingHeader("Particles", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::InputText("Fly Particle", flyBuf, sizeof(flyBuf))) {
			editingProfile_.flyParticle = flyBuf;
		}

		int flyCount = static_cast<int>(editingProfile_.flyParticleCount);
		if (ImGui::SliderInt("Fly Count/Frame", &flyCount, 1, 100)) {
			editingProfile_.flyParticleCount = static_cast<uint32_t>(flyCount);
		}

		if (ImGui::InputText("Hit Particle", hitBuf, sizeof(hitBuf))) {
			editingProfile_.hitParticle = hitBuf;
		}

		int hitCount = static_cast<int>(editingProfile_.hitParticleCount);
		if (ImGui::SliderInt("Hit Count", &hitCount, 1, 1000)) {
			editingProfile_.hitParticleCount = static_cast<uint32_t>(hitCount);
		}
	}

	if (ImGui::CollapsingHeader("Trail", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Enable Trail", &editingProfile_.enableTrail);
		ImGui::ColorEdit4("Trail Start Color", &editingProfile_.trail.startColor.x);
		ImGui::ColorEdit4("Trail End Color", &editingProfile_.trail.endColor.x);

		int maxPts = static_cast<int>(editingProfile_.trail.maxPoints);
		if (ImGui::SliderInt("Max Points", &maxPts, 10, 500)) {
			editingProfile_.trail.maxPoints = static_cast<uint32_t>(maxPts);
		}

		int steps = static_cast<int>(editingProfile_.trail.interpolationSteps);
		if (ImGui::SliderInt("Interp Steps", &steps, 1, 16)) {
			editingProfile_.trail.interpolationSteps = static_cast<uint32_t>(steps);
		}

		ImGui::DragFloat3("Tip Offset", &editingProfile_.trail.tipOffset.x, 0.01f);
		ImGui::DragFloat3("Base Offset", &editingProfile_.trail.baseOffset.x, 0.01f);
		ImGui::DragFloat("Lifetime", &editingProfile_.trail.lifetime, 0.01f, 0.1f, 5.0f);
	}

	if (ImGui::CollapsingHeader("Timing", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("Duration (sec)", &editingProfile_.duration, 0.05f, 0.1f, 10.0f);
		ImGui::DragFloat("Hit Duration (sec)", &editingProfile_.hitDuration, 0.05f, 0.1f, 5.0f);
	}

	ImGui::Separator();

	// --- 繝・せ繝育匱蟆・・繧ｿ繝ｳ ---
	if (ImGui::Button("Test Fire!")) {
		Fire(editingProfile_, defaultStartPos, defaultTargetPos);
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop")) {
		Reset();
	}

	ImGui::Separator();

	// --- 繝輔ぃ繧､繝ｫ謫堺ｽ・---
	ImGui::InputText("Filename", profileFilename_, sizeof(profileFilename_));

	if (ImGui::Button("Save to JSON")) {
		SaveProfile(profileFilename_, editingProfile_);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load from JSON")) {
		if (LoadProfile(profileFilename_, editingProfile_)) {
			// Load謌仙粥譎ゅ↓繝舌ャ繝輔ぃ繧貞・蜷梧悄
			needsSync = true;
		}
	}

	ImGui::End();
}
#endif




