struct Particle
{
    float3 position;
    float currentTime;
    float3 velocity;
    float lifeTime;
    float3 acceleration;
    float startScale;
    float4 startColor;
    float4 endColor;
    float endScale;
    uint isActive;
    float2 padding0; // ← ここを追加
    float3 rotate;
    float padding1;
    float3 angularVelocity;
    float padding2;
};

struct RenderData
{
    float4x4 WVP;
    float4x4 World;
    float4x4 WorldInverseTranspose;
    float4 color;
};

// バッファ
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<RenderData> gRenderData : register(u1); // 描画用

// 描画対象のインデックスを保持するバッファ
RWStructuredBuffer<uint> gAliveIndices : register(u2);
// ExecuteIndirect用の引数バッファ (InstanceCountを書き換える)
RWByteAddressBuffer gDrawArgs : register(u3);

// 定数
struct GlobalConfig
{
    float deltaTime;
    uint maxParticles;
};
struct SceneConfig
{
    float4x4 viewProjection;
};

ConstantBuffer<GlobalConfig> gConfig : register(b0);
ConstantBuffer<SceneConfig> gScene : register(b1);

// --- 行列生成関数 ---
float4x4 MakeAffineMatrix(float3 scale, float3 rotate, float3 translate)
{
    // スケール行列
    float4x4 mScale =
    {
        scale.x, 0, 0, 0,
        0, scale.y, 0, 0,
        0, 0, scale.z, 0,
        0, 0, 0, 1
    };

    // 回転行列 (XYZ順)
    float3 s = sin(rotate);
    float3 c = cos(rotate);
    
    float4x4 mRotateX =
    {
        1, 0, 0, 0,
        0, c.x, s.x, 0,
        0, -s.x, c.x, 0,
        0, 0, 0, 1
    };
    float4x4 mRotateY =
    {
        c.y, 0, -s.y, 0,
        0, 1, 0, 0,
        s.y, 0, c.y, 0,
        0, 0, 0, 1
    };
    float4x4 mRotateZ =
    {
        c.z, s.z, 0, 0,
        -s.z, c.z, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    float4x4 mRotate = mul(mRotateX, mul(mRotateY, mRotateZ));

    // 平行移動行列
    float4x4 mTranslate =
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        translate.x, translate.y, translate.z, 1
    };

    return mul(mScale, mul(mRotate, mTranslate));
}

[numthreads(1024, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // 1. 範囲外チェック
    if (DTid.x >= gConfig.maxParticles)
        return;

    // 2. アクティブチェック
    if (gParticles[DTid.x].isActive == 0)
        return;

    Particle p = gParticles[DTid.x];

    // 3. 寿命更新と終了判定
    p.currentTime += gConfig.deltaTime;
    if (p.currentTime >= p.lifeTime)
    {
        p.isActive = 0;
        gParticles[DTid.x] = p;
        // ここでは gRenderData への書き込みは不要（描画リストに入れないため）
        return;
    }

    // 4. 物理挙動の計算
    float t = p.currentTime / p.lifeTime;
    p.velocity += p.acceleration * gConfig.deltaTime;
    p.position += p.velocity * gConfig.deltaTime;
    p.rotate += p.angularVelocity * gConfig.deltaTime;

    // 5. 演出パラメータ計算
    float currentScale = lerp(p.startScale, p.endScale, t);
    float4 currentColor = lerp(p.startColor, p.endColor, t);
    currentColor.a *= (1.0f - t);

    // 6. 行列生成
    float4x4 world = MakeAffineMatrix(float3(currentScale, currentScale, currentScale), p.rotate, p.position);
    
    // --- ここからが「間接描画」のための重要処理 ---

    // 7. 生存カウンタをインクリメントして、書き込み先のインデックスを取得
    // gDrawArgs (RWByteAddressBuffer) の 4バイト目 (InstanceCount) を +1 する
    uint drawIndex;
    gDrawArgs.InterlockedAdd(4, 1, drawIndex);

    // 8. 取得した drawIndex 番目に描画データを詰めて書き込む
    // 注意：DTid.x ではなく drawIndex を使うことで、バッファの先頭から生存分が並ぶ
    gRenderData[drawIndex].World = world;
    gRenderData[drawIndex].WVP = mul(world, gScene.viewProjection);
    gRenderData[drawIndex].WorldInverseTranspose = transpose(world);
    gRenderData[drawIndex].color = currentColor;

    // (オプション) 生存している元のパーティクルIDを保持しておきたい場合
    gAliveIndices[drawIndex] = DTid.x;

    // 9. パーティクル状態を保存
    gParticles[DTid.x] = p;
}