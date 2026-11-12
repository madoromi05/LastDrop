# tools

ゲームの外で走らせる道具。どれも作業フォルダをプロジェクトの `LastDrop` にして動かす（使い方は各ファイルの頭のコメント）。
2026-11-09 に種類ごとのフォルダへ分けた。ファイルを足したら、ここにも 1 行足す。

| フォルダ | 中身 | 動かし方 |
|---|---|---|
| `convert/` | 外から持ってきた物を、ゲームが読む形にする | |
| 　fbx_to_gamelib.py | FBX → GameLib の Scene 形式（skinned / static）。設定は `configs/*.json` | Blender |
| 　fbx_ascii_to_binary.py | ASCII FBX → Blender が読めるバイナリ FBX | Blender |
| 　atlas_materials.py | マテリアルごとの画像を 1 枚にまとめ、UV を移す（`configs/*_atlas.json`） | Blender |
| 　orm_gen.py | 材質の画像（AO・粗さ・金属）を作る（`configs/orm/*.json`） | Blender |
| 　texture_to_dds.ps1 | Resource/Texture の TGA → BC7 / BC5 の DDS | PowerShell＋texconv |
| `texgen/` | エフェクト・UI の絵を数字から作る（種が固定で、何度作っても同じ絵） | Blender の python.exe |
| 　tga_image.py | 共通の部品（重ね方・TGA の書き出し）。ほかの *_gen.py が import する | |
| 　bullet_hole_gen.py・blood_gen.py・blood_stain_gen.py | 弾痕・血しぶき・血の跡 | |
| 　dust_gen.py・smoke_flipbook_gen.py・muzzle_flash_gen.py | 砂ぼこり・その 16 コマ・マズルフラッシュ | |
| 　moon_gen.py・puddle_gen.py・interact_mark_gen.py | 月・水たまりの模様・使える物のマーク | |
| 　cursor_gen.py | 窓の中のマウスカーソル（Resource/UI/cursor.cur。32・48・64 画素の 3 枚） | |
| `modelgen/` | Blender で形・アニメ・アイコンを作る | Blender |
| 　loot_prop_gen.py | ゴミ箱・木箱・包帯などの形と画像（出力は `generated/loot_props/`） | |
| 　pistol_reload_gen.py | 拳銃の構えとリロードの腕のアニメ（`configs/pistol_reload.json`） | |
| 　item_icon_gen.py | 3D モデルを撮ってカバンの絵にする（`configs/item_icons.json`） | |
| `render_check/` | 確かめ用。ゲームには使わない（絵は `pose_compare_out/` に出る） | Blender |
| 　pose_compare_render.py・ads_view_render.py | 一人称の腕の構え・拳銃を覗いた絵を描いて比べる | |
| `SoundGen/` | 音の合成（C++ の exe。ソリューションに入っている） | ビルドして実行 |
| `configs/` | 上のツールの設定（モデル・画像ごとの json） | |
| `generated/` | ツールが作った途中の物（loot_prop_gen.py の FBX） | |
