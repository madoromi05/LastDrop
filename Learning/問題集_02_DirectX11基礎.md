# 問題集 02：DirectX11 の基礎

章の教材（03〜05 章）に入る前後の、土台の確認。4 択が 40 問、記述が 14 問。
「三角形を 1 枚出して、動かして、テクスチャを貼る」までに出てくる言葉と API に絞ってある。

## 使い方

1. まず何も見ずに解く。知らない言葉ばかりで手が止まるなら、先に [03 章](03_D3D11の初期化と三角形.md) の「言葉の整理」と「先に読む」を読んでから戻る
2. 4 択は「なぜほかの 3 つが違うか」まで言えたら正解とする
3. 間違えた問題は、答えの最後の「戻る所」を読み直す
4. 記述は、紙かファイルに実際に書く。コードを書く問題は、API の引数を公式ドキュメントで引いてよい（順番と考え方が問題。引数の暗記は要らない）
5. [ミニレンダラ](課題_ミニレンダラ.md) の M2（三角形）を終えた後と、M5（テクスチャ）を終えた後に、もう一度全部解く。**2 回目に満点を取ること**が目標

### 採点の目安

| 4 択の正解 | 次にやること |
|---|---|
| 34 問以上 | 記述へ。ミニレンダラを進める |
| 24〜33 問 | 間違えた節の章（下の表）を読み直してから、ミニレンダラの対応する段へ |
| 23 問以下 | 03 章の「先に読む」の Direct3D11Tutorials（Tutorial01〜03）を、写しながら動かす。その後もう一度 |

| 節 | 戻る章 |
|---|---|
| A. 全体の仕組み | [00 章](00_全体の地図.md)・[03 章](03_D3D11の初期化と三角形.md) |
| B. 頂点とバッファ | [03 章](03_D3D11の初期化と三角形.md)・[06 章](06_メッシュとライティング.md) |
| C. シェーダ・定数バッファ・行列 | [04 章](04_行列とカメラと定数バッファ.md) |
| D. テクスチャ・ビュー・ステート | [05 章](05_テクスチャとステートと色空間.md) |
| E. COM・エラー・デバッグ | [03 章](03_D3D11の初期化と三角形.md)・[01 章](01_Cpp.md) の 3 節 |

---

# 4 択

## A. 全体の仕組み

**Q1.** `ID3D11Device` の主な役割は。

- A. 画面に絵を表示する
- B. バッファ・テクスチャ・シェーダ・ステートなどを**作る**
- C. 三角形を描く命令を出す
- D. ウィンドウを作る

<details><summary>答え</summary>

**B**。デバイスは「作る係」。`CreateBuffer`・`CreateTexture2D`・`CreateVertexShader`・`CreateBlendState` など、`Create〜` はデバイスの関数。

</details>

**Q2.** `ID3D11DeviceContext` の主な役割は。

- A. GPU の種類を調べる
- B. シェーダをコンパイルする
- C. 作った物をパイプラインに設定し、描画の命令を出す
- D. バックバッファを持つ

<details><summary>答え</summary>

**C**。コンテキストは「使う係」。`IASetVertexBuffers`・`VSSetShader`・`OMSetRenderTargets`・`Draw`・`Map` など。シェーダのコンパイルは D3D11 とは別のライブラリ（d3dcompiler）の `D3DCompile`。

</details>

**Q3.** `IDXGISwapChain::Present` を呼ぶと何が起きるか。

- A. バックバッファに描いた絵が、ウィンドウに表示される
- B. バックバッファが消去される
- C. 三角形が描かれる
- D. シェーダが実行される

<details><summary>答え</summary>

**A**。`Draw` は見えない絵（バックバッファ）へ描くだけ。`Present` を呼んで初めて画面に出る。呼び忘れると、何を描いても画面は変わらない。

</details>

**Q4.** パイプラインの段の正しい順は。

- A. 頂点シェーダ → 入力アセンブラ → ピクセルシェーダ → ラスタライザ → 出力マージャ
- B. 入力アセンブラ → ラスタライザ → 頂点シェーダ → ピクセルシェーダ → 出力マージャ
- C. 入力アセンブラ → 頂点シェーダ → ピクセルシェーダ → ラスタライザ → 出力マージャ
- D. 入力アセンブラ → 頂点シェーダ → ラスタライザ → ピクセルシェーダ → 出力マージャ

<details><summary>答え</summary>

**D**。IA（頂点を取り出す）→ VS（頂点を変換する）→ RS（三角形を画素に切り分ける）→ PS（画素の色を決める）→ OM（深度テスト・ブレンドして書く）。「頂点の処理 → 画素に分ける → 画素の処理」の順。API の頭文字（`IASet〜`・`VSSet〜`・`RSSet〜`・`PSSet〜`・`OMSet〜`）がそのまま段の名前。

</details>

**Q5.** 頂点シェーダが必ず出力しないといけない物は。

- A. 頂点の色
- B. クリップ空間の位置（`SV_Position`）
- C. 法線
- D. テクスチャ座標

<details><summary>答え</summary>

**B**。ラスタライザは `SV_Position` を使って、三角形が画面のどこになるかを決める。色・法線・UV は、ピクセルシェーダが要るときだけ渡す。

</details>

**Q6.** ピクセルシェーダは、何 1 つにつき 1 回実行されるか。

- A. 頂点
- B. 三角形
- C. 三角形が覆った画素
- D. フレーム

<details><summary>答え</summary>

**C**。三角形 1 枚が画面の 1 万画素を覆えば、頂点シェーダは 3 回、ピクセルシェーダは約 1 万回走る。だから重い計算をどちらに書くかが、速さに大きく効く。

</details>

**Q7.** ラスタライザの仕事に**含まれない**物は。

- A. テクスチャの色を読む
- B. 三角形を画素に切り分ける
- C. 裏を向いた三角形を捨てる
- D. 頂点の出力（色・UV など）を、画素ごとの値に補間する

<details><summary>答え</summary>

**A**。テクスチャを読むのはピクセルシェーダ。ラスタライザは、切り分け・カリング・画面の外の切り取り・ビューポートへの変換・補間。

</details>

**Q8.** `Draw` を呼んだとき、GPU はどの設定で描くか。

- A. `Draw` の引数で渡した設定
- B. 最初に設定した物
- C. D3D11 の既定の設定
- D. その時点で、コンテキストに設定されている物全部

<details><summary>答え</summary>

**D**。D3D11 は状態機械。`Draw` は「今の状態で描け」という合図で、設定は持たない。前の描画で設定した物は、変えるまで残る。設定のし忘れは「エラー」ではなく「前の設定のまま描かれる」になることが多く、原因が見つけにくい。

</details>

## B. 頂点とバッファ

**Q9.** 入力レイアウト（`ID3D11InputLayout`）が表す物は。

- A. 頂点の数
- B. 頂点バッファの 1 頂点の中身の並び（何バイト目に、どの型の、何のデータがあるか）
- C. 三角形のつなぎ方
- D. 画面の解像度

<details><summary>答え</summary>

**B**。頂点バッファはただのバイトの列。どこが位置でどこが色かは、入力レイアウトが教える。頂点シェーダの入力と合っていないと、作成に失敗するか、でたらめな値が渡る。

</details>

**Q10.** `D3D11_INPUT_ELEMENT_DESC` の `SemanticName` に書く `"POSITION"` は、何と対応するか。

- A. HLSL の頂点シェーダの入力に付けた `: POSITION`
- B. C++ の頂点の構造体のメンバーの名前
- C. HLSL の変数の名前
- D. 定数バッファの名前

<details><summary>答え</summary>

**A**。対応は「セマンティクス」の名前で取る。C++ のメンバーの名前や HLSL の変数の名前は、何でもよい。

</details>

**Q11.** インデックスバッファを使う利点は。

- A. 頂点シェーダが要らなくなる
- B. 三角形が必ず表を向く
- C. 複数の三角形が同じ頂点を共有でき、頂点のデータが減る
- D. テクスチャを読むのが速くなる

<details><summary>答え</summary>

**C**。隣り合う三角形は頂点を共有することが多い。頂点は 1 つ 30〜50 バイト、インデックスは 2 バイト（16 ビット）なので、共有するほど小さくなる。頂点シェーダが走る回数も減る。

</details>

**Q12.** 四角形 1 枚を、三角形リストとインデックスバッファで描く。頂点とインデックスはいくつ要るか。

- A. 頂点 6・インデックス 6
- B. 頂点 4・インデックス 6
- C. 頂点 4・インデックス 4
- D. 頂点 3・インデックス 6

<details><summary>答え</summary>

**B**。四角形は三角形 2 枚。角は 4 つ（頂点 4）。三角形 1 枚につきインデックス 3 つなので 6。インデックス無しなら頂点が 6 要る。

</details>

**Q13.** `D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST` で `Draw( 9, 0 )` を呼ぶと、三角形は何枚描かれるか。

- A. 9 枚
- B. 7 枚
- C. 1 枚
- D. 3 枚

<details><summary>答え</summary>

**D**。三角形リストは、頂点を 3 つずつ区切って三角形にする。9 ÷ 3 = 3。7 枚になるのは三角形ストリップ（前の 2 頂点を使い回す）のとき。

</details>

**Q14.** 毎フレーム CPU から中身を書き換えるバッファを作るとき、`Usage` に指定するのは。

- A. `D3D11_USAGE_IMMUTABLE`
- B. `D3D11_USAGE_STAGING`
- C. `D3D11_USAGE_DYNAMIC`
- D. どれでも同じ

<details><summary>答え</summary>

**C**。`DYNAMIC`（と `CPUAccessFlags = D3D11_CPU_ACCESS_WRITE`）は、CPU から頻繁に書く物用。`IMMUTABLE` は作ったら変えない物用（一番速く読める）。`DEFAULT` は GPU が読み書きする普通の物。`STAGING` は GPU から CPU へ読み戻す用。

</details>

**Q15.** `ID3D11DeviceContext::Map` と `Unmap` は何をするか。

- A. GPU 側のバッファに CPU から書き込むための場所を借り、書き終わったら返す
- B. テクスチャをシェーダに設定する
- C. バッファを作り、消す
- D. 頂点をワールド座標へ変換する

<details><summary>答え</summary>

**A**。`Map` で書き込み先のポインタをもらい、`memcpy` などで書き、`Unmap` で返す。`D3D11_MAP_WRITE_DISCARD` は「前の中身は捨てて、全部書き直す」。

</details>

**Q16.** 頂点バッファをパイプラインに設定する API は。

- A. `VSSetVertexBuffers`
- B. `IASetVertexBuffers`
- C. `OMSetVertexBuffers`
- D. `SetVertexBuffer`

<details><summary>答え</summary>

**B**。頂点バッファを読むのは入力アセンブラ（IA）なので `IASet〜`。インデックスバッファ（`IASetIndexBuffer`）・入力レイアウト（`IASetInputLayout`）・つなぎ方（`IASetPrimitiveTopology`）も IA。

</details>

## C. シェーダ・定数バッファ・行列

**Q17.** 定数バッファの大きさについての決まりは。

- A. 4 バイトの倍数
- B. 64 バイトまで
- C. 大きさに決まりは無い
- D. 16 バイトの倍数

<details><summary>答え</summary>

**D**。16 の倍数でないと `CreateBuffer` が失敗する。C++ の構造体が 20 バイトなら、空き（パディング）を足して 32 にする。

</details>

**Q18.** HLSL の `Texture2D gTexture : register( t0 );` の `t0` は何か。

- A. 定数バッファの 0 番
- B. サンプラの 0 番
- C. テクスチャ（シェーダリソース）の 0 番
- D. 描画先の 0 番

<details><summary>答え</summary>

**C**。`b` は定数バッファ（`cbuffer`）、`t` はテクスチャ、`s` はサンプラ。C++ 側の `PSSetShaderResources( 0, ... )` の 0 と対応する。

</details>

**Q19.** 同じ定数バッファを、頂点シェーダとピクセルシェーダの両方で使いたい。どうするか。

- A. `VSSetConstantBuffers` と `PSSetConstantBuffers` の両方で設定する
- B. `VSSetConstantBuffers` で設定すれば、ピクセルシェーダからも見える
- C. 定数バッファを 2 つ作らないといけない
- D. できない

<details><summary>答え</summary>

**A**。スロット（b0・b1…）は、シェーダの段ごとに別々にある。頂点シェーダの b0 に設定しても、ピクセルシェーダの b0 は空のまま。同じバッファを両方に設定するのは構わない。

</details>

**Q20.** 次の cbuffer で、`gB` は先頭から何バイト目に置かれるか。

```hlsl
cbuffer Example : register( b0 )
{
    float3 gA;
    float3 gB;
};
```

- A. 12
- B. 16
- C. 24
- D. 0

<details><summary>答え</summary>

**B**。`gA` は 0〜11。次の 12 から `gB`（12 バイト）を置くと 12〜23 で、16 バイトの区切りをまたぐ。1 つの変数は区切りをまたげないので、16 へ飛ばされる。C++ 側を `float a[ 3 ]; float b[ 3 ];` と書くと `b` は 12 から始まり、ずれる。C++ 側に `float pad;` を入れる。

</details>

**Q21.** ワールド行列・ビュー行列・射影行列のうち、「カメラの位置と向き」を表すのは。

- A. ワールド行列
- B. 射影行列
- C. ビュー行列
- D. どれでもない

<details><summary>答え</summary>

**C**。ワールド行列は「物を世界のどこに置くか」、ビュー行列は「世界をカメラから見た座標にする」（カメラの位置と向きの逆変換）、射影行列は「画角・遠近」。

</details>

**Q22.** D3D11 で、w で割った後（正規化デバイス座標）の z の範囲は。

- A. 0〜1
- B. −1〜1
- C. 0〜255
- D. near〜far

<details><summary>答え</summary>

**A**。x と y は −1〜1、z は 0（近い）〜1（遠い）。OpenGL は z も −1〜1 なので、OpenGL の資料の射影行列をそのまま使うとずれる。

</details>

**Q23.** 射影行列（透視射影）の役割は。

- A. 物を回転させる
- B. カメラを動かす
- C. 色を変換する
- D. 遠くの物ほど小さく見えるように、視界の範囲をクリップ空間へ変換する

<details><summary>答え</summary>

**D**。画角・縦横比・near・far から作る。出力の w に「奥行き」を入れ、GPU が x・y・z を w で割ることで遠近が付く。

</details>

**Q24.** HLSL を実行時にコンパイルする関数は。

- A. `CreateVertexShader`
- B. `D3DCompile`（か `D3DCompileFromFile`）
- C. `VSSetShader`
- D. `CompileShader`

<details><summary>答え</summary>

**B**。`D3DCompile` が HLSL の文字列をバイトコード（`ID3DBlob`）にする。`CreateVertexShader` はバイトコードからシェーダのオブジェクトを作る。`VSSetShader` はそれをパイプラインに設定する。「コンパイル → 作る → 設定する」の 3 段。

</details>

## D. テクスチャ・ビュー・ステート

**Q25.** テクスチャを、描く先（描画先）として使うために要るビューは。

- A. `ID3D11ShaderResourceView`
- B. `ID3D11DepthStencilView`
- C. `ID3D11RenderTargetView`
- D. ビューは要らない

<details><summary>答え</summary>

**C**。RTV は「色を描く先として使う」、DSV は「深度を書く先として使う」、SRV は「シェーダから読む」。同じテクスチャに、使い方ごとのビューを作る。

</details>

**Q26.** 深度バッファの役割は。

- A. 画素ごとに一番手前の深度を覚え、奥の物が手前の物を上書きしないようにする
- B. 色を覚えておく
- C. 半透明を計算する
- D. 影を付ける

<details><summary>答え</summary>

**A**。描こうとする画素の深度を、既に書かれている深度と比べ、奥なら捨てる。これが無いと「後から描いた物が手前」になり、描く順を自分で並べないといけない。

</details>

**Q27.** フレームの始めに深度バッファを消す（`ClearDepthStencilView`）とき、普通に入れる深度の値は。

- A. 0.0
- B. −1.0
- C. 0.5
- D. 1.0

<details><summary>答え</summary>

**D**。1.0 は一番奥。「それより手前なら描く」（`D3D11_COMPARISON_LESS`）ので、最初は何を描いても通る。0.0 で消すと、何も描かれなくなる。

</details>

**Q28.** サンプラステートが決める物は。

- A. テクスチャの大きさ
- B. テクスチャの読み方（画素の間の補い方と、0〜1 の外の扱い）
- C. テクスチャの形式（何ビットか）
- D. どのテクスチャを読むか

<details><summary>答え</summary>

**B**。フィルタ（点・線形・異方性）とアドレスモード（繰り返す・端で止める・鏡写し・枠の色）。大きさと形式はテクスチャ自身が持ち、どれを読むかは `PSSetShaderResources` で決まる。

</details>

**Q29.** 普通の半透明（アルファブレンド）にする `SrcBlend` と `DestBlend` の組は。

- A. `ONE` と `ZERO`
- B. `ONE` と `ONE`
- C. `SRC_ALPHA` と `INV_SRC_ALPHA`
- D. `ZERO` と `ONE`

<details><summary>答え</summary>

**C**。式は「描く色 × a ＋ 既にある色 ×（1 − a）」。A は不透明（上書き）、B は加算（光）、D は何も描かれない。

</details>

**Q30.** 裏を向いた三角形を捨てるかどうか（カリング）を決めるのは、どのステートか。

- A. ラスタライザステート
- B. ブレンドステート
- C. 深度ステンシルステート
- D. サンプラステート

<details><summary>答え</summary>

**A**。`D3D11_RASTERIZER_DESC` の `CullMode`（捨てない・裏を捨てる・表を捨てる）と `FrontCounterClockwise`（どちら回りが表か）。

</details>

**Q31.** 不透明な物と半透明な物が混ざったシーンを、正しく描く順は。

- A. 半透明を先に全部描き、その後に不透明を描く
- B. 描く順は関係ない（深度バッファが解決する）
- C. 全部を手前から順に描く
- D. 不透明を先に全部描き、その後に半透明を奥から順に（深度は書かずに）描く

<details><summary>答え</summary>

**D**。半透明は「既にある色」と混ぜるので、後ろの物が先に描かれていないといけない。深度を書くと、後ろの半透明が深度テストで捨てられるので、半透明は深度を読むだけにする。

</details>

**Q32.** ビューポート（`RSSetViewports`）を一度も設定しないで描くと、どうなるか。

- A. 画面全体に描かれる
- B. 何も描かれない
- C. 実行時に例外が飛ぶ
- D. 左上の 1 画素にだけ描かれる

<details><summary>答え</summary>

**B**。ビューポートは「正規化デバイス座標（−1〜1）を、描画先のどの範囲の画素に対応させるか」。設定しないと大きさが 0 で、何も描かれない。エラーで止まらないので、初めての三角形が出ない原因の定番。

</details>

**Q33.** テクスチャをピクセルシェーダから読むために、C++ 側で設定する物の組は。

- A. RTV とサンプラ
- B. DSV と SRV
- C. SRV（`PSSetShaderResources`）とサンプラ（`PSSetSamplers`）
- D. SRV だけでよい（サンプラは要らない）

<details><summary>答え</summary>

**C**。HLSL の `gTexture.Sample( gSampler, uv )` は、テクスチャとサンプラの 2 つを使う。片方でも設定を忘れると、黒になるか、デバッグレイヤーが警告を出す。

</details>

## E. COM・エラー・デバッグ

**Q34.** D3D11 の関数が返す `HRESULT` で、成功かどうかを確かめる正しい書き方は。

- A. `if ( SUCCEEDED( hr ) )`
- B. `if ( hr == 0 || hr == 1 )`
- C. `if ( hr )`
- D. `if ( hr != nullptr )`

<details><summary>答え</summary>

**A**。`HRESULT` は「負なら失敗」。成功にも `S_OK`（0）のほかに `S_FALSE`（1）などがあるので、`SUCCEEDED( hr )` か `FAILED( hr )` のマクロで確かめる。`if ( hr )` は、成功の `S_OK` が 0 なので逆になる。

</details>

**Q35.** `ID3D11Buffer*` などの COM オブジェクトを使い終わったら、どうするか。

- A. `delete` する
- B. `free` する
- C. 何もしなくてよい
- D. `Release()` を呼ぶ

<details><summary>答え</summary>

**D**。COM のオブジェクトは「参照カウント」で生きている。`Release` でカウントを 1 減らし、0 になったら自分で消える。`delete` してはいけない（ほかの人がまだ使っているかもしれない）。

</details>

**Q36.** `Microsoft::WRL::ComPtr< ID3D11Buffer >` を使う利点は。

- A. 描画が速くなる
- B. 変数が消えるとき、自動で `Release` を呼んでくれる
- C. `HRESULT` を確かめなくてよくなる
- D. バッファの中身を自動で更新してくれる

<details><summary>答え</summary>

**B**。COM 用のスマートポインタ。`Release` の書き忘れ（リーク）と、呼びすぎ（二重解放）が無くなる。C++ の RAII そのもの。

</details>

**Q37.** D3D11 のデバッグレイヤーを有効にするには。

- A. Visual Studio を Debug ビルドにするだけでよい
- B. `Present` の引数に 1 を渡す
- C. デバイスを作るときのフラグに `D3D11_CREATE_DEVICE_DEBUG` を付ける
- D. `#define DEBUG` を書く

<details><summary>答え</summary>

**C**。付けると、間違った使い方をしたときに、出力ウィンドウへ `D3D11 ERROR:`・`D3D11 WARNING:` が文章で出る。Debug ビルドにしただけでは付かない（自分のコードで `#ifdef _DEBUG` の中に書く）。

</details>

**Q38.** `Present( 1, 0 )` と `Present( 0, 0 )` の違いは。

- A. 1 はモニターの更新（垂直同期）を待ってから出す。0 は待たずにすぐ出す
- B. 1 はフルスクリーン、0 はウィンドウ
- C. 1 は 1 回だけ、0 は毎フレーム出す
- D. 違いは無い

<details><summary>答え</summary>

**A**。1 にすると、フレームレートがモニターのリフレッシュレートに揃い、ティアリング（絵が横にずれる）が出ない。0 は最大の速さで出す（計測のとき・遅延を減らしたいとき）。

</details>

**Q39.** ウィンドウの大きさが変わったので `IDXGISwapChain::ResizeBuffers` を呼びたい。その前に必ずやることは。

- A. デバイスを作り直す
- B. シェーダをコンパイルし直す
- C. `Present` を呼ぶ
- D. バックバッファを指しているビュー（RTV など）を、全部解放する

<details><summary>答え</summary>

**D**。バックバッファへの参照が 1 つでも残っていると、`ResizeBuffers` は失敗する。コンテキストからも外す（`OMSetRenderTargets( 0, nullptr, nullptr )`）。変えた後、RTV と深度バッファを新しい大きさで作り直す。

</details>

**Q40.** 1 フレームの処理の順として正しいのは。

- A. `Present` → 消去 → 設定 → `Draw`
- B. 消去（`Clear〜`）→ 設定（`〜Set〜`）→ `Draw` → `Present`
- C. `Draw` → 消去 → 設定 → `Present`
- D. 設定 → `Draw` → 消去 → `Present`

<details><summary>答え</summary>

**B**。前のフレームの絵を消す → 描く物ごとに設定して `Draw` → 全部描き終えたら `Present`。D の順だと、描いた絵を消してから出すので、消した色だけが出る。

</details>

---

# 記述

答えを開く前に、必ず自分で書くこと。コードを書く問題は、API の引数を公式ドキュメントで引いてよい。

**問 1.** デバイス・デバイスコンテキスト・スワップチェーンの役割を、1 文ずつで書け。

<details><summary>答えの例</summary>

- デバイス：GPU の物（バッファ・テクスチャ・シェーダ・ステート）を作る
- デバイスコンテキスト：作った物をパイプラインに設定し、描画の命令を出す
- スワップチェーン：ウィンドウに出す絵（バックバッファ）を持ち、`Present` で画面に出す

</details>

**問 2.** パイプラインの 5 つの段（IA・VS・RS・PS・OM）について、それぞれ「何を受け取り、何を出すか」を書け。

<details><summary>答えの例</summary>

| 段 | 受け取る物 | 出す物 |
|---|---|---|
| IA 入力アセンブラ | 頂点バッファ・インデックスバッファ・入力レイアウト・つなぎ方 | 頂点（と、どの 3 つで三角形か） |
| VS 頂点シェーダ | 頂点 1 つ・定数バッファ | クリップ空間の位置（`SV_Position`）と、PS へ渡す値（色・UV・法線など） |
| RS ラスタライザ | VS の出力 3 つ（三角形）・ラスタライザステート・ビューポート | 三角形が覆う画素と、その画素での補間した値 |
| PS ピクセルシェーダ | 補間した値・テクスチャ・サンプラ・定数バッファ | その画素の色（`SV_Target`） |
| OM 出力マージャ | PS の色と深度・深度ステンシルステート・ブレンドステート | 描画先（RTV）と深度（DSV）への書き込み |

</details>

**問 3.** 空のウィンドウに三角形を 1 枚出すまでに、(a) 起動時に 1 回作る物と、(b) 毎フレーム呼ぶ物を、順に全部書け。API の名前で書けるとなお良い。

<details><summary>答えの例</summary>

**(a) 起動時に 1 回**

1. デバイス・コンテキスト・スワップチェーン（`D3D11CreateDeviceAndSwapChain`）
2. バックバッファの RTV（`GetBuffer` → `CreateRenderTargetView`）
3. HLSL をコンパイル（`D3DCompile` を VS 用と PS 用に）
4. 頂点シェーダ・ピクセルシェーダ（`CreateVertexShader`・`CreatePixelShader`）
5. 入力レイアウト（`CreateInputLayout`。VS のバイトコードが要る）
6. 頂点バッファ（`CreateBuffer`）

**(b) 毎フレーム**

1. `OMSetRenderTargets`（描く先）
2. `RSSetViewports`（描く範囲）
3. `ClearRenderTargetView`（消す）
4. `IASetInputLayout`・`IASetVertexBuffers`・`IASetPrimitiveTopology`
5. `VSSetShader`・`PSSetShader`
6. `Draw( 3, 0 )`
7. `Present`

1・2 は変わらなければ起動時に 1 回でもよい。深度バッファは、三角形 1 枚だけなら無くても出る。
これを何も見ずに書けるのが、[03 章](03_D3D11の初期化と三角形.md) のゴール。

</details>

**問 4.** 次の頂点の構造体に合う `D3D11_INPUT_ELEMENT_DESC` の配列を書け。1 頂点は何バイトか。

```cpp
struct Vertex{
    float position[ 3 ];
    float normal[ 3 ];
    float uv[ 2 ];
};
```

<details><summary>答えの例</summary>

```cpp
D3D11_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
```

- 1 頂点は 32 バイト（12 ＋ 12 ＋ 8）
- 5 番目の数（0・12・24）が、頂点の先頭からのバイト数。`offsetof( Vertex, normal )` と書けば数え間違えない
- 形式は「float が何個か」で決まる。3 個なら `R32G32B32_FLOAT`、2 個なら `R32G32_FLOAT`（名前に R・G・B とあるが、色とは関係ない）
- 自分のゲームの同じ所：`ManagerImpl.h` のコンストラクタの `layout[]`（位置が `float` 4 個・色が 4 バイトなので、数は違う）

</details>

**問 5.** 次の仕様の HLSL を書け。

- 定数バッファ（b0）に、ワールド・ビュー・射影を掛け合わせた行列が 1 つ入っている
- 頂点の入力は、位置（`float3`）と色（`float4`）
- 頂点シェーダは、位置を行列で変換し、色はそのまま渡す
- ピクセルシェーダは、渡された色をそのまま出す

<details><summary>答えの例</summary>

```hlsl
cbuffer PerObject : register( b0 )
{
    float4x4 gWorldViewProjection;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color    : COLOR0;
};

struct PSInput
{
    float4 position : SV_Position;
    float4 color    : COLOR0;
};

PSInput VSMain( VSInput v )
{
    PSInput o;
    o.position = mul( float4( v.position, 1.0 ), gWorldViewProjection );
    o.color = v.color;
    return o;
}

float4 PSMain( PSInput p ) : SV_Target
{
    return p.color;
}
```

確かめる点：

- 位置を `float4( v.position, 1.0 )` にしているか（w = 1 でないと平行移動が効かない）
- 頂点シェーダの出力の位置に `SV_Position` が付いているか
- ピクセルシェーダの戻り値に `SV_Target` が付いているか
- `mul` の引数の順（`mul( ベクトル, 行列 )` か `mul( 行列, ベクトル )` か）は、C++ 側の行列の決まりと渡し方で変わる。DirectXMath の行列を転置せずに `row_major` で渡すか、転置して渡すなら上の順。**どちらの決まりで書いたかを言えること**が大事（[04 章](04_行列とカメラと定数バッファ.md) の表）

</details>

**問 6.** 次の cbuffer と同じ並びになる、C++ の構造体を書け（DirectXMath の `XMFLOAT4X4`・`XMFLOAT3`・`XMFLOAT2` を使ってよい）。全体は何バイトか。

```hlsl
cbuffer Frame : register( b0 )
{
    float4x4 gViewProjection;
    float3   gEyePosition;
    float    gTime;
    float3   gLightDirection;
    float2   gScreenSize;
};
```

<details><summary>答えの例</summary>

```cpp
struct FrameConstants{
    DirectX::XMFLOAT4X4 viewProjection;  //   0 〜  63
    DirectX::XMFLOAT3   eyePosition;     //  64 〜  75
    float               time;            //  76 〜  79（eyePosition の後ろの空きに入る）
    DirectX::XMFLOAT3   lightDirection;  //  80 〜  91
    float               pad0;            //  92 〜  95（空き）
    DirectX::XMFLOAT2   screenSize;      //  96 〜 103
    float               pad1[ 2 ];       // 104 〜 111（16 の倍数にする空き）
};
static_assert( sizeof( FrameConstants ) == 112, "FrameConstants : size must match the cbuffer." );
```

全体は 112 バイト。

- `gTime` は、`gEyePosition`（12 バイト）の後ろの 4 バイトにちょうど入る
- `gScreenSize`（8 バイト）は、`gLightDirection` の後ろの 4 バイトには入らない（16 バイトの区切りをまたぐ）ので、次の区切り（96）へ飛ぶ。**C++ 側は `pad0` を入れないと、ここから後ろが全部 4 バイトずれる**
- 最後の `pad1` は、全体を 16 の倍数にするため
- ずれても、エラーも警告も出ない。`static_assert` で大きさを確かめておくと、少なくとも大きさの違いは見つかる

自分のゲームで、まさにこの種類の食い違いが 1 つある（[04 章](04_行列とカメラと定数バッファ.md) の課題 04-2）。

</details>

**問 7.** 問 6 の定数バッファを、(a) 作るコードと、(b) 毎フレーム中身を書き換えて頂点シェーダに設定するコードを書け。

<details><summary>答えの例</summary>

```cpp
// (a) 作る
D3D11_BUFFER_DESC desc = {};
desc.ByteWidth      = sizeof( FrameConstants );
desc.Usage          = D3D11_USAGE_DYNAMIC;
desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
ComPtr< ID3D11Buffer > frameBuffer;
HRESULT hr = device->CreateBuffer( &desc, nullptr, &frameBuffer );

// (b) 毎フレーム
FrameConstants constants = {};
// …… constants に値を入れる ……
D3D11_MAPPED_SUBRESOURCE mapped = {};
hr = context->Map( frameBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
if ( SUCCEEDED( hr ) ){
    memcpy( mapped.pData, &constants, sizeof( constants ) );
    context->Unmap( frameBuffer.Get(), 0 );
}
context->VSSetConstantBuffers( 0, 1, frameBuffer.GetAddressOf() );
```

確かめる点：

- `Usage` が `DYNAMIC`、`CPUAccessFlags` が `WRITE`（この 2 つが揃わないと `Map` で書けない）
- `BindFlags` が `CONSTANT_BUFFER`
- `Map` の種類が `WRITE_DISCARD`
- `Map` と `Unmap` が対になっている
- 自分のゲームの同じ所：`ConstantBufferImpl.h` のコンストラクタと `set`

</details>

**問 8.** 次のプログラムは、画面は青く塗られるが、三角形が出ない。足りない呼び出しを全部挙げよ（シェーダ・入力レイアウト・頂点バッファは正しく作ってあり、変数に入っているとする）。

```cpp
// 起動時
D3D11CreateDeviceAndSwapChain( /* 省略 */ );
swapChain->GetBuffer( 0, IID_PPV_ARGS( &backBuffer ) );
device->CreateRenderTargetView( backBuffer.Get(), nullptr, &rtv );

// 毎フレーム
float color[ 4 ] = { 0.f, 0.f, 0.3f, 1.f };
context->ClearRenderTargetView( rtv.Get(), color );
context->IASetVertexBuffers( 0, 1, vertexBuffer.GetAddressOf(), &stride, &offset );
context->VSSetShader( vertexShader.Get(), nullptr, 0 );
context->PSSetShader( pixelShader.Get(), nullptr, 0 );
context->Draw( 3, 0 );
swapChain->Present( 1, 0 );
```

<details><summary>答え</summary>

4 つ足りない。

1. `OMSetRenderTargets( 1, rtv.GetAddressOf(), nullptr )`：描く先を設定していない。消去（`Clear〜`）はビューを直接渡すので動くが、`Draw` は設定された描画先へ描く
2. `RSSetViewports`：描く範囲を設定していない
3. `IASetInputLayout`：頂点の並びを設定していない
4. `IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST )`：頂点のつなぎ方を設定していない（既定は「未定義」）

デバッグレイヤーを付けていれば、出力ウィンドウに何が足りないかが出る。**付けていないと、何も言われずに何も出ない。**
「青くは塗られる」のが引っかけで、塗られているから描画先は合っている、と思い込みやすい。

</details>

**問 9.** 深度バッファが無いと何が起きるか。深度バッファを使うために、起動時と毎フレームに何をするか書け。

<details><summary>答えの例</summary>

無いと、物の前後が「描いた順」で決まる。後から描いた物が、奥にあっても手前に出る。立方体 1 つでも、裏側の面が表に出ておかしく見える。

起動時：

1. バックバッファと同じ大きさのテクスチャを、深度の形式（`D32_FLOAT` か `D24_UNORM_S8_UINT`）・`D3D11_BIND_DEPTH_STENCIL` で作る（`CreateTexture2D`）
2. その DSV を作る（`CreateDepthStencilView`）
3. 深度ステンシルステートを作る（既定のままでも、テストする・書く・`LESS` になっている）

毎フレーム：

1. `OMSetRenderTargets` の 3 番目の引数に DSV を渡す
2. `ClearDepthStencilView` で 1.0 に消す

ウィンドウの大きさが変わったら、深度のテクスチャも同じ大きさで作り直す。

</details>

**問 10.** 四角形にテクスチャを貼って表示するために、三角形だけのときから増える物を全部書け（頂点・C++ 側・HLSL 側）。

<details><summary>答えの例</summary>

- 頂点：UV（`float2`）を足す。入力レイアウトにも `TEXCOORD` を足す
- C++ 側（作る）：テクスチャ（`CreateTexture2D`）、その SRV（`CreateShaderResourceView`）、サンプラステート（`CreateSamplerState`）
- C++ 側（毎フレーム）：`PSSetShaderResources`、`PSSetSamplers`
- HLSL 側：`Texture2D gTexture : register( t0 );`・`SamplerState gSampler : register( s0 );` の宣言。頂点シェーダが UV をピクセルシェーダへ渡す。ピクセルシェーダで `gTexture.Sample( gSampler, uv )`

UV は左上が ( 0, 0 )、右下が ( 1, 1 )。

</details>

**問 11.** 「ビュー」とは何か。RTV・DSV・SRV の違いを説明せよ。なぜテクスチャを直接設定せず、ビューを通すのか。

<details><summary>答えの例</summary>

ビューは、リソース（テクスチャやバッファ）を「何として・どの部分を・どの型で使うか」を決める札。

- RTV（レンダーターゲットビュー）：色を描く先として使う。`OMSetRenderTargets`
- DSV（深度ステンシルビュー）：深度を比べる・書く先として使う。`OMSetRenderTargets`
- SRV（シェーダリソースビュー）：シェーダから読む。`PSSetShaderResources`

ビューを通す理由：1 枚のテクスチャを、あるときは描く先・別のときは読む対象、と使い分けられる（描画先にした絵を、次のパスで読む。[08 章](08_描画先とディファードとPBR.md)）。ミップの 1 段だけ・配列の 1 枚だけを選ぶことも、同じデータを別の型（深度として書き、数として読む）で使うこともできる。

</details>

**問 12.** COM の参照カウントとは何か。`Release` を呼び忘れると・呼びすぎると、それぞれ何が起きるか。防ぐ方法は。

<details><summary>答えの例</summary>

COM のオブジェクトは、「自分を使っている人の数」を自分で数えている。`AddRef` で 1 増え、`Release` で 1 減り、0 になると自分で消える。`Create〜` や `GetBuffer` で受け取った時点で、自分の分の 1 が入っている。

- 呼び忘れ：0 にならないので消えない（リーク）。デバッグレイヤーを付けていれば、終了時に生き残っている物の一覧が出る
- 呼びすぎ：まだ使っている人がいるのに消える。消えた物を触って落ちる

防ぐ方法：`Microsoft::WRL::ComPtr` を使う。変数が消えるとき自動で `Release` し、コピーすると自動で `AddRef` する。

</details>

**問 13.** ある頂点を、モデルの中の位置から画面の画素の位置まで変換する流れを、順に書け。どこまでが頂点シェーダの仕事で、どこからが GPU の仕事か。

<details><summary>答えの例</summary>

1. モデルの中の位置（頂点バッファに入っている）
2. × ワールド行列 → 世界の中の位置
3. × ビュー行列 → カメラから見た位置
4. × 射影行列 → クリップ空間の位置（x・y・z・w）　**ここまでが頂点シェーダ（`SV_Position` として出す）**
5. 画面の外を切る（クリッピング）
6. x・y・z を w で割る → 正規化デバイス座標（x・y が −1〜1、z が 0〜1）
7. ビューポートで、画素の位置に直す（x・y）。z は深度になる　**5〜7 は GPU（ラスタライザ）がやる**

2〜4 の行列は、あらかじめ 1 つに掛け合わせて渡すことが多い。

</details>

**問 14.** 自分のゲームのコードは、D3D11 を直接呼ばず `Graphics::Manager` の関数を呼んでいる。次の 6 つの関数が、D3D11 のどの API を呼んでいるかを書け（[00 章](00_全体の地図.md) の表を見ずに。分からなければ `ManagerImpl.h` の同じ名前の関数を読んで確かめる）。

- `setVertexBuffer`
- `setTexture`
- `setBlendMode`
- `setCullMode`
- `setRenderTargets`
- `drawIndexed`

<details><summary>答え</summary>

| GameLib | D3D11 |
|---|---|
| `setVertexBuffer` | `IASetVertexBuffers` |
| `setTexture` | `PSSetShaderResources`（スロット 0） |
| `setBlendMode` | `OMSetBlendState` |
| `setCullMode` | `RSSetState` |
| `setRenderTargets` | `OMSetRenderTargets`（と `RSSetViewports`） |
| `drawIndexed` | `DrawIndexed`（その前に、変わっていれば定数バッファを `Map`／`Unmap` で送る） |

この対応が頭に入っていれば、公式のドキュメントやチュートリアルを「自分のコードのどこの話か」に読み替えられる。

</details>
