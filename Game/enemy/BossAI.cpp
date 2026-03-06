#include "BossAI.h"
#include "Enemy.h"
#include <algorithm>

void BossAI::Reset(int maxHP) {
    maxHP_ = std::max(1, maxHP);
}

void BossAI::Update(Enemy& e, float dt) {
    // 現在のカードバトルではAIが勝手に動いたり攻撃したりすることは無いので、
    // ここは基本的に空っぽでOKです。
    // 将来、ターンごとの特殊な演出や、死んだ時のエフェクトを入れたい場合はここに書きます。
}