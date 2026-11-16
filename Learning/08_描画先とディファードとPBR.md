# 08 描画先・ディファード・PBR

ここから先の技法（ディファード・影・ブルーム・SSAO…）は、全部**同じ 2 つの部品**でできている。

1. **テクスチャへ描く**（バックバッファではなく、自分で作ったテクスチャを描画先にする）
2. **全画面を 1 回描く**（1 で描いたテクスチャを読んで、画素ごとに計算する）

この 2 つを自分の手で書ければ、残りは「何を描いて、何を読むか」の違いでしかない。

## この章でできるようになること

- 描画先（レンダーターゲット）を作り、そこへ描き、それをテクスチャとして読める
- 全画面を三角形 1 枚で描ける
- G-Buffer に何を入れ、ライティングのパスでどう使うかを説明できる
- PBR の式の D・G・F が、それぞれ何を表すか言える

## 先に読む

- LearnOpenGL「[Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers)」：テクスチャへ描く考え方（API は違うが図が良い）
- LearnOpenGL「[Deferred Shading](https://learnopengl.com/Advanced-Lighting/Deferred-Shading)」：G-Buffer とライティングのパス
- LearnOpenGL「[PBR / Theory](https://learnopengl.com/PBR/Theory)」・「[PBR / Lighting](https://learnopengl.com/PBR/Lighting)」：D・G・F の説明。**PBR はまずこれ**
- LearnOpenGL「[HDR](https://learnopengl.com/Advanced-Lighting/HDR)」：1 を超える明るさとトーンマップ
- Brian Karis「[Real Shading in Unreal Engine 4](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)」（英語・PDF）：自分の `Pbr.hlsli` が参照している資料。LearnOpenGL の後に読む
- Krzysztof Narkowicz「[ACES Filmic Tone Mapping Curve](https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/)」：`Present.hlsl` のトーンマップの式の元
- 自分の記録：`SummaryPlan/個別仕様/グラフィック/GBuffer_Plan.txt`・`PbrNightLighting_Plan.txt`・`LinearToneMap_Plan.txt`、`SummaryPlan/計測の記録.md` の⑥⑦

## 言葉の整理

| 言葉 | 中身 |
|---|---|
| 描画先（レンダーターゲット） | 描く先にできるテクスチャ。`D3D11_BIND_RENDER_TARGET \| D3D11_BIND_SHADER_RESOURCE` で作り、RTV（描く用）と SRV（読む用）の 2 つのビューを持つ |
| MRT | 描画先を同時に何枚も付けること。ピクセルシェーダが `SV_Target0`・`SV_Target1`…へ別々の値を出す |
| フォワード | 物を描くときに、その場でライティングまで計算するやり方（06 章の内蔵シェーダ） |
| ディファード | 物を描くときは「面の情報」（色・法線・材質）だけを G-Buffer に書き、後から全画面 1 回でライティングするやり方 |
| G-Buffer | 面の情報を入れた何枚かのテクスチャ |
| HDR | 色を 0〜1 に切らず、光の量のまま持つこと（`RGBA16F`）。最後にトーンマップで 0〜1 に収める |

### フォワードとディファード

| | フォワード | ディファード |
|---|---|---|
| ライティングの回数 | 描いた画素の数 × ライトの数（後ろに隠れる面も計算する） | 画面の画素の数 × ライトの数（見えている面だけ） |
| ライトが多いとき | 重い | 強い |
| 半透明 | そのまま描ける | 苦手（G-Buffer は 1 画素に 1 つの面しか持てない）。半透明だけ後からフォワードで描く |
| MSAA | 使える | 使いにくい（だから FXAA・TAA） |
| メモリ | 少ない | G-Buffer の分だけ多い |
| 画面全体の効果 | 作りにくい | 法線・深度が残っているので SSAO・SSR・デカールが作りやすい |

このゲームは夜の町で、月・懐中電灯・銃口の光・街灯と、同時に照らすライトが十数個あるので、ディファードにした（技術まとめ 5-1・計測の記録⑥）。

## 自分のコードのどこか

### 2 つの部品

| 見る物 | ファイル | 名前 |
|---|---|---|
| 描画先を作る | [RenderTargetImpl.h](../LastDrop/GameLib/src/Graphics/RenderTargetImpl.h) | `createDxObjects` |
| 深度の描画先（シェーダからも読める） | [DepthTargetImpl.h](../LastDrop/GameLib/src/Graphics/DepthTargetImpl.h) | `R32_TYPELESS`・`D32_FLOAT`・`R32_FLOAT`・`mReadOnlyDSV` |
| 描画先を付ける | [ManagerImpl.h](../LastDrop/GameLib/src/Graphics/ManagerImpl.h) | `setRenderTargets`・`unbindUserShaderResources`・`resetRenderTargets` |
| 描画先をテクスチャとして読む | 同上 | `setShaderResource` |
| 全画面を描く（C++） | 同上 | `drawFullScreen` |
| 全画面を描く（HLSL） | [Common.hlsli](../LastDrop/Resource/Shader/Common.hlsli) | `fullScreenVertex` |

### ディファードと PBR

| 見る物 | ファイル | 名前 |
|---|---|---|
| 全部の順番 | [SceneRenderer.cpp](../LastDrop/GameSrc/src/Scene/Game/World/SceneRenderer.cpp) | `render` |
| G-Buffer の並び（ただ 1 か所の決まり） | [GBuffer.hlsli](../LastDrop/Resource/Shader/GBuffer.hlsli) | 先頭のコメント・`GBufferOutput`・`writeGBuffer` |
| G-Buffer を作る・付ける | [GBuffer.cpp](../LastDrop/GameSrc/src/Render/Lighting/GBuffer.cpp) | `kFormats`・`begin`・`end` |
| G-Buffer へ書くシェーダ | [StaticMeshGBuffer.hlsl](../LastDrop/Resource/Shader/StaticMeshGBuffer.hlsl) | `VSMain`・`PSMain` |
| 今どのパスか | [RenderPass.cpp](../LastDrop/GameSrc/src/Render/RenderPass.cpp) | `beginGBuffer`・`setSurface` |
| ライティングのパス（C++） | [DeferredLighting.cpp](../LastDrop/GameSrc/src/Render/Lighting/DeferredLighting.cpp) | `drawLighting` |
| ライティングのパス（HLSL） | [DeferredLighting.hlsl](../LastDrop/Resource/Shader/DeferredLighting.hlsl) | `PSMain` |
| 深度から位置を戻す | [DepthPosition.hlsli](../LastDrop/Resource/Shader/DepthPosition.hlsli) | `worldPositionFromDepth` |
| PBR の式 | [Pbr.hlsli](../LastDrop/Resource/Shader/Pbr.hlsli) | `surfaceBrdf`・`distributionGgx`・`geometrySmith`・`fresnelSchlick`・`evaluateLight`・`shadeSurface` |
| ライトの一覧 | [LightList.cpp](../LastDrop/GameSrc/src/Render/Lighting/LightList.cpp)・[LightList.hlsli](../LastDrop/Resource/Shader/LightList.hlsli) | 一度に照らせる数の上限は `kMaxLights`（C++ とシェーダの両方にある。04 章の課題 04-5） |
| ライトを集める | [SceneLights.cpp](../LastDrop/GameSrc/src/Scene/Game/World/SceneLights.cpp) | `setup`・`addMoon`・`addObjectLights`（物に付いた `LightComponent`・`MuzzleFlashComponent` を全部たどる。10 章） |
| シーンの色（HDR）と画面へ写す所 | [PostProcess.cpp](../LastDrop/GameSrc/src/Render/PostProcess/PostProcess.cpp)・[Present.hlsl](../LastDrop/Resource/Shader/Present.hlsl) | `beginScene`・`endScene`・`toneMapAces` |
| デカール（弾痕） | [DecalBatch.cpp](../LastDrop/GameSrc/src/Render/DecalBatch.cpp)・[Decal.hlsl](../LastDrop/Resource/Shader/Decal.hlsl) | G-Buffer の色と法線へ上書き |

## 読むときの問い

### 描画先

1. `RenderTargetImpl.h` の `createDxObjects` は、1 枚のテクスチャから `mRTV` と `mSRV` の 2 つのビューを作っている。なぜ 2 つ要るか
2. `ManagerImpl.h` の `setRenderTargets` は、最初に `unbindUserShaderResources()` を呼んで、t2 以降を全部外している。外さないと何が起きるか
3. そのせいで、[GBuffer.cpp](../LastDrop/GameSrc/src/Render/Lighting/GBuffer.cpp) の `endDecals` には「材質の画像（t11）も外れている」ことから起きた不具合の話が書いてある。何が起きて、どう直したか。これは 05 章のどの話と同じ種類か
4. `DepthTargetImpl.h` は、テクスチャを `R32_TYPELESS`、DSV を `D32_FLOAT`、SRV を `R32_FLOAT` で作っている。なぜ型を決めない形式（TYPELESS）で作るのか
5. `mReadOnlyDSV`（読み取り専用の DSV）は何のためにあるか。`SceneRenderer::render` のどこで使っているか
6. `setRenderTargets` は、描画先を付けた後に `RSSetViewports` もしている。なぜ要るか（SSAO は画面の半分の大きさ）

<details><summary>答え</summary>

1. 同じテクスチャを、あるときは「描く先」として、別のときは「読むテクスチャ」として使うから。ビューは「何として使うか」の札（03 章）
2. D3D11 は、1 つのテクスチャを「読み込み」と「描画先」に同時に付けることを許さない。付けようとすると、D3D11 が読み込み側を勝手に外して警告を出す。黙って外されると、次に読むとき何も付いていない（黒）ので、自分で先に外して「描画先を変えたら付け直す」決まりにしている
3. デカールのために描画先を付け替えた後、材質の画像（t11）が外れたままだった。`RenderPass` は「前と同じ画像なら付けない」ので付け直されず、後から描くキャラは何も無い t11 を読んで粗さ 0（つるつる）になり、街灯のハイライトで金色に光った。`forgetSentSurface` で「前に付けた物」を忘れさせて直した。D3D11 が状態機械であること（と、自前の「同じなら送らない」の覚えがずれること）から来る不具合
4. 1 枚のテクスチャを、深度として書く（D32）のと、数として読む（R32）のと、2 通りの型で使いたいから。型を決めずに作っておき、ビューごとに型を決める
5. 深度を「テストには使うが書かない」とき用。読み取り専用で付けている間は、同じ深度を SRV からも読める。半透明の板を描くとき（`resumeSceneReadOnlyDepth`）。板は深度で隠れつつ、同じ深度を読んで「壁に刺さった所を薄くする」（ソフトパーティクル）
6. ビューポートは描画先の大きさに合わせないといけない。半分の大きさの描画先に、全体の大きさのビューポートのまま描くと、左上の 1/4 しか入らない

</details>

### 全画面の三角形

1. `Common.hlsli` の `fullScreenVertex` は、頂点バッファを使わず `SV_VertexID`（0・1・2）だけから位置と UV を作っている。id が 0・1・2 のとき、`uv` と `position` はそれぞれ何になるか。紙で計算する
2. 四角形（三角形 2 枚）ではなく、画面からはみ出す大きな三角形 1 枚で描くのはなぜか
3. `drawFullScreen` は `IASetInputLayout( nullptr )` し、深度テストとカリングを切ってから `Draw( 3, 0 )` している。それぞれ理由は

<details><summary>答え</summary>

1. id 0：uv ( 0, 0 )・位置 ( −1, 1 )。id 1：uv ( 2, 0 )・位置 ( 3, 1 )。id 2：uv ( 0, 2 )・位置 ( −1, −3 )。画面（−1〜1）を覆い、右と下へはみ出す三角形。はみ出した分は GPU が切る。画面の中では uv がちょうど 0〜1 になる
2. 四角形だと対角線に三角形の境目ができ、境目の画素は 2 回（の一部ずつ）処理される。`ddx`・`ddy` も境目で少し狂う。三角形 1 枚なら境目が無い。頂点バッファも要らない
3. 頂点バッファを読まないので、入力レイアウトは要らない。全画面のパスは「前後」が無いので深度テストは不要（切らないと、残っている深度で捨てられる）。カリングも、向きを気にしなくてよいように切る

</details>

### G-Buffer とライティング

1. `GBuffer.hlsli` の先頭のコメントから、G-Buffer の 3 枚＋深度に何が入っているかを表にする。「位置」が入っていないが、ライティングには位置が要る。どうしているか
2. `StaticMeshGBuffer.hlsl` の `PSMain` の戻り値は `GBufferOutput`（`SV_Target0`〜`2`）。C++ のどの呼び出しが、この 3 つの出力先を決めているか
3. `DeferredLighting.hlsl` の `PSMain` は、`gSceneDepth.Load( pixel )` で深度を読み、`depth >= kFarDepth` なら空を描いている。なぜ深度で「何も描かれていない」と分かるか
4. `SceneRenderer::render` で、半透明の板（砂ぼこり・血・マズルフラッシュ）は、ライティングのパスの**後**に描いている。なぜ G-Buffer に書かないのか
5. デカール（弾痕）は「動かない物 → デカール → 動く物」の順に描いている。この順番で何が実現できているか（コメントにある）
6. `GBuffer.hlsli` に「書く側と読む側が、並びをここだけで知る」とある。並びを 1 か所に書く設計にしないと、何が起きやすいか（04 章の課題を思い出す）

<details><summary>答え</summary>

1. RT0：色（sRGB）。RT1：法線（16 ビット浮動小数）と、a に動きの種類。RT2：AO・粗さ・金属・発光の強さ。深度：`PostProcess` の深度。位置は入れず、深度と画面の uv から、射影ビュー行列の逆行列で戻す（`worldPositionFromDepth`）。位置を入れると 1 枚（12 バイト／画素）余分に要るので、どのエンジンもこうする
2. `GBuffer::begin` の `gm.setRenderTargets( mTargets, kTargetNumber, depth )`（中身は `OMSetRenderTargets( 3, rtvs, dsv )`）。配列の順が `SV_Target` の番号になる
3. 深度は毎フレーム 1（一番奥）で消してある（`clear( DepthTarget )` の `kFarthestDepth`）。何か描かれた画素は 1 より小さい値に書き換わるので、1 のままの画素には何も無い
4. G-Buffer は 1 画素に 1 つの面しか持てない。半透明は「後ろの面」と「手前の板」の 2 つが要るので入らない。だからライティングが済んだ後の色の上へ、フォワード（その場でライティング）で重ねる。板も同じ PBR の式（`evaluateLight`）を通している
5. デカールは「その時点で G-Buffer にある面」へ書く。動かない物の後・動く物の前に描けば、後から描くキャラがデカールを上書きするので、キャラには弾痕が乗らない
6. 書く側と読む側で並びがずれる。ずれても何のエラーも出ず、絵がおかしくなるだけ

</details>

### PBR

`Pbr.hlsli` の `surfaceBrdf` を読む。

1. 鏡面の式は `D × G × F ÷ ( 4 × N・L × N・V )`。D（`distributionGgx`）・G（`geometrySmith`）・F（`fresnelSchlick`）は、それぞれ面の何を表しているか
2. 粗さ（roughness）を 0 に近づけると D はどう変わるか。`kMinRoughness` で下限を付けているのはなぜか
3. 金属（metallic）が 1 のとき、`diffuseColorOf` と `f0Of` は何を返すか。金属と金属でない物は、何が違うのか
4. フレネル（F）は、面を正面から見たときと、真横すれすれから見たときで、どう変わるか。身の回りの例は
5. `surfaceBrdf` の最後は `( 拡散 + π × 鏡面 ) × radiance × nol`。`nol`（N・L）を掛けるのは 06 章のどの式と同じか
6. `viewCosine` は `abs( dot( n, v ) )` にしている。コメントに不具合の話がある。何が起きていたか
7. シーンの色は `RGBA16F`（HDR）。マズルフラッシュの芯の明るさが 10 だとして、8 ビットの描画先（0〜1 で切られる）に描いた場合と比べて、ブルームとトーンマップで何が違ってくるか

<details><summary>答え</summary>

1. 面は、目に見えない細かい鏡（マイクロファセット）の集まりだと考える。D：その鏡のうち、ちょうど「光を目へ反射する向き（H）」を向いている物の割合。G：細かい凹凸どうしが陰になって、光が届かない・目から見えない割合。F：その向きで、光のどれだけが反射するか（残りは中へ入って拡散になる）
2. H の向きにだけ鋭く尖る（鏡に近づく）。点光源のハイライトが 1 画素より小さくなって、動くたびにちらつくので、下限を置く
3. 拡散は 0（黒）、F0 は面の色そのもの。金属は光を中へ通さず全部を表面で反射するので拡散が無く、反射に色が付く。金属でない物は、正面では 4% ほどしか反射せず（`kDielectricF0`）、反射に色は付かない
4. 正面では F0（少ししか反射しない）、すれすれでは 1 に近づく（ほとんど反射する）。水面を真上から見ると底が見え、遠くを見ると空が映る。濡れた路面が遠くで光る
5. ランバートの `max( 0, dot( N, L ) )`。面が光に対して傾くほど、同じ光が広い面積に薄く当たる。拡散にも鏡面にも掛かる
6. 補間した法線や法線マップでは、輪郭の近くで法線が目と反対を向く画素ができる。その N・V を 0 で止めると「真横から見た」扱いになり、フレネルで鏡面が何十倍にもなって、逆光のゾンビの輪郭が光った
7. 8 ビットだと 1 で切られるので、「1 の白」と「10 の光」の区別が無くなる。ブルームは「1 より明るい所」を取り出してにじませるので、HDR なら本当に明るい所だけがにじむ。トーンマップも、10 の光を「色を残したまま白へなだらかに近づける」ことができる

</details>

## 課題

### 課題 08-1：M7

[課題 ミニレンダラ](課題_ミニレンダラ.md) の M7（描画先と全画面のパス）をやる。**この章で一番大事な課題。**

### 課題 08-2：M9

M9（G-Buffer とディファード）をやる。

### 課題 08-3：自分のゲームの G-Buffer を見る

- F6 の Render タブで、G-Buffer の表示を 1 つずつ切り替える（Albedo・Normal・Roughness・Metallic・Emission・Depth・Position）。それぞれ、`GBuffer.hlsli` の並びのどこを表示しているかを言う
- RenderDoc でフレームを撮る。G-Buffer へ描いているドローコールを 1 つ選び、Texture Viewer で Outputs の 3 枚を見る。ライティングのパスのドローコール（頂点 3 つ）を選び、Inputs に G-Buffer の 3 枚と深度が付いているのを見る
- ライティングのパスで、Pipeline State の「頂点バッファ」「入力レイアウト」がどうなっているかを見る

### 課題 08-4：式を 1 つずつ切る

[Pbr.hlsli](../LastDrop/Resource/Shader/Pbr.hlsli) の `surfaceBrdf` で、次を 1 つずつ試して保存する（Debug ならホットリロードで反映）。絵がどう変わるかをメモして、戻す。

- `specular` を 0 にする（拡散だけ）
- 拡散を 0 にする（鏡面だけ）
- F を `f0Of( s )` の定数にする（フレネル無し）
- G を 1 にする
- 粗さを全部 0.1 にする・全部 0.9 にする

### 課題 08-5：消して書き直す

学習用のブランチで、[Common.hlsli](../LastDrop/Resource/Shader/Common.hlsli) の `fullScreenVertex` と、[DepthPosition.hlsli](../LastDrop/Resource/Shader/DepthPosition.hlsli) の `worldPositionFromDepth` の中身を消して書き直す。

## 説明できるか

1. 描画先とは何か。作るのに要るビューは何と何か
2. 同じテクスチャを、読み込みと描画先に同時に付けられないのはなぜか。それで起きた不具合を説明できるか
3. 全画面を三角形 1 枚で描く方法と、四角形にしない理由
4. フォワードとディファードの違い。このゲームがディファードにした理由。ディファードの弱点と、それをどう補っているか
5. G-Buffer に何を入れているか。位置を入れない理由
6. PBR の D・G・F を、それぞれ一言で
7. 金属と金属でない物の違いを、式で言えるか
8. HDR で描いて最後にトーンマップする理由
