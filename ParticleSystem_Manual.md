# GPU Particle System & Editor マニュアル

## 1. 概要
このシステムは、コンピュートシェーダを用いた高速なGPUパーティクルシステムです。
専用の `ParticleEditor` を使ってエフェクトを作成・調整し、JSONファイルとして保存。ゲーム内ではそれを自動で読み込み、1行のコードで発生させることができます。
現在、最大描画数は 1グループにつき **10,000個** まで対応しています。

---

## 2. エディターの使い方 (ParticleEditor)

### エフェクトの作成
1. **Create New Particle** 
   - `Group Name` にパーティクルの要素名（例: "Core", "Ring", "Sparks" など）を入力し、`Create` を押します。
   - 作成したグループが下のリストに追加されます。炎なら「中心の火の玉」「舞い散る火の粉」のように、複数のグループを組み合わせて1つの「エフェクト」を作ります。

### パラメータの調整
各グループ（ツリー）を開くと、以下の設定が可能です。

* **Blend Mode**: 加算(Add)、乗算(Multiply)、通常(Normal)などの合成方法。光るエフェクトには `Add` が適しています。
* **Billboard Mode**: 
  * `Billboard`: 常にカメラの方を向く（標準的なパーティクル）
  * `Velocity Aligned`: 飛んでいく方向を向く（火の粉や矢など）
  * `None`: 回転せず固定される
* **Model**: デフォルトの板ポリ（Default Plane）、またはカスタムOBJモデル、プリミティブ形状を選択できます。
* **Texture**: `Resources` フォルダ内から画像を選択できます。未選択の場合は白画像になります。

#### Emission Settings (発生設定)
* **Auto Emit**: チェックを入れると自動で発生し続けます（エディタでの確認用）。**ゲームで任意のタイミングで出す場合は、チェックを外して保存してください。**
* **Count**: 1回に発生するパーティクルの数。
* **Frequency**: `Auto Emit` がオンの時の、発生間隔（秒）。

#### Shape Settings (発生形状)
* `Sphere`(球), `Cone`(円錐), `Box`(箱) から発生範囲を選びます。
* 形状に合わせて `Radius`（半径）や `Angle`（広がり角）を設定できます。

#### Particle Settings (個別の挙動)
* **LifeTime (Min/Max)**: パーティクルが消えるまでの時間（秒）。
* **Velocity Base**: 飛んでいく基本の速度・方向。
* **Velocity Variance**: 速度のばらつき（ランダム性）。
* **Acceleration (Gravity)**: 加速度。下に落としたい場合はYをマイナスにします（例: 0, -9.8, 0）。
* **Start Color / End Color**: 発生時と消滅時の色。時間経過でなめらかに変化します。透明にしたい場合はアルファ（A）を0にします。

### セーブとロード
* **保存**: `File Name` にファイル名（例: `fire_magic.json`）を入力し、`Save Particles` を押します。
  * 保存先は自動的に `Resources/Particles/` フォルダ内になります。
* **読み込み**: `Saved JSONs` のリストからファイルを選び、`Load Particles` を押すと読み込まれます。
* **一括操作**: `Batch Controls` にある `All Auto Emit OFF` を押すと、全パーティクルの自動発生をまとめて止めることができます。保存前の準備に便利です。

---

## 3. ゲームへの組み込み方

### 自動ロードの仕組み
ゲームの起動時（`GameApp::WarmupAssets_` 等）に以下の関数が呼ばれています。
```cpp
ParticleManager::GetInstance()->LoadAllEffects();
```
これにより、`Resources/Particles/` フォルダ内に保存されたすべての `.json` ファイルが、**全自動でメモリに読み込まれます。** 自分でロード処理を書く必要はありません。

### ゲーム内でエフェクトを発生させる方法
自動ロードされたエフェクトは、**「ファイル名から .json を抜いた名前」** が「エフェクト名」になります。
（例： `fire_magic.json` ➔ エフェクト名: `"fire_magic"` ）

敵にヒットした時や、クリックした時のコード内に、以下のように1行記述するだけで発生します。

```cpp
// 例: fire_magic エフェクトを enemyPos の位置に出す
ParticleManager::GetInstance()->EmitEffect("fire_magic", enemyPos);
```

これだけで、JSONで設定した複数のパーティクルグループが一斉にその場所で発生します。
