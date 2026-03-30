#include "Player.h"
#include "Object3dCommon.h"
#include "DirectXCommon.h"
#include "Matrix4x4.h"
#include "AnimationEvaluate.h"
#include "ImGuizmo.h"
#include "Camera.h"

#ifdef USE_IMGUI
#include <imgui.h>
#include <functional>
#include <fstream>
#include "json.hpp"
using json = nlohmann::json;
#endif

static Quaternion EulerDegToQuaternion(const Vector3& rotDeg) {
    float radX = rotDeg.x * (3.14159265f / 180.0f);
    float radY = rotDeg.y * (3.14159265f / 180.0f);
    float radZ = rotDeg.z * (3.14159265f / 180.0f);

    float cx = std::cos(radX * 0.5f); float sx = std::sin(radX * 0.5f);
    float cy = std::cos(radY * 0.5f); float sy = std::sin(radY * 0.5f);
    float cz = std::cos(radZ * 0.5f); float sz = std::sin(radZ * 0.5f);

    Quaternion q;
    q.w = cx * cy * cz + sx * sy * sz;
    q.x = sx * cy * cz - cx * sy * sz;
    q.y = cx * sy * cz + sx * cy * sz;
    q.z = cx * cy * sz - sx * sy * cz;
    return q;
}

// JSONファイルからアニメーションを読み込む関数
static Animation LoadAnimationFromJson(const std::string& filepath) {
    Animation anim;
    std::ifstream file(filepath);
    if (!file.is_open()) return anim;

    nlohmann::json j;
    file >> j;

    anim.duration = j.value("duration", 0.0f);

    if (j.contains("nodeAnimations")) {
        for (auto& [boneName, jNode] : j["nodeAnimations"].items()) {
            auto& na = anim.nodeAnimations[boneName];

            if (jNode.contains("translate")) {
                for (auto& jKey : jNode["translate"]) {
                    Vector3 v; // 1つずつ安全に代入！
                    v.x = jKey["value"][0].get<float>();
                    v.y = jKey["value"][1].get<float>();
                    v.z = jKey["value"][2].get<float>();
                    na.translate.keyframes.push_back({ jKey["time"].get<float>(), v });
                }
            }
            if (jNode.contains("rotate")) {
                for (auto& jKey : jNode["rotate"]) {
                    Quaternion q;
                    q.x = jKey["value"][0].get<float>();
                    q.y = jKey["value"][1].get<float>();
                    q.z = jKey["value"][2].get<float>();
                    q.w = jKey["value"][3].get<float>();
                    na.rotate.keyframes.push_back({ jKey["time"].get<float>(), q });
                }
            }
            if (jNode.contains("scale")) {
                for (auto& jKey : jNode["scale"]) {
                    Vector3 s;
                    s.x = jKey["value"][0].get<float>();
                    s.y = jKey["value"][1].get<float>();
                    s.z = jKey["value"][2].get<float>();
                    // ★強制防衛：スケールが0になっていたら絶対に1.0に戻す！
                    if (s.x == 0.0f && s.y == 0.0f && s.z == 0.0f) { s = { 1.0f, 1.0f, 1.0f }; }
                    na.scale.keyframes.push_back({ jKey["time"].get<float>(), s });
                }
            }
        }
    }
    return anim;
}

static Vector3 QuaternionToEulerDeg(const Quaternion& q) {
    Vector3 rot;
    float sinr_cosp = 2.0f * (q.w * q.x + q.y * q.z);
    float cosr_cosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    rot.x = std::atan2(sinr_cosp, cosr_cosp);

    float sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (std::abs(sinp) >= 1.0f) rot.y = std::copysign(3.14159265f / 2.0f, sinp);
    else rot.y = std::asin(sinp);

    float siny_cosp = 2.0f * (q.w * q.z + q.x * q.y);
    float cosy_cosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    rot.z = std::atan2(siny_cosp, cosy_cosp);

    const float rad2deg = 180.0f / 3.14159265f;
    rot.x *= rad2deg; rot.y *= rad2deg; rot.z *= rad2deg;
    return rot;
}

struct Ray {
    Vector3 origin;
    Vector3 direction;
};

// 画面のクリック座標から、3D空間に撃ち込むレーザー（Ray）を作る関数
static Ray ComputePickingRay(float mouseX, float mouseY, float screenW, float screenH, const Matrix4x4& viewMat, const Matrix4x4& projMat) {
    // 画面座標を -1.0 ～ 1.0 の正規化デバイス座標（NDC）に変換
    float x = (2.0f * mouseX) / screenW - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenH;

    // カメラの View と Projection を掛けたものの「逆行列」を作る
    Matrix4x4 viewProj = Matrix4x4::Multiply(viewMat, projMat);
    Matrix4x4 invViewProj = Matrix4x4::Inverse(viewProj);

    // 画面の手前（Near）と奥（Far）の座標を作成
    Vector4 clipStart{ x, y, 0.0f, 1.0f };
    Vector4 clipEnd{ x, y, 1.0f, 1.0f };

    // 逆行列を使って、3Dのワールド座標に戻す！
    Vector4 worldStart = MulRowVec4Mat4(clipStart, invViewProj);
    Vector4 worldEnd = MulRowVec4Mat4(clipEnd, invViewProj);

    worldStart.x /= worldStart.w; worldStart.y /= worldStart.w; worldStart.z /= worldStart.w;
    worldEnd.x /= worldEnd.w; worldEnd.y /= worldEnd.w; worldEnd.z /= worldEnd.w;

    Vector3 origin{ worldStart.x, worldStart.y, worldStart.z };
    Vector3 end{ worldEnd.x, worldEnd.y, worldEnd.z };

    // 方向ベクトルを計算して正規化
    Vector3 dir = end - origin;
    float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len > 0.0f) { dir.x /= len; dir.y /= len; dir.z /= len; }

    return { origin, dir };
}

// レーザー（Ray）と、ボーンの球体（Sphere）がぶつかったか判定する関数
static bool RaySphereIntersect(const Ray& ray, const Vector3& center, float radius, float& outDist) {
    Vector3 oc = ray.origin - center;
    float a = ray.direction.x * ray.direction.x + ray.direction.y * ray.direction.y + ray.direction.z * ray.direction.z;
    float b = 2.0f * (oc.x * ray.direction.x + oc.y * ray.direction.y + oc.z * ray.direction.z);
    float c = (oc.x * oc.x + oc.y * oc.y + oc.z * oc.z) - radius * radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    float t = (-b - sqrtf(discriminant)) / (2.0f * a);
    if (t < 0) t = (-b + sqrtf(discriminant)) / (2.0f * a);
    if (t < 0) return false;

    outDist = t; // ぶつかった距離
    return true;
}

void Player::Initialize(Object3dCommon* objCommon, DirectXCommon* dx, Camera* cam) {
    model_ = std::make_unique<Object3d>();
    model_->Initialize(objCommon, dx);
    model_->SetCamera(cam);

    // 複雑なコンボモデルは捨てて、基本となるモデルを1つだけ読み込む
    model_->SetModel("enemy/boss/boss.gltf");
    model_->SetScale({ 0.5f, 0.5f, 0.5f });
    // 待機アニメーションをループ再生
    if (model_->HasAnimation()) {
        model_->PlayAnimation("Idle", true);
    }

    basePos_ = pos_;

    block_ = 0;
    boostedPower_ = 0;

    isAlive_ = true;
}

void Player::PlayAttackAnim(const Vector3& targetPos) {
    animState_ = AnimState::AttackForward;
    animTimer_ = 0.0f;
    animDuration_ = 0.2f; // 0.2秒で突進
    startPos_ = basePos_;
    // 相手の位置の少し手前まで移動
    targetPos_ = Lerp(basePos_, targetPos, 0.8f);
}

void Player::PlayDamageAnim() {
    animState_ = AnimState::Damage;
    animTimer_ = 0.0f;
    animDuration_ = 0.15f; // 0.15秒でのけぞる
    startPos_ = basePos_;
    // プレイヤーは左側にいる想定なので、左(Xのマイナス方向)に下がる
    targetPos_ = { basePos_.x - 2.0f, basePos_.y, basePos_.z };
}

void Player::Update(float dt) {
    // 動き（アニメーション）の計算
    if (animState_ != AnimState::Idle) {
        animTimer_ += dt;
        float t = animTimer_ / animDuration_;
        if (t > 1.0f) t = 1.0f;

        // イージング（後半ゆっくりになる計算）
        float easeT = t * (2.0f - t);
        pos_ = Lerp(startPos_, targetPos_, easeT);

        if (animTimer_ >= animDuration_) {
            if (animState_ == AnimState::AttackForward) {
                // 突進が終わったら戻る
                animState_ = AnimState::AttackReturn;
                animTimer_ = 0.0f;
                animDuration_ = 0.3f; // 戻る時は少しゆっくり
                startPos_ = pos_;
                targetPos_ = basePos_;
            } else if (animState_ == AnimState::AttackReturn || animState_ == AnimState::Damage) {
                // 戻り・ダメージ終了で待機状態へ
                animState_ = AnimState::Idle;
                pos_ = basePos_;
            }
        }
    }

    // 赤点滅（フラッシュ）の計算
    if (flashTimer_ > 0.0f) {
        flashTimer_ -= dt;
        model_->SetMaterialColor({ 1.0f, 0.2f, 0.2f, 1.0f });
    } else {
        model_->SetMaterialColor({ 1.0f, 1.0f, 1.0f, 1.0f });
    }
    if (model_) {
        model_->SetTranslate(pos_);
        model_->SetRotate(rot_);

        // アニメーションの更新
        model_->Update(dt);
    }

    // 敵の弾などが当たったか判定するための箱（AABB）を自分の位置に合わせて更新
    body_.min = { pos_.x - 1.0f, pos_.y, pos_.z - 1.0f };
    body_.max = { pos_.x + 1.0f, pos_.y + 2.0f, pos_.z + 1.0f };
    
    ModelParticleManager::GetInstance()->LoadFromJson("fire_particle.json", attackEffectConfig_);
    
    // Update内
    attackEffectConfig_.position = pos_ + Vector3(0, 1.0f, 0); // 位置だけ更新
    for (int i = 0; i < 2; ++i) {
        ModelParticleManager::GetInstance()->Emit(
            ModelParticleManager::GetInstance()->MakeParticle(attackEffectConfig_)
        );
    }
}

void Player::Draw() {
    if (model_) {
        model_->Draw();
    }
}

void Player::SetSpawnPos(const Vector3& p) {
    pos_ = p;
    basePos_ = p;
}

void Player::SetRotation(const Vector3& r) {
    rot_ = r;
}

void Player::Damage(int damage) 
{
    // ブロックの値が０じゃないならブロックの値分ダメージを減らす
    if (block_ > 0) {
        if (block_ >= damage) {
            // ブロックの方が大きい（全ダメージ防げる）場合
            block_ -= damage;
            damage = 0;
        } else {
            // ダメージの方が大きい（ブロックが壊れる）場合
            damage -= block_;
            block_ = 0;
        }
    }

    // ダメージが0以下ならダメージ処理なし
    if (damage <= 0) {
        return;
    }

    hp_ -= damage;

    if (hp_ < 0) {
        hp_ = 0;
        isAlive_ = false;
    }
}

void Player::DrawAnimationEditorImGui() {
#ifdef USE_IMGUI
    if (!model_) return;
    Model::Skeleton* skeleton = model_->GetSkeleton();
    if (!skeleton || skeleton->joints.empty()) return;

    // ===============================================
    // ★追加：テスト再生中かどうかを判定するスイッチ
    // ===============================================
    static std::unordered_map<std::string, Quaternion> s_editorQuats;
    static bool isTestingPlay = false;

    if (!isTestingPlay) {
        // エディタの骨がリセットされたら金庫も空にする
        if (model_->boneOffsets_.empty()) { s_editorQuats.clear(); }

        for (const auto& joint : skeleton->joints) {
            if (model_->boneOffsets_.find(joint.name) == model_->boneOffsets_.end()) {
                auto& offset = model_->boneOffsets_[joint.name];
                offset.translate = joint.transform.translate;
                offset.scale = joint.transform.scale;
                if (offset.scale.x == 0.0f) offset.scale = { 1.0f, 1.0f, 1.0f };

                // 表示用のオイラー角（ここはバグってもヨシ）
                offset.rotate = QuaternionToEulerDeg(joint.transform.rotate);

                // ★本物の純粋なデータを金庫に隔離保管！！
                s_editorQuats[joint.name] = joint.transform.rotate;
            }
        }
    }

    // ==============================================
    // 1. ボーン階層ツリー（左側パネルなど）
    // ==============================================
    ImGui::Begin("Animation Editor (Hierarchy)");

    // 再帰関数（ラムダ式）でツリーを描画
    std::function<void(int)> DrawBoneNode = [&](int jointIndex) {
        if (jointIndex < 0 || jointIndex >= skeleton->joints.size()) return;
        auto& joint = skeleton->joints[jointIndex];

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
        if (joint.children.empty()) { flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen; }

        // ★選択ハイライト
        if (jointIndex == selectedJointIndex_) { flags |= ImGuiTreeNodeFlags_Selected; }

        ImGui::PushID(jointIndex);
        bool isOpen = ImGui::TreeNodeEx(joint.name.c_str(), flags);

        // ★クリックで選択
        if (ImGui::IsItemClicked()) {
            selectedJointIndex_ = jointIndex;
        }

        if (isOpen && !joint.children.empty() || (flags & ImGuiTreeNodeFlags_Leaf)) {
            if (!joint.children.empty()) {
                for (int childIndex : joint.children) { DrawBoneNode(childIndex); }
                ImGui::TreePop();
            }
        }
        ImGui::PopID();
        };

    ImGui::Text("Model Bone Hierarchy");
    ImGui::Separator();
    DrawBoneNode(skeleton->root);
    ImGui::End();

    // ==============================================
    // 2. ギズモ操作用の設定（小さなウィンドウなど）
    // ==============================================
    ImGui::Begin("Gizmo Settings");
    if (ImGui::RadioButton("Translate (W)", currentGizmoOperation_ == ImGuizmo::TRANSLATE)) currentGizmoOperation_ = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate (E)", currentGizmoOperation_ == ImGuizmo::ROTATE)) currentGizmoOperation_ = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale (R)", currentGizmoOperation_ == ImGuizmo::SCALE)) currentGizmoOperation_ = ImGuizmo::SCALE;

    ImGui::Separator();
    if (ImGui::RadioButton("Local (L)", currentGizmoMode_ == ImGuizmo::LOCAL)) currentGizmoMode_ = ImGuizmo::LOCAL;
    ImGui::SameLine();
    if (ImGui::RadioButton("World (K)", currentGizmoMode_ == ImGuizmo::WORLD)) currentGizmoMode_ = ImGuizmo::WORLD;

    ImGui::Separator();
    if (selectedJointIndex_ >= 0) {
        ImGui::Text("Selected: %s", skeleton->joints[selectedJointIndex_].name.c_str());
        if (ImGui::Button("Reset Selected Bone")) {
            model_->boneOffsets_[skeleton->joints[selectedJointIndex_].name] = Object3d::BoneOffset();
        }
    } else { ImGui::Text("Selected: None"); }
    ImGui::End();

    // ==============================================
     // 3. 画面上にギズモを描画する
     // ==============================================
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::BeginFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

    // camera_ に直接アクセスするのではなく、GetCamera() を使う
    Camera* cam = model_->GetCamera();
    if (!cam) return;

    Matrix4x4 viewMat = cam->GetViewMatrix();
    Matrix4x4 projMat = cam->GetProjectionMatrix();

    // ImGuiのウィンドウ上ではなく、かつギズモの矢印を引っ張っていない時に左クリックされたら
    if (ImGui::IsMouseClicked(0) && !io.WantCaptureMouse && !ImGuizmo::IsOver()) {

        // 1. クリックした場所にレーザーを撃つ
        Ray ray = ComputePickingRay(io.MousePos.x, io.MousePos.y, io.DisplaySize.x, io.DisplaySize.y, viewMat, projMat);

        Matrix4x4 objectWorld = model_->GetWorldMatrix();
        float closestDist = 999999.0f;
        int hitJointIndex = -1;

        // 2. すべてのボーンとレーザーの当たり判定を行う
        for (size_t i = 0; i < skeleton->joints.size(); ++i) {
            auto& joint = skeleton->joints[i];

            // ボーンの現在のワールド空間での座標を計算
            Matrix4x4 finalWorld = Matrix4x4::Multiply(joint.skeletonSpaceMatrix, objectWorld);
            Vector3 jointPos = { finalWorld.m[3][0], finalWorld.m[3][1], finalWorld.m[3][2] };

            // 当たり判定用の球の大きさ（クリックしやすさ。もし当たりにくければ 1.0f など大きくしてください）
            float hitRadius = 0.3f;
            float hitDist = 0.0f;

            // ぶつかっていて、かつ今までで一番手前にあるボーンなら記録する
            if (RaySphereIntersect(ray, jointPos, hitRadius, hitDist)) {
                if (hitDist < closestDist) {
                    closestDist = hitDist;
                    hitJointIndex = (int)i;
                }
            }
        }

        // 3. 当たったボーンがあれば、それを「選択中」にする！
        if (hitJointIndex != -1) {
            selectedJointIndex_ = hitJointIndex;
        }
    }
    // ボーンが選択されている時だけギズモを出す
    if (selectedJointIndex_ >= 0 && !isTestingPlay) {
        auto& joint = skeleton->joints[selectedJointIndex_];

        //データがなければ初期化
        if (model_->boneOffsets_.find(joint.name) == model_->boneOffsets_.end()) {
            model_->boneOffsets_[joint.name] = Object3d::BoneOffset();
        }
        auto& offset = model_->boneOffsets_[joint.name];
        // ----------------------------------------------
         // A. ギズモに渡す「ボーンの最終ワールド行列」を計算する
         // ----------------------------------------------
         // ★修正：スケールをごまかす処理を削除し、純粋に計算します！
         // （無限増殖バグは直っているので、これで完璧な位置に出ます）
        Matrix4x4 objectWorld = model_->GetWorldMatrix();
        Matrix4x4 boneSkeletonSpace = joint.skeletonSpaceMatrix;
        Matrix4x4 boneFinalWorld = Matrix4x4::Multiply(boneSkeletonSpace, objectWorld);

        float gizmoMat[16];
        boneFinalWorld.ToFloat16(gizmoMat);

        // ----------------------------------------------
        // B. ギズモの描画と操作！
        // ----------------------------------------------
        ImGuizmo::Manipulate(viewMat.m[0], projMat.m[0], currentGizmoOperation_, currentGizmoMode_, gizmoMat);

        // ----------------------------------------------
         // C. ギズモ操作による「新しいワールド行列」を逆変換して反映する！
         // ----------------------------------------------
        if (ImGuizmo::IsUsing()) {
            Matrix4x4 newBoneFinalWorld = Matrix4x4::FromFloat16(gizmoMat);

            // 1. 親ボーンのワールド行列を取得
            Matrix4x4 parentWorld = objectWorld;
            if (joint.parent.has_value() && joint.parent.value() >= 0) {
                int32_t pIdx = joint.parent.value();
                parentWorld = Matrix4x4::Multiply(skeleton->joints[pIdx].skeletonSpaceMatrix, objectWorld);
            }

            // 2. ギズモ操作後の「新しい絶対ローカル行列 (newLocal)」
            Matrix4x4 newLocal = Matrix4x4::Multiply(newBoneFinalWorld, Matrix4x4::Inverse(parentWorld));

            // ★修正：複雑な差分計算をやめ、絶対座標をそのまま保存する！
            Vector3 T, R, S;
            newLocal.Decompose(T, R, S);

            offset.translate = T;
            offset.rotate = R;
            offset.scale = S;
            s_editorQuats[joint.name] = MatrixToQuaternion(newLocal);
        }
    }
    // ==============================================
    // 4. ★追加：タイムラインとキーフレーム操作！
    // ==============================================
    ImGui::Begin("Animation Editor (Timeline)");

    ImGui::DragFloat("Max Duration (sec)", &editorMaxDuration_, 0.1f, 0.1f, 10.0f);
    editedAnim_.duration = editorMaxDuration_; // データの長さを同期

    // ★スライダーを動かした時、保存されている「その時間のポーズ」を読み込んで反映する
    if (ImGui::SliderFloat("Time", &editorTime_, 0.0f, editorMaxDuration_)) {
        for (auto& pair : editedAnim_.nodeAnimations) {
            const std::string& boneName = pair.first;
            auto& na = pair.second;

            if (model_->boneOffsets_.find(boneName) == model_->boneOffsets_.end()) {
                model_->boneOffsets_[boneName] = Object3d::BoneOffset();
            }
            auto& offset = model_->boneOffsets_[boneName];

            // 記録されているキーフレームから、現在の時間の数値を計算して上書き
            if (!na.translate.keyframes.empty()) {
                offset.translate = CalculateValue(na.translate.keyframes, editorTime_);
            }
            if (!na.rotate.keyframes.empty()) {
                Quaternion q = CalculateValue(na.rotate.keyframes, editorTime_);
                offset.rotate = QuaternionToEulerDeg(q); // Degreeに戻す
                s_editorQuats[boneName] = q;
            }
            if (!na.scale.keyframes.empty()) {
                offset.scale = CalculateValue(na.scale.keyframes, editorTime_);
            }
        }
    }

    ImGui::Separator();

    // 現在の全身のポーズを、キーフレームとして記録（全身セーブ）するボタン
    if (ImGui::Button("Record Keyframe (Save ALL bones)", ImVec2(250, 40))) {

        for (auto& joint : skeleton->joints) {
            const std::string& boneName = joint.name;

            if (model_->boneOffsets_.find(boneName) == model_->boneOffsets_.end()) {
                model_->boneOffsets_[boneName] = Object3d::BoneOffset();
            }
            auto& offset = model_->boneOffsets_[boneName];
            auto& na = editedAnim_.nodeAnimations[boneName];

            // --- 移動(Translate)の記録 ---
            Vector3 safePos = offset.translate;
            if (std::isnan(safePos.x)) safePos = { 0.0f, 0.0f, 0.0f }; // 念のためNaNチェック

            auto itT = std::find_if(na.translate.keyframes.begin(), na.translate.keyframes.end(), [&](const KeyframeVector3& k) { return std::abs(k.time - editorTime_) < 0.001f; });
            if (itT != na.translate.keyframes.end()) { itT->value = safePos; } else { na.translate.keyframes.push_back({ editorTime_, safePos }); }

            // ==============================================================
            // ★絶対防衛線１：クォータニオンの崩壊を防ぐ！（キモく捻じれる最大の原因）
            // ==============================================================
            Quaternion q = s_editorQuats[boneName];
            float qLen = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
            // 長さが0、または計算エラー(NaN)になっていたら、安全な「無回転」に戻す
            if (qLen < 0.0001f || std::isnan(qLen)) {
                q = { 0.0f, 0.0f, 0.0f, 1.0f };
            } else {
                // 確実に正規化（長さを1.0にする）して保存する
                q.x /= qLen; q.y /= qLen; q.z /= qLen; q.w /= qLen;
            }

            

            auto itR = std::find_if(na.rotate.keyframes.begin(), na.rotate.keyframes.end(), [&](const KeyframeQuaternion& k) { return std::abs(k.time - editorTime_) < 0.001f; });
            if (itR != na.rotate.keyframes.end()) { itR->value = q; } else { na.rotate.keyframes.push_back({ editorTime_, q }); }

            // ==============================================================
            // ★絶対防衛線２：スケールの消失を防ぐ！（ぐしゃぐしゃに丸くなる最大の原因）
            // ==============================================================
            Vector3 safeScale = offset.scale;
            // スケールが0.0に近すぎる、またはNaNになっていたら、強制的に「1.0（等倍）」に戻す
            if (std::abs(safeScale.x) < 0.001f || std::isnan(safeScale.x)) safeScale.x = 1.0f;
            if (std::abs(safeScale.y) < 0.001f || std::isnan(safeScale.y)) safeScale.y = 1.0f;
            if (std::abs(safeScale.z) < 0.001f || std::isnan(safeScale.z)) safeScale.z = 1.0f;

            auto itS = std::find_if(na.scale.keyframes.begin(), na.scale.keyframes.end(), [&](const KeyframeVector3& k) { return std::abs(k.time - editorTime_) < 0.001f; });
            if (itS != na.scale.keyframes.end()) { itS->value = safeScale; } else { na.scale.keyframes.push_back({ editorTime_, safeScale }); }

            // ★時間を小さい順にソートする
            std::sort(na.translate.keyframes.begin(), na.translate.keyframes.end(), [](const KeyframeVector3& a, const KeyframeVector3& b) { return a.time < b.time; });
            std::sort(na.rotate.keyframes.begin(), na.rotate.keyframes.end(), [](const KeyframeQuaternion& a, const KeyframeQuaternion& b) { return a.time < b.time; });
            std::sort(na.scale.keyframes.begin(), na.scale.keyframes.end(), [](const KeyframeVector3& a, const KeyframeVector3& b) { return a.time < b.time; });
        }
    }

    // 5. JSONへのエクスポート（保存）機能
    ImGui::Separator();
    ImGui::Text("Export Animation");

    // 保存するファイル名を入力する欄
    static char exportFileName[256] = "Resources/CustomAnim/CustomAnim.json";
    ImGui::InputText("File Path", exportFileName, sizeof(exportFileName));

    if (ImGui::Button("Export to JSON", ImVec2(250, 40))) {

        for (const auto& joint : skeleton->joints) {
            auto& na = editedAnim_.nodeAnimations[joint.name];

            // 1. もし1回も動かしていない（キーフレームが無い）骨があれば、初期ポーズを自動で登録！
            if (na.translate.keyframes.empty()) na.translate.keyframes.push_back({ 0.0f, joint.transform.translate });
            if (na.rotate.keyframes.empty()) na.rotate.keyframes.push_back({ 0.0f, joint.transform.rotate });
            if (na.scale.keyframes.empty()) {
                Vector3 initScale = joint.transform.scale;
                if (initScale.x == 0.0f) initScale = { 1.0f, 1.0f, 1.0f }; // 念のためのゼロ対策
                na.scale.keyframes.push_back({ 0.0f, initScale });
            }

            // 2. キーフレームが「1つ」しかない場合、エンジンが計算エラー（0になる）を起こすため、
            //    終点（duration）にも同じポーズをコピーして必ず「2つ以上」にする！
            if (na.translate.keyframes.size() == 1) na.translate.keyframes.push_back({ editedAnim_.duration, na.translate.keyframes[0].value });
            if (na.rotate.keyframes.size() == 1) na.rotate.keyframes.push_back({ editedAnim_.duration, na.rotate.keyframes[0].value });
            if (na.scale.keyframes.size() == 1) na.scale.keyframes.push_back({ editedAnim_.duration, na.scale.keyframes[0].value });

            // 順番をソートしておく
            std::sort(na.translate.keyframes.begin(), na.translate.keyframes.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
            std::sort(na.rotate.keyframes.begin(), na.rotate.keyframes.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
            std::sort(na.scale.keyframes.begin(), na.scale.keyframes.end(), [](const auto& a, const auto& b) { return a.time < b.time; });
        }

        // JSONオブジェクトを作成
        json j;
        j["duration"] = editedAnim_.duration;
        j["nodeAnimations"] = json::object(); // カラっぽのオブジェクトを用意

        // 全てのボーンのアニメーションデータをJSONに詰め込む
        for (const auto& pair : editedAnim_.nodeAnimations) {
            const std::string& boneName = pair.first;
            const auto& na = pair.second;

            // もしこのボーンにキーフレームが1つも無ければスキップ（データ容量の節約！）
            if (na.translate.keyframes.empty() && na.rotate.keyframes.empty() && na.scale.keyframes.empty()) {
                continue;
            }

            json jNode;

            // --- 移動 (Translate) ---
            json jTrans = json::array();
            for (const auto& k : na.translate.keyframes) {
                jTrans.push_back({
                    {"time", k.time},
                    {"value", {k.value.x, k.value.y, k.value.z}}
                    });
            }
            jNode["translate"] = jTrans;

            // --- 回転 (Rotate - クォータニオン) ---
            json jRot = json::array();
            for (const auto& k : na.rotate.keyframes) {
                jRot.push_back({
                    {"time", k.time},
                    {"value", {k.value.x, k.value.y, k.value.z, k.value.w}}
                    });
            }
            jNode["rotate"] = jRot;

            // --- 拡縮 (Scale) ---
            json jScale = json::array();
            for (const auto& k : na.scale.keyframes) {
                jScale.push_back({
                    {"time", k.time},
                    {"value", {k.value.x, k.value.y, k.value.z}}
                    });
            }
            jNode["scale"] = jScale;

            // ボーンの名前をキーにしてノードを追加
            j["nodeAnimations"][boneName] = jNode;
        }

        // ファイルに書き出す
        std::ofstream file(exportFileName);
        if (file.is_open()) {
            file << j.dump(4); // 4はインデントのスペース数（綺麗に改行されるようにする）
            file.close();

            // 保存成功のログを出す（必要ならコンソールなどにも）
            OutputDebugStringA(("Animation Exported to: " + std::string(exportFileName) + "\n").c_str());
        } else {
            OutputDebugStringA("Failed to open file for writing!\n");
        }
    }
    ImGui::Separator();
    ImGui::Text("Test Animation");
    ImGui::Separator();
    ImGui::Text("Import Animation for Editing");

    // ★追加：保存されているJSONをタイムラインに読み込んで、続きを作れるようにするボタン
    if (ImGui::Button("Load JSON to Edit", ImVec2(250, 40))) {

        // 1. JSONファイルを読み込む（関数は作成済みのものを使い回せます！）
        Animation loadedAnim = LoadAnimationFromJson(exportFileName);

        if (loadedAnim.duration > 0.0f) {
            // 2. エディタが編集しているデータ（editedAnim_）にまるごと上書きする
            editedAnim_ = loadedAnim;
            editorMaxDuration_ = loadedAnim.duration; // スライダーの最大値も合わせる
            editorTime_ = 0.0f; // タイムラインの時間を先頭(0.0)に戻す

            // 3. 0.0秒の時点のポーズを計算して、キャラクターの関節（boneOffsets_）に即座に反映させる！
            for (auto& pair : editedAnim_.nodeAnimations) {
                const std::string& boneName = pair.first;
                auto& na = pair.second;

                if (model_->boneOffsets_.find(boneName) == model_->boneOffsets_.end()) {
                    model_->boneOffsets_[boneName] = Object3d::BoneOffset();
                }
                auto& offset = model_->boneOffsets_[boneName];

                if (!na.translate.keyframes.empty()) {
                    offset.translate = CalculateValue(na.translate.keyframes, editorTime_);
                }
                if (!na.rotate.keyframes.empty()) {
                    Quaternion q = CalculateValue(na.rotate.keyframes, editorTime_);
                    offset.rotate = QuaternionToEulerDeg(q); // Degreeに戻す
                    s_editorQuats[boneName] = q;
                }
                if (!na.scale.keyframes.empty()) {
                    offset.scale = CalculateValue(na.scale.keyframes, editorTime_);
                }
            }
            OutputDebugStringA("Animation Loaded for Editing!\n");
        }
    }

    if (isTestingPlay) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "=== Now Playing Animation ===");

        if (ImGui::Button("Stop Play & Return to Editor", ImVec2(250, 40))) {
            isTestingPlay = false; // エディタモードに戻る
            // ※もし model_->isPlayAnimation_ を false にする関数があればここで呼びます
        }
    } else {
        if (ImGui::Button("Load JSON & Play!", ImVec2(250, 40))) {
            Animation loadedAnim = LoadAnimationFromJson(exportFileName);

            if (loadedAnim.duration > 0.0f) {
                model_->GetModel()->AddAnimation("CustomAnim", loadedAnim);
                model_->PlayAnimation("CustomAnim", true);

                model_->boneOffsets_.clear(); // 一旦エディタの骨を空っぽにする
                isTestingPlay = true; // ★追加：エディタを一時停止して再生モードに入る！

                OutputDebugStringA("Animation Loaded and Playing!\n");
            }
        }
    }

    ImGui::End();
#endif
}