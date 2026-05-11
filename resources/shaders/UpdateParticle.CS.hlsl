struct Particle {
    float3 translate;
    float3 scale;
    float lifeTime;
    float3 velocity;
    float currentTime;
    float4 color;
};

struct EmitterData {
    float3 translate; // 位置
    float radius; // 射出半径
    
    uint count; // 射出数
    float frequency; // 射出間隔
    float frequencyTime; // 射出間隔調整用時間
    uint emit; // 射出許可

    float3 velocityBase; // 基本速度
    float velocityVariance; // 速度分散

    float lifeTimeMin; // 最小寿命
    float lifeTimeMax; // 最大寿命
    uint shapeType;
    float shapeAngle;

    float3 shapeSize;
    float padding1;

    float3 acceleration; // 加速度
    float padding2;

    float4 startColor; // 発生時の色
    float4 endColor; // 消滅時の色
};

ConstantBuffer<EmitterData> gEmitter : register(b0);

static const uint kMaxParticles = 10000;
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

struct PerFrame {
    float time;
    float deltaTime;
};
ConstantBuffer<PerFrame> gPerFrame : register(b1);

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    uint particleIndex = DTid.x;
    if (particleIndex < kMaxParticles) {
        // lifeTimeが0より大きい場合、生きているとみなす
        if (gParticles[particleIndex].lifeTime > 0.0f) {
            gParticles[particleIndex].currentTime += gPerFrame.deltaTime;

            if (gParticles[particleIndex].currentTime >= gParticles[particleIndex].lifeTime) {
                // 寿命が来たので死ぬ
                gParticles[particleIndex].lifeTime = 0.0f; // 死んだマーク
                gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);
                
                int freeListIndex;
                InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);
                if ((freeListIndex + 1) < kMaxParticles) {
                    gFreeList[freeListIndex + 1] = particleIndex;
                } else {
                    InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
                }
            } else {
                // 生きているので更新
                gParticles[particleIndex].velocity += gEmitter.acceleration * gPerFrame.deltaTime;
                gParticles[particleIndex].translate += gParticles[particleIndex].velocity;
                
                // 色の補間（startColor と endColor の間でLerp）
                float t = saturate(gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime);
                gParticles[particleIndex].color = lerp(gEmitter.startColor, gEmitter.endColor, t);
            }
        }
    }
}
