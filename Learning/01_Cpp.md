# 01 C++：このコードで実際に使っている機能

C++ は広いので、全部は追わない。**このゲームのコードに実際に出てくる書き方**だけを、出てくる回数の多い順に押さえる。

## この章でできるようになること

- GameSrc のどのファイルを開いても、「知らない書き方」で止まらない
- 「誰が持ち主で、誰が借りているか」を、型を見て言える
- `GameObject::addComponent< T >()` のようなテンプレートを、自分で書ける

## このコードが使っている C++（数えた結果）

GameSrc の `.h`・`.cpp`（619 ファイル）を文字で数えた（2026-11-12）。回数はおおよそ。

| 書き方 | 回数 | 出てくるファイル | 節 |
|---|---|---|---|
| `const 型&`（const 参照で受ける） | 1187 | 365 | 2 |
| `std::vector` | 515 | 192 | 6 |
| `static_cast` | 371 | 135 | 7 |
| `ASSERT( 条件 && "説明" )` | 342 | 169 | 8 |
| 無名の `namespace{ }` | 206 | 205 | 1 |
| 範囲 for（`for ( const T& x : list )`） | 182 | 78 | 6 |
| `= delete`（コピー禁止） | 128 | 63 | 3 |
| `virtual`・`override` | 102・83 | 43 | 4 |
| `std::unique_ptr` | 66 | 36 | 3 |
| ラムダ式 | 61 | 32 | 6 |
| `template` | 50 | 31 | 5 |
| `enum class` | 44 | 39 | 7 |
| `std::function` | 28 | 20 | 6 |
| `std::move` | 12 | 10 | 3 |

**使っていない物**も大事。`std::shared_ptr` は 0 回、`dynamic_cast` は 1 回、生の `new` は 1 回。
つまりこのコードは「持ち主は 1 人（`unique_ptr`）、ほかは借りるだけ（生のポインタか参照）」で統一されている。
規約は `C:\AIResorce\GameDev\Cpp\メモリ管理方針.md`。

## 先に読む

- [江添亮の C++ 入門](https://ezoeryou.github.io/cpp-intro/)（日本語・無料）：ポインタ・参照・クラス・テンプレートの章
- [learncpp.com](https://www.learncpp.com/)（英語・無料）：22 章 Move semantics and smart pointers、25 章 Virtual functions、26 章 Templates and classes
- [cpprefjp](https://cpprefjp.github.io/)：標準ライブラリを引く辞書。`std::unique_ptr`・`std::vector`・`std::function`
- Microsoft Learn「[スマート ポインター (Modern C++)](https://learn.microsoft.com/ja-jp/cpp/cpp/smart-pointers-modern-cpp)」
- 本：Scott Meyers『Effective Modern C++』項目 18（`unique_ptr`）・23〜25（`std::move` と `std::forward`）

## 1. ファイルの分け方：ヘッダ・cpp・無名 namespace

**見る所**：[RenderPass.cpp](../GameSrc/src/Render/RenderPass.cpp) の先頭、[GBuffer.cpp](../GameSrc/src/Render/Lighting/GBuffer.cpp) の先頭

どの `.cpp` も先頭に `namespace{ ... }` があり、`kXxx` という定数や小さな関数が入っている。

**読むときの問い**

1. 無名の `namespace{ }` に入れた定数・関数は、ほかの `.cpp` から見えるか
2. `RenderPass.cpp` の `gCurrent` はグローバル変数だが、ほかのファイルから書き換えられるか。書き換える入口はどこか
3. `GBuffer.cpp` の `puddleParams()` は、中に `static PuddleParams sParams;` を持つ。この変数はいつ作られ、いつ消えるか。なぜ普通のグローバル変数にしないのか
4. ヘッダの `#ifndef INCLUDED_...` は何を防いでいるか。[Component.h](../GameSrc/include/GameSrc/World/Component.h) の `class GameObject;`（中身の無い宣言）は何のためか

<details><summary>答え</summary>

1. 見えない。無名 namespace の中の名前は、その `.cpp` の中だけで有効（内部リンケージ）。別のファイルに同じ名前の定数があってもぶつからない。マジックナンバーに名前を付ける場所として使っている
2. 直接は書き換えられない。入口は `RenderPass::beginGBuffer`・`endGBuffer`・`beginShadow`・`endShadow` だけ。「状態はファイルの中に隠し、変える入口を関数に限る」形
3. 初めて `puddleParams()` が呼ばれたときに 1 回だけ作られ、プログラムの終わりに消える。普通のグローバル変数は、ファイルをまたぐと「どれが先に作られるか」が決まっていない。関数の中の static なら「初めて使うとき」と決まるので、作られる前に使われる事故が無い
4. 同じヘッダを 2 回読んでも、中身が 2 回定義されないようにする。`class GameObject;` は前方宣言。ポインタ（`GameObject*`）を持つだけなら中身を知らなくてよいので、`GameObject.h` を読み込まずに済む（ヘッダどうしが互いに読み合うのを避け、ビルドも速くなる）

</details>

## 2. クラス・const・参照とポインタ

**見る所**：[FixedStepClock.cpp](../GameSrc/src/Core/FixedStepClock.cpp)、[GameObject.h](../GameSrc/include/GameSrc/World/GameObject.h)

**読むときの問い**

1. `FixedStepClock::FixedStepClock() : mAccumulated( 0.0 ), ...` の `:` の後ろ（初期化リスト）と、`{ }` の中で代入するのは何が違うか
2. `bool isPaused() const` の `const` は何を約束しているか
3. `const std::string& name() const` は、なぜ `std::string` ではなく `const std::string&` で返すか
4. 引数が `const Matrix34& m` の関数と `Matrix34* out` の関数がある。この使い分けの決まりは何か

<details><summary>答え</summary>

1. 初期化リストは、メンバーを「作るとき」に値を入れる。`{ }` の中の代入は、一度（既定の値で）作ってから入れ直す。const のメンバー・参照のメンバー・既定のコンストラクタが無い型は、初期化リストでしか入れられない。書く順ではなく**クラスの中で宣言した順**に初期化される点にも注意
2. この関数の中でメンバーを書き換えない、という約束。`const` なオブジェクト（や const 参照で借りた物）からも呼べるようになる
3. `std::string` で返すと、呼ぶたびに文字列を写す（メモリを確保する）。const 参照なら写さず、書き換えもされない。ただし返した参照は、持ち主（GameObject）が生きている間しか使えない
4. 読むだけの入力は const 参照、関数が書き込む出力はポインタ。呼び出し側に `&` が見えるので、「ここで書き換えられる」と読める（`worldToRoot.mul( &rootPoint, worldPoint )`）

</details>

## 3. 持ち主と寿命：unique_ptr・std::move・コピー禁止・Release

**見る所**：[GameObject.h](../GameSrc/include/GameSrc/World/GameObject.h) の `addComponent`、[SceneManager.cpp](../GameSrc/src/Scene/SceneManager.cpp) の `createScene` と `change`、[Component.h](../GameSrc/include/GameSrc/World/Component.h)

このコードには「物の持ち方」が 3 種類ある。まずこれを見分けられるようにする。

| 持ち方 | 書き方 | 消えるとき | 例 |
|---|---|---|---|
| 持ち主（1 人だけ） | `std::unique_ptr< T >` | 持ち主が消えるとき自動で | `GameObject` の中の `Entry::component` |
| 借りる | `T*`・`const T&` | 自分では消さない | `Component::mGameObject`、`addComponent` の戻り値 |
| GameLib のハンドル | `Graphics::Shader`・`Texture`・`RenderTarget` など | 最後の 1 つのハンドルが消えたとき（参照カウント） | `GBuffer` の `mStaticMeshShader` |

GameLib のハンドルは、小さなクラスが `Impl*` を 1 つ持つ形（[Shader.cpp](../GameLib/src/Graphics/Shader.cpp)・[ShaderImpl.h](../GameLib/src/Graphics/ShaderImpl.h)）。
コピーしても GPU の物は増えず、指す先を数えるだけ。中身の `Impl` が D3D11 の COM オブジェクト（`ID3D11VertexShader*` など）を持ち、デストラクタで `Release()` する。

**読むときの問い**

1. `addComponent` の中で、`std::make_unique< T >(...)` の後に `T* raw = component.get();` を取り、その後 `entry.component = std::move( component );` している。`std::move` の後の `component` はどうなっているか。なぜ先に `raw` を取るのか
2. `Component` は `Component( const Component& ) = delete;` でコピーを禁じている。コピーできると何が壊れるか
3. `SceneManager::change` は、`createScene( id )` で次のシーンを作り終えてから `mCurrent = std::move( next );` している。前のシーンはどの行で消えるか。順番を逆（先に前のシーンを消す）にしない理由は
4. `Component` のデストラクタは `virtual ~Component() = default;`。`virtual` を付け忘れると何が起きるか
5. [ManagerImpl.h](../GameLib/src/Graphics/ManagerImpl.h) のデストラクタは `mDevice->Release()` などを 1 つずつ書いている。`Release` を 1 つ忘れるとどうなるか。忘れない仕組みは何か

<details><summary>答え</summary>

1. `std::move` の後の `component` は空（nullptr）。持ち主が `entry.component` に移ったため。呼び出し側へ返す「借りるポインタ」は、移す前に取っておかないと取れない
2. 部品は自分の付いている物（`mGameObject`）や隣の部品のポインタを覚えている。写すと、写した方も元の物を指したままになり、片方が消えたとき消えた物を指す。そもそも「1 つの物に同じ部品は 1 つ」という決まりとも合わない
3. `mCurrent = std::move( next );` の代入の中で、`mCurrent` が前に持っていたシーンが delete される。先に消すと、次のシーンを作っている途中に「シーンが 1 つも無い」時間ができ、作るのに失敗したとき戻れない
4. `std::unique_ptr< Component >` 経由で消すとき、派生クラス（例：`ZombieBrainComponent`）のデストラクタが呼ばれず、派生側のメンバーが片付かない（未定義動作）。基底クラスのポインタで消す物には必ず `virtual` なデストラクタを付ける
5. GPU のメモリが返らない（リーク）。Debug ビルドなら、終了時に D3D11 のデバッグレイヤーが「生きたままのオブジェクト」を出力ウィンドウに出す。忘れない仕組みは `Microsoft::WRL::ComPtr`（COM 用のスマートポインタ。消えるとき自動で `Release`）。GameLib は元が古いので手で `Release` しているが、ミニレンダラでは `ComPtr` を使う

</details>

## 4. 継承・virtual・インターフェース

**見る所**：[IScene.h](../GameSrc/include/GameSrc/Scene/IScene.h)、[Component.h](../GameSrc/include/GameSrc/World/Component.h)、[ShaderImpl.h](../GameLib/src/Graphics/ShaderImpl.h) の `IncludeHandler`

**読むときの問い**

1. `SceneManager` は `std::unique_ptr< IScene > mCurrent` を持ち、`mCurrent->update()` を呼ぶ。実際に走るのが `GameScene::update` か `MenuScene::update` かは、いつ・どうやって決まるか
2. `override` を付けると何が嬉しいか。付けなくても動くのに 83 回も書いてあるのはなぜか
3. `Component` の `awake`・`start`・`update`・`onDestroy` は `virtual void awake(){}` と中身が空。`= 0`（純粋仮想）にしていないのはなぜか
4. `IncludeHandler` は D3D の `ID3DInclude` を継承している。これは自分のコードが「呼ばれる側」になる例。誰が `Open` を呼ぶか

<details><summary>答え</summary>

1. 実行時に決まる。`virtual` な関数を持つクラスのオブジェクトは「仮想関数テーブル」へのポインタを持ち、呼び出しはその表を引いて飛ぶ。`createScene` で `GameScene` を作ったなら、表は `GameScene` の物
2. 基底クラスに同じ形の virtual 関数が無いと、コンパイルエラーになる。引数の型を 1 つ間違えて「別の新しい関数」を作ってしまう事故（呼ばれないのに気付かない）を防ぐ
3. 部品ごとに要る物だけ上書きすればよいようにするため。`= 0` だと、使わない関数も全部の部品に書かないといけない
4. `D3DCompile`。HLSL の中の `#include "Common.hlsli"` に出会うたびに、渡した `IncludeHandler` の `Open` を呼んでファイルの中身をもらう

</details>

## 5. テンプレート

**見る所**：[GameObject.h](../GameSrc/include/GameSrc/World/GameObject.h)（`addComponent`・`getComponent`・`worldMemberOf`）、[ComponentTypeId.h](../GameSrc/include/GameSrc/World/ComponentTypeId.h)

`GameObject.h` は、このコードの中で一番 C++ らしい所。ここが読めれば、ほかのテンプレートは読める。

**読むときの問い**

1. `template< class T, class... Args > T* addComponent( Args&&... args )` の `class... Args` と `Args&&...` と `std::forward< Args >( args )...` は、それぞれ何をしているか
2. `static_assert( std::is_base_of< Component, T >::value, "..." )` は、いつ・何を確かめているか。`ASSERT` と何が違うか
3. `componentTypeId< T >()` は、`T` ごとに違う番号を返す。どういう仕組みか（[ComponentTypeId.h](../GameSrc/include/GameSrc/World/ComponentTypeId.h)）。`dynamic_cast` を使わないのはなぜか
4. `worldMemberOf` の `if constexpr` は、普通の `if` と何が違うか
5. テンプレートの関数の中身が `.cpp` ではなくヘッダに書いてあるのはなぜか

<details><summary>答え</summary>

1. `class... Args` は「型がいくつでも」、`Args&&... args` は「その型の引数をいくつでも、左辺値でも右辺値でも受ける」、`std::forward< Args >( args )...` は「受けた形のまま（写さず）T のコンストラクタへ渡す」。これで `addComponent< PlayerComponent >( a, b, c )` のように、部品ごとに違う引数を 1 つの関数で受けられる
2. コンパイル時に、T が Component を継承しているかを確かめる。違えばビルドが通らない。`ASSERT` は実行して初めて分かるが、`static_assert` は実行前に分かる
3. `componentTypeId< T >()` の中の `static const int id` は、`T` ごとに別の変数になる（テンプレートは型ごとに別の関数が作られるため）。初めて呼ばれたときに `nextComponentTypeId()` が 0, 1, 2… と番号を振る。`dynamic_cast` は実行時の型情報をたどるので遅く、「同じ種類の部品を探す」を毎フレーム何度もするには向かない。整数の比較なら速い
4. `if constexpr` はコンパイル時に片方だけを残す。T が `IWorldMember` を継承していなければ、`static_cast< IWorldMember* >( component )` の行はコンパイルされない（普通の `if` だと、通らないキャストがコンパイルエラーになる）
5. テンプレートは、使う側が T を決めたときに初めて実体が作られる。その時に中身が見えていないと作れないので、ヘッダに書く

</details>

## 6. 標準ライブラリ：vector・string・function・ラムダ

**見る所**：[ShaderImpl.h](../GameLib/src/Graphics/ShaderImpl.h) の `IncludeHandler`、[PuddleParams.h](../GameSrc/include/GameSrc/Render/PostProcess/PuddleParams.h) の `visit`、[SceneRenderer.cpp](../GameSrc/src/Scene/Game/World/SceneRenderer.cpp) の `firstShaderError`

**読むときの問い**

1. `IncludeHandler` の `mBuffers` は `std::list< std::vector< char > >`。コメントに「list なのは、足しても前の中身のアドレスが変わらないため」とある。`std::vector< std::vector< char > >` だと何が起きるか
2. `for ( const Entry& entry : mComponents )` の `const Entry&` を `Entry` と書くと、動きは同じだが何が変わるか
3. `PuddleParams::visit` は `template< class V > void visit( V& v )` で、`v( "coverage", coverage, 0.f, 1.f, "" )` と呼んでいる。`v` には何が渡せるか
4. `std::vector::push_back` を呼んだ後、前に取っておいた要素へのポインタや参照を使ってよいか

<details><summary>答え</summary>

1. `Open` は `*data = mBuffers.back().data()` と、中身のアドレスを D3D のコンパイラへ渡している。外側が vector だと、足りなくなったときに内側の `std::vector< char >` という「入れ物」が新しい場所へ移る。実際には中身の配列ごと引っ越す（ムーブ）ので `data()` のアドレスは変わらないが、それは細かい決まりを知らないと言い切れない。list は要素を動かさないと決まっているので、渡したアドレスが有効なままだと一目で言える。なお中身が `std::string` だと、短い文字列は入れ物の中に直接入っているので、ムーブでアドレスが本当に変わる
2. 1 個ごとに `Entry` を写そうとする。`Entry` は `unique_ptr` を持つのでコピーできず、コンパイルエラーになる。コピーできる型なら、エラーにならずに毎回写すので遅くなる。「読むだけなら `const T&`」が基本
3. `( 名前, 値への参照, 下限, 上限, 単位 )` で呼べる物なら何でも。関数・ラムダ・`operator()` を持つクラス。F6 のエディタはスライダーを作るクラスを渡し、保存の処理は値を書き出すクラスを渡す。調整値の一覧を 1 か所に書くだけで、どちらにも使える（`GameSrc/src/DebugMode/TuningVisitors.cpp`）
4. 使ってはいけない。`push_back` で容量が足りなくなると、全部の要素が新しい場所へ移り、前のポインタは消えた場所を指す。添字（番号）で覚えるか、足し終えてから取る

</details>

## 7. キャスト・enum class・定数

**読むときの問い**

1. `static_cast< int >( mAccumulated / GameTime::kTickSeconds )` は、小数をどう整数にするか（切り捨てか四捨五入か）。[GameTime.h](../GameSrc/include/GameSrc/Core/GameTime.h) の `secondsToTicks` がわざわざ四捨五入にしている理由は
2. `enum class RenderPassKind` と、GameLib の古い `enum`（`Graphics::BLEND_OPAQUE` など）は何が違うか
3. D3D11 のコードには `reinterpret_cast< void** >( &backBuffer )` が出てくる（[ManagerImpl.h](../GameLib/src/Graphics/ManagerImpl.h) の `createRTVandDSV`）。`static_cast` ではだめな理由は

<details><summary>答え</summary>

1. 0 の方向へ切り捨てる。`secondsToTicks` は、float の誤差で 2.1 秒が 125.99999 ティックになり 1 ティック短くなるのを防ぐために四捨五入にしている（ヘッダのコメント）
2. `enum class` は名前が外へ漏れず（`RenderPassKind::GBuffer` と書く）、勝手に int にならない。古い `enum` は名前がそのまま外へ出て、int と混ぜて使えてしまう
3. `ID3D11Texture2D**` と `void**` は関係の無い型どうしなので、`static_cast` では変換できない。COM の `GetBuffer` や `QueryInterface` は「どんな型でも返せる」ように `void**` で受ける作りなので、`reinterpret_cast` が要る。ビットをそのまま別の型として読む、一番危ないキャスト。`ComPtr` と `IID_PPV_ARGS` を使えば自分で書かずに済む

</details>

## 8. ASSERT

`ASSERT( 条件 && "説明の文字列" )` が 342 回ある。Debug ビルドで条件が偽なら、説明を出して止まる。

**読むときの問い**

1. `"説明"` は文字列なのに、なぜ `&&` でつなげられるか
2. `ASSERT` と `STRONG_ASSERT` の違いは（[ManagerImpl.h](../GameLib/src/Graphics/ManagerImpl.h) の `checkCreated` のコメント）
3. ASSERT に書いてよい条件と、書いてはいけない条件は

<details><summary>答え</summary>

1. 文字列リテラルはポインタで、nullptr ではないので常に真。`条件 && 真` は条件と同じ値になる。止まったときに、式ごと画面に出るので、説明が読める
2. `ASSERT` は Debug だけ、`STRONG_ASSERT` は Release でも止まる。デバイスやステートを作るのに失敗したら先へ進めないので `STRONG_ASSERT`
3. 書いてよいのは「プログラムが正しければ絶対に成り立つこと」（呼び方の間違い・前提の確認）。書いてはいけないのは「実行して起こり得ること」（ファイルが無い・ユーザーの入力）と、副作用のある式（Release では式ごと消えるので、`ASSERT( load() )` と書くと Release で読み込まれない）

</details>

## 課題

どれも新しい空のコンソールプロジェクトに、AI なしで書く。

### 課題 01-1：自前の UniquePtr

`template< class T > class UniquePtr` を書く。

- コンストラクタで生のポインタを受け取り、デストラクタで delete する
- コピーは禁止（`= delete`）。ムーブはできる（ムーブ元は nullptr になる）
- `get()`・`operator->`・`operator*`・`reset()` を持つ
- 確かめ方：コンストラクタとデストラクタで文字を出すクラスを入れて、「1 回作られ、1 回だけ消える」ことを見る。`UniquePtr< Foo > b = std::move( a );` の後の `a.get()` が nullptr であることを見る

### 課題 01-2：vector の再確保を見る

`std::vector< int > v;` に 1 個入れて `int* p = &v[ 0 ];` を取り、その後 1000 個 `push_back` して、`p` と `&v[ 0 ]` のアドレスを出して比べる。
`v.reserve( 2000 )` を先に呼ぶとどう変わるか。これが「足した後に前のポインタを使ってはいけない」の中身。

### 課題 01-3：小さな GameObject と Component

100 行ほどで、次の物を書く。先に自分の `GameObject.h` を閉じること。

- `class Component`（virtual な `update()` とデストラクタ）
- `class GameObject`：`addComponent< T >( 引数... )` と `getComponent< T >()`。部品は `std::vector< std::unique_ptr< Component > >` で持つ
- 種類の見分けは、まず `dynamic_cast` で書いてよい。動いたら `componentTypeId< T >()` の方法へ書き換える
- 確かめ方：`Position` と `Mover`（`Position` を毎回動かす）の 2 つの部品を作り、`Mover` が `getComponent< Position >()` で隣の部品を引いて動かす

書けたら、自分の [GameObject.h](../GameSrc/include/GameSrc/World/GameObject.h) と見比べる。自分の物に無くて、あちらにある物（`requireComponent`・`removeComponentLater`・`awake` と `start` の 2 段）が、何の問題を解いているかを書き出す。

### 課題 01-4：消して書き直す

学習用のブランチで、次の関数の中身を消して書き直す（やり方は README）。

- [FixedStepClock.cpp](../GameSrc/src/Core/FixedStepClock.cpp) の `ticksToRun`（02 章でやってもよい）
- [SceneRenderer.cpp](../GameSrc/src/Scene/Game/World/SceneRenderer.cpp) の `firstShaderError`（配列と範囲 for の練習。短い）

## 説明できるか

1. このゲームで、物の「持ち主」と「借りる人」をどう書き分けているか。`shared_ptr` を使っていない理由を言えるか
2. `unique_ptr` をムーブした後の元の変数はどうなるか
3. 基底クラスのデストラクタに `virtual` が要るのはどんなときか
4. `addComponent< T >( args... )` が、部品ごとに違う引数を受けられる仕組みを説明できるか
5. `dynamic_cast` を使わずに「同じ種類の部品」を探す仕組みを説明できるか
6. ヘッダに書いてよい物と、`.cpp` に書くべき物の線はどこか
