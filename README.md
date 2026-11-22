# LAST DROP

夜の町でガソリンを探し、ゾンビから逃げ切って車で脱出する一人称のゾンビシューターです。


## ゲーム概要

| 項目 | 内容 |
|---|---|
| ジャンル | 一人称のゾンビシューター（PvE・1 回勝負） |
| 目的 | 町のどこかの箱にあるガソリンを見つけ、脱出用の車の給油口に入れ、そばで 5 秒耐えると脱出 |

## 操作説明

![操作説明（設定 → 操作 の画面）](docs/images/controls.webp)

- ゲームパッド（Xbox 系・DualSense）にも対応しています。割り当ては同じ画面の「パッド」で確認できます。

## ゲームの始め方

### 必要な物
- Windows 10 / 11（64 bit）
- DirectX11 が動く GPU

### ビルドと起動

1. [Releases](https://github.com/madoromi05/LastDrop/releases) から、一番新しい版の `LastDrop_v1.0.zip` をダウンロードします。
2. zip を展開します
3. 展開した `LastDrop` フォルダの `LastDrop.exe` を起動します。

### ソースコードについて
このリポジトリライセンスにより、素材（モデル・画像・音）は入っていません

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
