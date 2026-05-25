#pragma once

#include <memory>
#include <vector>

class Camera;
class Card3D;

class BattleFieldViewController {
public:
	static int PickFieldIndexByMouse(
		const std::vector<std::unique_ptr<Card3D>>& fieldViews,
		const Camera& camera,
		int mouseX,
		int mouseY,
		float screenWidth,
		float screenHeight);
};