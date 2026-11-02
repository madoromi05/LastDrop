タイトル案：G-Buffer の作り方と意外な落とし穴（DirectX11 / 自作エンジン）

---

G-Buffer は、一言でいえば**「色を出す前の面の情報を、画面 1 枚ぶん貯めておく描画先」**です。
自作の DirectX11 エンジンで作っている FPS ゾンビゲームを、フォワードからディファードへ切り替えたときに G-Buffer を作りました。
この記事では、**何をどの形式で入れたか・既存の描画コードをどう切り替えたか・ハマった不具合**の 3 点に絞って整理します。

### 1. G-Buffer とは何か

#### フォワードとディファードの違い

| 描き方 | やること | 向いている場面 |
|---|---|---|
| フォワード | 物を描いたその場でライトを全部計算し、色を出す | ライトが少ない・半透明が多い |
| ディファード | まず面の情報（色・法線・材質）だけを G-Buffer に書き、後で画面 1 枚に対してまとめてライトを計算する | ライトが多い・SSAO や SSR など画面全体の処理を足したい |

ディファードでは、描く処理が 2 段に分かれます。

1. **ジオメトリのパス**：不透明な物を全部描き、面の情報を G-Buffer へ書く
2. **ライティングのパス**：全画面の板を 1 枚描き、画素ごとに G-Buffer を読んでライトを計算する

このとき 1 と 2 をつなぐ「受け渡しの箱」が G-Buffer です。

#### なぜ入れたか

ゲームは夜の町が舞台で、懐中電灯・街灯・マズルフラッシュとライトが増えていく予定でした。
さらに SSAO・SSR・影も入れたかったので、**画面全体の情報（法線・深度・材質）がそろっている状態**を先に作っておく必要がありました。

> この段階の目標は「切り替える前と同じ絵」だけ。速くするためではなく、後の処理の土台として入れた。

見た目を変えずに切り替えれば、絵が変わったときに「G-Buffer のせいか、新しいライトのせいか」を分けられます。

### 2. G-Buffer に何を入れたか

#### 並び

最終的な並びはこうなりました。

| 描画先 | 形式 | r | g | b | a |
|---|---|---|---|---|---|
| RT0 | RGBA8_SRGB | 色 | 色 | 色 | 予備 |
| RT1 | RGBA16F | 法線 x | 法線 y | 法線 z | 動きの種類（TAA 用） |
| RT2 | RGBA8 | AO | 粗さ | 金属 | 発光の強さ |
| 深度 | ― | ポストプロセスの深度を共有 | | | |

**位置は持たず、深度から戻します**（4 章）。

#### 形式を選んだ理由

| 描画先 | 形式 | 理由 |
|---|---|---|
| 色 | RGBA8_**SRGB** | リニアの値を 8bit にそのまま入れると、暗い色に段差が出る |
| 法線 | RGBA16F | -1〜1 の値で、8bit では精度が足りない |
| 材質 | RGBA8（sRGB にしない） | AO・粗さ・金属はどれも 0〜1 の「数」で、色ではない |

ここで一つ、**sRGB にしないと値が消える**例がありました。
最初の版では RT2 にスペキュラの色（0.02）を入れていました。これをリニアに直すと約 0.0015 です。

```
0.0015 × 255 ≒ 0.4  →  8bit に丸めると 0
```

UNORM8 にリニアのまま入れると、**スペキュラが丸ごと 0 になります**。
sRGB 形式にすると GPU が書くときに sRGB へ変換し、読むときにリニアへ戻してくれるので、暗い値ほど細かく残せます。

> 8bit の描画先に「小さい値」を入れるときは、形式で精度の配り方が変わることを意識する。

#### 並びは 1 か所にだけ書く

書く側（各メッシュのシェーダ）と読む側（ライティング）が同じ並びを知っている必要があります。
そこで並びと書き込みの関数を `GBuffer.hlsli` に 1 か所だけ置き、両方から include しました。

```hlsl
// G-Buffer の並び。書く側と読む側が、並びをここだけで知る。
//   RT0  RGBA8_SRGB : rgb 色  a 予備
//   RT1  RGBA16F    : xyz 法線（ワールド・長さ 1）  a 動きの種類
//   RT2  RGBA8      : r AO  g 粗さ  b 金属  a 発光の強さ
struct GBufferOutput
{
    float4 albedo   : SV_Target0;
    float4 normal   : SV_Target1;
    float4 material : SV_Target2;
};

GBufferOutput writeGBuffer(float4 color, float2 uv, float3 worldPosition, float3 worldNormal, bool isFrontFace)
{
    float3 geometric = normalize(worldNormal);
    geometric = isFrontFace ? geometric : -geometric; // 裏面は見ている側へ法線を向ける
    float3 n = perturbNormal(geometric, worldPosition, uv); // 法線マップ
    float3 albedo = (color * gAlbedo.Sample(gWrapSampler, uv)).rgb;
    float3 orm = gOrmMap.Sample(gWrapSampler, uv).rgb;

    GBufferOutput o;
    o.albedo   = float4(albedo, kSpare);
    o.normal   = float4(n, gMotion.x);
    o.material = float4(orm.r, saturate(gSurface.x * orm.g), saturate(gSurface.y * orm.b), saturate(gSurface.z));
    return o;
}

// 読む側。番号は C++ 側と揃える。
Texture2D        gGBufferAlbedo   : register(t2);
Texture2D        gGBufferNormal   : register(t3);
Texture2D        gGBufferMaterial : register(t4);
Texture2D<float> gSceneDepth      : register(t5);
```

C++ 側の形式の配列も「並びは GBuffer.hlsli と揃える」とコメントで入口を示しておき、`static_assert` で数のずれを止めています。

```cpp
const Graphics::RenderTargetFormat kFormats[] = {
	Graphics::RENDER_TARGET_RGBA8_SRGB, // 色。8bit にリニアで入れると暗い色に段差が出るので sRGB
	Graphics::RENDER_TARGET_RGBA16F,    // 法線（-1〜1）。8bit に入らない数なので浮動小数
	Graphics::RENDER_TARGET_RGBA8,      // AO・粗さ・金属・発光（どれも 0〜1。色ではないので sRGB にしない）
};
static_assert( sizeof( kFormats ) / sizeof( kFormats[ 0 ] ) == kTargetNumber, "GBuffer : one format per target." );
```

### 3. 既存の描画コードをどう切り替えたか

一番悩んだのはここです。建物・地面・ゾンビ・自キャラ・銃と、`draw` を呼んでいる所は 10 か所以上ありました。

#### × 全部の draw に引数を足す

```cpp
// × 悪い例：呼ぶ側が全員「今どこへ描いているか」を知る必要がある
building.draw( isGBuffer );
zombie.draw( isGBuffer );
```

これだと影のパスを足すたびに、また全部の呼び出しを直すことになります。

#### ○ 「今のパス」を 1 つ置き、シェーダを選ぶ所だけが読む

```cpp
// 今どの描き方で物を描いているか。
// 変えるのは GBuffer::begin / end（と ShadowMap::begin / end）だけ。
// 読むのはシェーダを選ぶ 2 か所（StaticMeshPart::draw・SkinningPipeline::begin）だけで、
// 建物・ゾンビなどの draw は、どちらで描かれているかを知らない。
enum class RenderPassKind{
	Forward,
	GBuffer,
	Shadow,
};
```

使う側はこうなります。

```cpp
gBuffer.begin( depth );   // ここで RenderPass が GBuffer になる
world.drawOpaque();       // いつもどおり描くだけ
gBuffer.end();            // Forward に戻る
deferredLighting.drawLighting( gBuffer );
```

#### 迷った所：どこでシェーダを選ぶか

最初は材質を設定する `MeshMaterial::apply` で G-Buffer 用シェーダに切り替えようとしました。
ところがキャラ（スキニングするメッシュ）も `apply` を通るので、**キャラのシェーダを静的メッシュ用で上書きしてしまいます**。
そこで「静的メッシュの入口」と「スキニングの入口」の 2 か所で選ぶ形にしました。

> 切り替えは「全員が通る所」ではなく「種類ごとの入口」に置く。

この形にしておいたおかげで、後から影のパス（`Shadow`）を足したときも、変えたのは同じ 2 か所だけでした。

### 4. 位置は深度から戻す

G-Buffer に位置（xyz）を持たせると、16F × 3 チャンネルぶん帯域を食います。
深度はもともと持っているので、**画面の uv と深度から、射影×ビューの逆行列で位置を戻します**。

```hlsl
// 画面の位置（uv）と深度から、ワールドの位置へ戻す（射影×ビューの逆を掛けて w で割る）。
float3 worldPositionFromDepth(float4 inverseProjectionView[4], float2 uv, float depth)
{
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0); // uv は下向き、クリップ空間は上向き
    float4 world = float4(dot(inverseProjectionView[0], clip), dot(inverseProjectionView[1], clip),
                          dot(inverseProjectionView[2], clip), dot(inverseProjectionView[3], clip));
    return world.xyz / world.w;
}
```

自作の数学ライブラリには 4×4 の逆行列が無かったので、`Matrix44::setInverse` を足しました。
この関数は SSAO・霧でも同じものを使っています。

#### 何も描かれていない画素

深度が 1（消した値）の画素は、G-Buffer の中身を読まずに、ライティングのパスで最初に分けます。
最初の版では `discard` してポストプロセスが消した色を残していました。「消した色」を G-Buffer 側にも書くと、消す色を決める場所が 2 か所になってしまうためです。
今はここで空のグラデーションを出しています。

### 5. 注意点① ブレンドの設定が残っていて画面が真っ暗

切り替えた直後、**UI は見えるのに 3D が真っ黒**になりました。

#### 切り分け

1. ライティングの結果を「NaN → 紫、0 → 緑」で出す → **全部緑（ちょうど 0）**
2. 画面を 3 分割して「G-Buffer の色 / ライトの値 / 法線」を出す
   - 色：**0**
   - ライト：正常
   - 法線：**濃すぎる**

#### 原因

G-Buffer へ描くとき、ブレンドが**前に描いた物（半透明の板・UI）の「アルファで混ぜる」設定のまま**でした。

| 描画先 | a に入っていた値 | 結果 |
|---|---|---|
| RT0（色） | 予備の 0 | アルファ 0 で混ぜる → **何も書かれない** |
| RT1（法線） | 当時は鋭さ（8〜48） | アルファ倍される → **法線が何倍にも** |

フォワードのときはシェーダが画像のアルファ（ほぼ 1）を出していたので、ブレンドが残っていても気づきませんでした。

#### 修正

```cpp
void GBuffer::begin( Graphics::DepthTarget depth ) {
	Graphics::Manager gm = Graphics::Manager::instance();
	gm.setRenderTargets( mTargets, kTargetNumber, depth );
	// ...
	// G-Buffer のアルファは色の透明度ではなくデータなので、混ぜずにそのまま書く。
	// 前に描いた物のブレンドが残っていると、アルファ 0 の RT0 は何も書かれない。
	gm.setBlendMode( Graphics::BLEND_OPAQUE );
	// ...
}
```

> 描画先を切り替える所では、ブレンドも自分で設定する。アルファにデータを入れる描画先は特に。

### 6. 注意点② 描画先を付け直すと画像が外れる

後にデカール（弾痕など）を足したとき、G-Buffer の途中で描画先を一度付け直す処理を入れました。
DirectX11 では、描画先として付けたものを同時に読み込み元にはできないので、付け直すと**読み込み元の画像も外れます**。

ここで材質の画像（粗さ・金属）が外れたままになり、後から描いた自キャラやゾンビが「粗さ 0（つるつる）・AO 0」で描かれ、街灯のハイライトが金色に光って見える不具合が出ました。
「前と同じ画像なら付け直さない」という最適化が裏目に出た形です。

```cpp
void GBuffer::endDecals( Graphics::DepthTarget depth ) {
	// ...
	gm.setRenderTargets( mTargets, kTargetNumber, depth );
	gm.setBlendMode( Graphics::BLEND_OPAQUE ); // begin と同じ
	// 付け直しで材質の画像も外れている。「前と同じなら付けない」を忘れさせ、次の物で付け直させる。
	RenderPass::forgetSentSurface();
}
```

> 「付け直さない」最適化を入れたら、「外れる」タイミングを全部洗い出す。

### 7. 中身を自分の目で確かめる

G-Buffer はそのままでは見えないので、デバッグ用のエディタ（ImGui）から**中身を画面に出す表示**を最初に作りました。

| 表示 | 確認したいこと |
|---|---|
| Albedo | 色が暗くなっていないか（sRGB の変換が二重になっていないか） |
| Normal | 法線マップが効いているか・裏面の向き |
| Material | 粗さ・金属が材質どおりか |
| Depth / Position | 深度から戻した位置が、ワールドの格子とずれていないか |

確認したいのは 2 点です。

- **表示は ToneMap を通さない**：通すと数値が歪んで、正しいかどうか判断できません。ポストプロセスの後に画面へ直接写しています。
- **画素は uv × 大きさで読む**：写す先のビューポートが G-Buffer とずれることがあるので、`SV_Position` ではなく uv から画素を求めます。

5 章の真っ暗の不具合も、この「中身を出す」考え方で切り分けられました。

### 8. 速くなったのか

Profile ビルド・VSync なしで、切り替え前後の GPU 時間を測りました。

| 項目 | フォワード | ディファード |
|---|---|---|
| GPU 全体（中央値） | 0.340ms | 0.345ms |
| 増えた固定の仕事 | ― | 約 +0.09ms（G-Buffer 3 枚への書き込み＋ライティングのパス 0.056ms） |

当時のライトは 4 つだったので、**速くも遅くもならない**という結果でした。
ディファードはライトが増えるほど得をする描き方なので、この規模では「土台を入れた」と捉えています。

測るときに気を付けたこと：

- **Debug ビルドでは測らない**：シェーダが最適化なしでコンパイルされ、ゾンビの描画が 1.37ms に見えていました。
- 前後は別々の起動なので、映っている物の数は揃っていません。ゾンビの描画時間が下がったのは、描き方ではなく映っていた数の差と見ています。

### 9. まとめ

> G-Buffer は「何を・どの形式で・どこに書くか」を 1 か所に決め、既存の描画コードには「今のパス」以外を知らせない。

- **影響範囲**：G-Buffer へ描くかどうかを読むのは、シェーダを選ぶ 2 か所だけ。建物・ゾンビなどの `draw` は変えていない。
- **修正の入口**：並びを変えるときは `GBuffer.hlsli`（シェーダ）と `GBuffer.cpp` の形式の配列の 2 か所。
- **設計意図**：見た目を変えずに切り替え、後の SSAO・影・SSR などの「画面全体の処理」の土台にする。

#### チェックリスト

- 8bit の描画先に小さい値を、リニアのまま入れていないか
- 色ではない数（粗さ・金属）を sRGB 形式にしていないか
- G-Buffer へ描く前に、ブレンドを不透明に戻しているか
- 描画先を付け直した後、読み込み元の画像を付け直しているか
- 中身を確かめる表示に ToneMap を通していないか
- Debug ビルドの数字で速さを判断していないか

### 参考記事

- [Microsoft Learn「DXGI_FORMAT」](https://learn.microsoft.com/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format)
- [LearnOpenGL「Deferred Shading」](https://learnopengl.com/Advanced-Lighting/Deferred-Shading)
- [MJP「Reconstructing Position From Depth」](https://mynameismjp.wordpress.com/2009/03/10/reconstructing-position-from-depth/)
- [80.lv「Escape from Tarkov Game Tech Overview」](https://80.lv/articles/escape-from-tarkov-game-tech-overview/)

#DirectX11 #C++ #HLSL #ゲーム開発
