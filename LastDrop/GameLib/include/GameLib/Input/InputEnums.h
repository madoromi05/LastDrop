#pragma once

// ────────────────────────────────────────────────
// 入力ビルダー層：ゲーム側が文字列を書かずに済むように
// するための列挙型。
//
// 実際のデバイス名／ボタン名／ワーク名の文字列は
// InputBuilder.cpp が一元管理し、ゲーム側（Pad.cpp 等）は
// ここに定義された列挙型だけを扱えばよい。
// ────────────────────────────────────────────────
namespace GameLib {
namespace Input {

// デバイス種別
enum class DeviceKind
{
    Keyboard,
    Mouse,
    Joystick,
    MouseAxis, // マウスの軸（移動量・ホイール）。code は MouseAxisKind。ボタンと code が重なるので種別を分ける
               // （保存ファイルは番号で持つので、途中に挿さず末尾へ足すこと）
};

// 入力ワーク（ゲームのアクション）種別
enum class WorkId
{
    Up,
    Down,
    Left,
    Right,
    Jump,
    Fire,
    Wire,     // 立体機動ワイヤー（押下保持でグラップル、離すと離脱）
    WireReel, // ワイヤー引き寄せ（保持中は自然長を縮めて手前へ引かれる）
    Dodge,    // 回避ステップ
    Run,      // 走る（押している間）
    Look,     // 視点操作（軸ワーク。GetAxisWorkState で横・縦の移動量を読む）
    Menu,     // メニュー／中断（カーソルロックの解除にも使う）
    Aim,      // 構える（押している間）。保存ファイルは番号で持つので、新しいワークは末尾へ足す
    Holster,  // 武器を出す／しまう（押した瞬間に切り替え）
    DebugCollision, // 当たり判定の表示を切り替える（押した瞬間。Debug ビルドだけで効く）
    DebugPickupCollision, // 落ちている物（燃料など）の当たり判定を、壁を透かして表示する切り替え（押した瞬間。Debug ビルドだけで効く）
    UiUp,     // 画面の部品（ボタンなど）の選択を上へ（押した瞬間）
    UiDown,   // 画面の部品の選択を下へ（押した瞬間）
    UiDecide, // 選んでいる部品を決定する（押した瞬間）
    Interact, // 見ている物を使う（拾う・給油する。押した瞬間）
    DebugZombieHealth, // ゾンビの体力（全体・部位）を頭の上に表示する切り替え（押した瞬間。Debug ビルドだけで効く）
};

// コレクター（シーン別の入力割り当てグループ）種別
enum class CollectorId
{
    InGame,
};

// マウス軸種別
enum class MouseAxisKind
{
    Move = 0,
    Wheel = 1,
};

// ジョイスティック軸種別
enum class JoystickAxisKind
{
    LeftStick = 0,
};

// ────────────────────────────────────────────────
// バインド1件を表すデータ（保存/読込・デフォルト表の最小単位）
//
// 「どのコレクターの・どのワークを・どのデバイスの・どのコードで」発動するか。
// code は物理コードの生値：
//   Keyboard … ASCII（'w' など）
//   Mouse    … Mouse::Button の整数値
//   Joystick … ボタンコード（Joystick::BUTTON_UP など。A/B は 0,1）
//
// enum とコードだけで表現でき、そのまま整数として保存できるので
// InputConfig がファイルへ書き出す最小単位になる。
// ────────────────────────────────────────────────
struct BindingRecord
{
    CollectorId collector;
    WorkId      work;
    DeviceKind  device;
    int         code;
};

}
}
