#pragma once
#include <memory>

#include "TextSprite.h"

#include<imgui.h>

class GameApp;
class Input;

class PausingUI
{
public:

	void Initialize(GameApp& app);
	void Update(GameApp& app, Input* input);
	void Draw(GameApp& app);

    void DrawImGui();

	//	ポーズ状態の取得
	bool GetIsPaused() const { return isPaused_; }

private:

	// ポーズ状態の管理
	bool isPaused_ = false;

	std::unique_ptr<Sprite> pausingBg_;           //　ポーズ中暗くする用の背景スプライト
	std::unique_ptr<Sprite> pausingSprite_;       //　ポーズ中UIスプライト										      
	std::unique_ptr<Sprite> resumeBg_;            //　再開の背景スプライト
	std::unique_ptr<Sprite> deckCheckBg_;         //　デッキ確認の背景スプライト
	std::unique_ptr<Sprite> giveUpCheckBg_;       //　降参確認の背景スプライト
	std::unique_ptr<Sprite> tutrialCheckBg_;       //　チュートリアル確認の背景スプライト

	// 名前で判定するための走査用リスト
	std::vector<Sprite*> interactiveSprites_;
};