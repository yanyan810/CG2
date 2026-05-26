#pragma once

class Camera;
class EnemyManager;

class CardTargetingController {
public:
	int PickHoveredEnemy(
		EnemyManager* enemyMgr,
		const Camera* camera,
		int mouseX,
		int mouseY,
		float screenWidth,
		float screenHeight) const;

	void ClearHighlights(EnemyManager* enemyMgr) const;
	void ApplyHoverHighlight(EnemyManager* enemyMgr, int hoverIndex) const;
	bool IsValidTarget(EnemyManager* enemyMgr, int index) const;
};
