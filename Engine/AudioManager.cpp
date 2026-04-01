#include "AudioManager.h"

AudioManager* AudioManager::GetInstance() {
	static AudioManager instance;
	return &instance;
}

// 全ての音設定をロードしてAudioクラスに登録する
void AudioManager::LoadAllConfigs(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) return;

	nlohmann::json j;
	file >> j;

	for (auto& [key, value] : j.items()) {
		AudioConfig config;
		config.FromJson(key, value);
		configs_[key] = config;

		// Audioクラス（エンジン側）に読み込み命令を出す
		// std::string -> std::wstring 変換が必要
		std::wstring wName = ConvertString(key);
		std::wstring wPath = ConvertString(config.filePath);
		Audio::GetInstance()->LoadAudio(wName, wPath, config.maxConcurrency);
	}
}

void AudioManager::UpdateImGui() {
    ImGui::Begin("Audio Editor");

    if (ImGui::Button("Add New Audio")) {
        // 重複しない名前を生成
        std::string newName = "NewAudio_" + std::to_string(configs_.size());
        configs_[newName] = AudioConfig{ newName, "resources/audio/sample.wav" };
    }

    ImGui::Separator();

    std::string renameFrom = "";
    std::string renameTo = "";

    int i = 0; // ループカウンターを用意
    for (auto& [name, config] : configs_) {
        std::string treeLabel = name + "##" + std::to_string(i++);

        if (ImGui::TreeNode(treeLabel.c_str())) {
            // --- 1. 名前の変更 (Enterキーで確定するように変更) ---
            char nameBuf[64];
            strcpy_s(nameBuf, name.c_str());

            ImGui::Text("Audio ID:");
            ImGui::SameLine();

            // Flagsに ImGuiInputTextFlags_EnterReturnsTrue を追加
            std::string inputLabel = "##inputID" + std::to_string(i);
            if (ImGui::InputText(inputLabel.c_str(), nameBuf, sizeof(nameBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (strlen(nameBuf) > 0) { // 空文字でないことを確認
                    renameFrom = name;
                    renameTo = nameBuf;
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("(Press Enter)"); // ユーザーにEnterが必要だと伝える

            // --- 2. パスの変更 ---
            // パスもIDを固定して、他の項目の影響を受けないようにする
            char pathBuf[256];
            strcpy_s(pathBuf, config.filePath.c_str());
            std::string pathLabel = "Path##path" + std::to_string(i);
            if (ImGui::InputText(pathLabel.c_str(), pathBuf, sizeof(pathBuf))) {
                config.filePath = pathBuf;
            }

            // --- 3. パラメータの変更 ---
            ImGui::DragFloat("Volume", &config.defaultVolume, 0.01f, 0.0f, 1.0f);
            ImGui::Checkbox("Loop", &config.loop);

            if (ImGui::InputInt("Concurrency", &config.maxConcurrency)) {
                if (config.maxConcurrency < 1) config.maxConcurrency = 1;
            }

            // --- 4. 反映とプレビュー ---
            std::wstring wName = ConvertString(name);
            if (ImGui::Button("Preview Play")) {
                if (config.loop) {
                    Audio::GetInstance()->PlayAudio(wName, config.loop, config.defaultVolume);
                } else {
                    Audio::GetInstance()->PlayAudioSE(wName, config.defaultVolume);
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Apply & Reload")) {
                Audio::GetInstance()->UnloadAudio(wName);
                Audio::GetInstance()->LoadAudio(wName, ConvertString(config.filePath), config.maxConcurrency);
            }

            // --- 5. 削除機能 ---
            if (ImGui::Button("Delete This Audio")) {
                Audio::GetInstance()->UnloadAudio(wName);
                renameFrom = name;
                renameTo = "";
            }

            ImGui::TreePop();
        }
    }

    // mapの更新（ループ外で行う）
    if (!renameFrom.empty()) {
        if (renameTo.empty()) {
            configs_.erase(renameFrom);
        } else if (renameFrom != renameTo) {
            configs_[renameTo] = configs_[renameFrom];
            configs_[renameTo].name = renameTo;
            configs_.erase(renameFrom);
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Save Config")) {
        SaveAllConfigs("resources/configs/audioSettings.json");
    }

    ImGui::End(); // 正しいEndはここ一箇所だけ！
}

// 文字列変換用ヘルパー (std::string <-> std::wstring)
std::wstring AudioManager::ConvertString(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

void AudioManager::SaveAllConfigs(const std::string& path) {
	nlohmann::json j;
	for (auto& [name, config] : configs_) {
		j[name] = config.ToJson();
	}
	std::ofstream file(path);
	file << std::setw(4) << j << std::endl;
}

// AudioManager経由でSEを鳴らす
void AudioManager::PlaySE(const std::string& name) {
    auto it = configs_.find(name);
    if (it == configs_.end()) return;

    // エディターで設定した volume を使って再生！
    Audio::GetInstance()->PlayAudioSE(
        ConvertString(name),
        it->second.defaultVolume
    );
}

// BGMも同様
void AudioManager::PlayBGM(const std::string& name) {
    // 1. 設定が存在するかチェック
    auto it = configs_.find(name);
    if (it == configs_.end()) return;

    // 2. 既に同じBGMが流れているなら、何もしない（最初から再生し直したい場合はここを消す）
    if (currentBGMName_ == name) return;

    // 3. 他のBGMが流れていたら停止する
    if (!currentBGMName_.empty()) {
        Audio::GetInstance()->StopAudio(ConvertString(currentBGMName_));
    }

    // 4. 新しいBGMを再生
    // エディターの Loop 設定と Volume 設定をそのまま使う
    Audio::GetInstance()->PlayAudio(
        ConvertString(name),
        it->second.loop,
        it->second.defaultVolume
    );

    // 5. 「今はこの音が流れている」と記憶する
    currentBGMName_ = name;
}

void AudioManager::StopBGM() {
    if (!currentBGMName_.empty()) {
        Audio::GetInstance()->StopAudio(ConvertString(currentBGMName_));
        currentBGMName_ = "";
    }
}