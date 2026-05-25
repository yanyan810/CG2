#include "BattleFieldViewController.h"

#include "Camera.h"
#include "Card3D.h"

int BattleFieldViewController::PickFieldIndexByMouse(
	const std::vector<std::unique_ptr<Card3D>>& fieldViews,
	const Camera& camera,
	int mouseX,
	int mouseY,
	float screenWidth,
	float screenHeight)
{
	const Matrix4x4& vp = camera.GetViewProjectionMatrix();

	int best = -1;
	float bestD2 = 80.0f * 80.0f;

	for (int i = 0; i < (int)fieldViews.size(); ++i) {
		Vector3 w = fieldViews[i]->GetWorldPos();

		Vector4 clip{};
		clip.x = w.x * vp.m[0][0] + w.y * vp.m[1][0] + w.z * vp.m[2][0] + 1.0f * vp.m[3][0];
		clip.y = w.x * vp.m[0][1] + w.y * vp.m[1][1] + w.z * vp.m[2][1] + 1.0f * vp.m[3][1];
		clip.z = w.x * vp.m[0][2] + w.y * vp.m[1][2] + w.z * vp.m[2][2] + 1.0f * vp.m[3][2];
		clip.w = w.x * vp.m[0][3] + w.y * vp.m[1][3] + w.z * vp.m[2][3] + 1.0f * vp.m[3][3];

		if (clip.w <= 0.0f) {
			continue;
		}

		const float ndcX = clip.x / clip.w;
		const float ndcY = clip.y / clip.w;

		const float sx = (ndcX * 0.5f + 0.5f) * screenWidth;
		const float sy = (-ndcY * 0.5f + 0.5f) * screenHeight;

		const float dx = sx - (float)mouseX;
		const float dy = sy - (float)mouseY;
		const float d2 = dx * dx + dy * dy;

		if (d2 < bestD2) {
			bestD2 = d2;
			best = i;
		}
	}

	return best;
}