#include "TrailInstance.h"

void TrailInstance::Update(const Vector3& tipPos, const Vector3& basePos, const TrailConfig& config) {
    config_ = config;
    points_.push_front({ tipPos, basePos });

    // Configの制限に合わせて古い点を削除
    while (points_.size() > config_.maxPoints) {
        points_.pop_back();
    }
}