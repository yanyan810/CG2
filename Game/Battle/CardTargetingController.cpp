#include "CardTargetingController.h"

#include "Camera.h"
#include "Enemy.h"

int CardTargetingController::PickHoveredEnemy(
	EnemyManager* enemyMgr,
	const Camera* camera,
	int mouseX,
	int mouseY,
	float screenWidth,
	float screenHeight) const
{
	if (!enemyMgr || !camera) {
		return -1;
	}

	return enemyMgr->PickEnemyByMouse(
		mouseX,
		mouseY,
		camera->GetViewProjectionMatrix(),
		screenWidth,
		screenHeight);
}

void CardTargetingController::ClearHighlights(EnemyManager* enemyMgr) const
{
	if (!enemyMgr) {
		return;
	}

	for (auto& enemy : enemyMgr->GetEnemies()) {
		enemy.SetHighlight(false);
	}
}

void CardTargetingController::ApplyHoverHighlight(EnemyManager* enemyMgr, int hoverIndex) const
{
	if (!IsValidTarget(enemyMgr, hoverIndex)) {
		return;
	}

	enemyMgr->GetEnemies()[hoverIndex].SetHighlight(true);
}

bool CardTargetingController::IsValidTarget(EnemyManager* enemyMgr, int index) const
{
	if (!enemyMgr || index < 0) {
		return false;
	}

	const auto& enemies = enemyMgr->GetEnemies();
	return index < static_cast<int>(enemies.size()) && enemies[index].IsAlive();
}
