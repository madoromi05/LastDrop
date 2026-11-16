# LAST DROP

夜の町でガソリンを探し、ゾンビから逃げ切って車で脱出する一人称のゾンビシューターです。
C++ と DirectX11 で作っています（ゲームエンジンは使っていません）。

## ゲーム概要

| 項目 | 内容 |
|---|---|
| ジャンル | 一人称のゾンビシューター（PvE・1 回勝負） |
| 目的 | 町のどこかの箱にあるガソリンを見つけ、脱出用の車の給油口に入れ、そばで 5 秒耐えると脱出 |
| 毎回変わる物 | ガソリンの入った箱・車の場所（北か南）・町の状態（ふつう／停電／霧／雨／群れ） |
| 持ち物 | 拳銃と 9mm の弾から始まり、ライフル・弾・包帯は町で漁って手に入れる（マス目のカバンで管理） |
| ゾンビ | 足音と銃声を聞いて寄ってくる。部位ごとに体力があり、頭が一番効く（ライフルで 3 発・拳銃で 5 発ほど。たまに 1 発で倒れる）。脚を撃つと這って追ってくる |
| 終わり方 | 脱出でクリア、死ぬとゲームオーバー。どちらも結果画面にかかった時間と倒した数が出る |
| 言語・API | C++20 / DirectX11 / HLSL |
| 開発 | 個人制作（2026 年 5 月〜） |

## 操作説明

![操作説明（設定 → 操作 の画面）](docs/images/controls.webp)

- 上の画面は、ゲーム内の **設定 → 操作** でいつでも見られます。
- ゲームパッド（Xbox 系・DualSense）にも対応しています。割り当ては同じ画面の「パッド」で確認できます。
- `Alt` + `Enter` でフルスクリーンとウィンドウを切り替えます。

## ゲームの始め方

### 必要な物
- Windows 10 / 11（64 bit）
- Visual Studio 2026（ワークロード「C++ によるデスクトップ開発」。Windows SDK を含む）
- DirectX11 が動く GPU

### ビルドと起動
1. このリポジトリをクローンします。
2. `LastDrop.slnx` を Visual Studio で開きます。
3. 構成を **Release / x64** にします。
4. `Ctrl` + `F5`（デバッグなしで開始）で起動します。

`Resource/` を作業フォルダからの相対パスで読むため、Visual Studio から起動してください。
exe を直接起動する場合は、`LastDrop/` フォルダを作業フォルダにしてください。

### 遊び方
1. タイトルで **START** を選ぶと、町へ出撃します。
2. 始めの数秒に、その回の町の状態と脱出の車の方角が出ます。
3. ゴミ箱や木箱を `E` で漁り、ガソリン・弾・包帯を集めます。
4. 車の給油口にガソリンを入れ、そばで 5 秒耐えると脱出です。

## ライセンス表記

### ライブラリ

| 名前 | ライセンス | 同梱場所 |
|---|---|---|
| [Dear ImGui](https://github.com/ocornut/imgui) | MIT | `GameLib/src/ThirdParty/imgui/LICENSE.txt` |
| [miniaudio](https://miniaud.io/) | Public Domain または MIT No Attribution | `GameLib/src/ThirdParty/miniaudio/LICENSE` |
| [Recast & Detour](https://github.com/recastnavigation/recastnavigation) | zlib | `GameLib/src/ThirdParty/recastnavigation/License.txt` |

### 素材

- "G2 Combat Pistol" by Michael Karel, licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) (modified)
- "modular kit city builder starter kit" by Pieter Ferreira, licensed under [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) (modified)
- "Street Lights PBR" by wagner-daviana, CGTrader Royalty Free License
- "Post Apocalyptic UAZ" by BLND-studio, CGTrader Royalty Free License
- "Metal Jerrycan" by Sean Buckley・"Fire Hydrant" by Gonçalo Felício：[Poly Haven](https://polyhaven.com/)（CC0）
- キャラクターとアニメーション：[Adobe Mixamo](https://www.mixamo.com/)
- 銃声・リロード・命中の効果音：Adobe Audition の効果音素材集ほか（一覧は [`Resource/Sound/SoundReadme.md`](LastDrop/Resource/Sound/SoundReadme.md)）
- 足音・環境音などの効果音と、弾痕・血・砂ぼこり・マズルフラッシュの絵：自作
