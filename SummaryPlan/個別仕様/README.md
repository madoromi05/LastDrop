# 個別仕様（機能ごとの計画書）

機能ごとに「計画書を書く → Q を★で承認 → 実装 → 画面確認 → 計測」の順で進める。その計画書の置き場所。
進み具合の全体は `../main.txt`、計測した数字は `../計測の記録.md` を見る。

計画書を足したら、下の種類のフォルダのどれかに入れて、この一覧にも足す。
コードのコメントから計画書を指すときは `SummaryPlan/個別仕様/<フォルダ>/<名前>.txt` と書く。
取り下げた案・作らないと決めた案・役目を終えた手順書は消す（中身は git の履歴に残る）。後の決定で古くなった所は、冒頭に「今の正はどこか」を書き、消えた仕組みの節は削る。

| フォルダ | 中身 | 全体の計画 |
|---|---|---|
| `グラフィック/` | 描き方の段階1〜12（リニア化・ディファード・PBR・影・霧・SSAO・SSR・TAA・月と LUT・血の跡・材質・Hi-Z）、段階13 ボリュームフォグ（コードに無い・保留）、ブルーム・街灯の模型と光（StreetLamp） | `Graphics_Spec.txt`（段階1〜8）・`GraphicsExtra_Spec.txt`（段階9〜13） |
| `最適化と計測/` | 最適化【S】【A】【B】【C】、ライトの間引き（`LightCulling_Plan.txt`）・シェーダの覚え（`ShaderDiskCache_Plan.txt`）・置物のまとめ描き（`PropInstancing_Plan.txt`）、計測の仕組み（Profile 構成・F6 の Perf 欄）、計測の道具の案（AutoPlay と PerfCompare を `AutoPlayBench_Plan.txt` に 1 本。保留） | `Optimization_Plan.txt` |
| `射撃と手応え/` | マズルフラッシュ・弾痕・煙のコマ送り・カメラシェイクと反動・照準のぶれ・手応えの強弱・着弾音・拳銃 | — |
| `ゾンビ/` | 追う・道探し（NavMesh）・聞く（銃声と足音）・湧く・攻撃・体力と欠損・よろけ・這う・群れ・ルートモーション・不確かさ | — |
| `出撃と帰還/` | 今の出撃の形は 1 回勝負（`SimpleRun_Plan.txt`）。マス目のカバン・拾う・漁れる場所（R3）・走るゾンビ（R4）・日ごとの条件（ふつう・停電・霧・群れ）・脱出の車と給油口。5 日間の作り（RaidLoop・R2）は経緯として残す | `SimpleRun_Plan.txt`（経緯は `RaidLoop_Spec.txt`） |
| `UI/` | 一時停止・設定画面（タブ・パッドの操作と感度・明るさ Brightness_Plan.txt は実装・画面確認待ち）・キーの割り当て（作らない。保存の直しだけ）・日本語化・物の絵・使える物のマーク・弾数・タイトル画面・メニューを RE:2 風に揃える・結果の画面・マウスカーソル・タイトルと結果の画面の BGM（Bgm_Plan.txt） | — |
| `設計と分割/` | ComponentSystem（2・3）、GameWorld・Zombie（3・4 回目）・ゾンビの判断・大きいクラスの分割（高・中）・更新の順番、小さな計算の置き場所、マス目の移動の重複をまとめる（ItemGridMove_Plan.txt・未着手）、部品にする範囲を広げる 5 つ（1 明かり LightComponent_Plan.txt・2 銃を 1 つの物に WeaponObject_Plan.txt・3 持ち物を自キャラの部品に InventoryComponent_Plan.txt・4 見ている物を使うを自キャラの部品に InteractorComponent_Plan.txt・5 千切れた部位を物に SeveredLimbObject_Plan.txt）、素材を 1 つのファイルにまとめて配る（ResourcePack_Plan.txt・実装・画面確認待ち） | — |

`射撃と手応え/images/` は拳銃の構えを比べた絵（`PistolAnimation_Plan.txt` から指している）。
