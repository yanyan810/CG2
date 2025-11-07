#include "TextureManager.h"
TextureManager* TextureManager::instance = nullptr;

TextureManager* TextureManager::GetInstance() {

	if (instance == nullptr) {
		instance = new TextureManager();
	}
	return instance;
}

void TextureManager::Finalize() {
	//if (instance != nullptr) {
		delete instance;
		instance = nullptr;
	//}
}

void TextureManager::Initialize() {

	textureDatas_.reserve(DirectXCommon::kMaxSRVCount);

}