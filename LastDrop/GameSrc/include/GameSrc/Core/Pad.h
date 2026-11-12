#pragma once
#include <memory>
#include "GameLib/Input/Input.h"
#include "GameLib/Input/InputEnums.h"
#include "GameLib/Input/InputBuilder.h"
#include "GameLib/Input/InputConfig.h"
#include "GameLib/Input/KeyboardDevice.h"
#include "GameLib/Input/MouseDevice.h"
#include "GameLib/Input/JoystickDevice.h"
#include "GameLib/Input/Mouse.h"

// ────────────────────────────────────────────────
// Pad：GameLib の Input システムをこのゲーム用に組み立てるクラス
//
// ゲームコードは
//   InputRun() / InputFire()
// のようなショートカット（このファイルの下）で呼べる。内部では Input クラスと InputBuilder を使っている
//
// デバイス名・ボタン名・ワーク名などの文字列は GameLib::Input::InputBuilder
// （GameLib/src/Input/InputBuilder.cpp）が一元管理しており、
// Pad はキーコードと列挙型（WorkId / DeviceKind 等）だけを扱う
// ────────────────────────────────────────────────
class Pad
{
public:
    static Pad* instance();
    static void create();
    static void destroy();

    void update();	// 1回の描画につき1回、入力を読む（SceneManager が、シーンの beginFrame の前に呼ぶ）

    // ─────────────────────────────────────────
    // 操作（WorkId）の読み方。ショートカットが使う
    // ─────────────────────────────────────────
    bool isOn(GameLib::Input::WorkId work, int playerId) const;						// 押されている間ずっと
    bool isTriggered(GameLib::Input::WorkId work, int playerId) const;				// 押した瞬間だけ
    bool isWorkRelease(GameLib::Input::WorkId work, int playerId) const;			// 離した瞬間だけ
    AxisStatus getAxisWorkState(GameLib::Input::WorkId work, int playerId) const;	// 軸ワークの今フレームの量
    const std::vector<GameLib::Input::BindingRecord>& bindings(int playerId) const;	// 今の割り当て（設定画面の操作の一覧が、どのキーかを見せるのに使う。キーの名前は GameLib::Input::BindingDisplayName）

    // ─────────────────────────────────────────
    // キーコンフィグ：ワークへのボタン追加・削除
    // 例: addKeyboardBindingToWork(WorkId::Jump, ' ', 0)
    // ─────────────────────────────────────────
    void addKeyboardBindingToWork(GameLib::Input::WorkId work, char asciiKey, int playerId);
    void addMouseBindingToWork(GameLib::Input::WorkId work, GameLib::Input::Mouse::Button button, int playerId);
    void addJoystickBindingToWork(GameLib::Input::WorkId work, int buttonCode, int playerId);

    void removeKeyboardBindingFromWork(GameLib::Input::WorkId work, char asciiKey, int playerId);
    void removeMouseBindingFromWork(GameLib::Input::WorkId work, GameLib::Input::Mouse::Button button, int playerId);
    void removeJoystickBindingFromWork(GameLib::Input::WorkId work, int buttonCode, int playerId);

private:
    static const int MAX_PLAYERS = 2;

    Pad();
    ~Pad();

    void setupDevicesAndBindings();

    void loadOrDefault(int playerId);	// 設定を読み込む（ファイルが無ければ GameLib のデフォルト表）→ Input へ反映
    void applyConfig(int playerId);		// 現在の設定（m_configs）を builder 経由で Input に一括反映する

    ::Input m_inputs[MAX_PLAYERS];	// プレイヤーごとに独立した Input インスタンスを持つ

    // プレイヤーごとの組み立て役（デバイス生成・キー登録・割当を自己完結）
    // Input への参照を握るので生成後は破棄しない
    std::unique_ptr<InputBuilder> m_builders[MAX_PLAYERS];

    GameLib::Input::InputConfig m_configs[MAX_PLAYERS];	// プレイヤーごとの割当設定（唯一の正。保存/読込・キーコンフィグの対象）

    static Pad* mInstance;
};

// ────────────────────────────────────────────────
// ゲームコード向けショートカット（アクション名＝関数名）
//
// 文字列も enum も書かずに InputJump() のように入力を問い合わせる
// ゲームロジック側は「何の操作か」だけを書けばよく、
// デバイス・ワーク名・状態種別は一切意識しなくてよい
//
// 状態の意味は操作の性質に合わせて固定している：
//   移動系（Up/Down/Left/Right/Jump） … 押されている間ずっと true
//   Fire                              … 押した瞬間だけ true（トリガー）
//
// 内部では Pad::instance() に操作の番号（WorkId）を渡しているだけなので、
// 割り当て変更やデバイス追加の影響はここには波及しない
// ────────────────────────────────────────────────
bool InputUp   (int playerId = 0);
bool InputDown (int playerId = 0);
bool InputLeft (int playerId = 0);
bool InputRight(int playerId = 0);
bool InputJump (int playerId = 0);
bool InputFire (int playerId = 0);
bool InputFireHeld   (int playerId = 0);   // 撃つ（押されている間。連射用。InputFire と同じ Fire を読む）
bool InputDodge      (int playerId = 0);   // 回避ステップ（押した瞬間）
bool InputRun        (int playerId = 0);   // 走る（押されている間）
AxisStatus InputLook (int playerId = 0);   // 視点操作の量（横=右が正、縦=下が正。マウスなら画素数）
AxisStatus InputMove (int playerId = 0);   // パッドのスティックの移動（-1〜1。横=右が正、縦=下が正）。キーの InputUp などと別に読む
AxisStatus InputLookStick(int playerId = 0); // パッドのスティックの視点操作（-1〜1。横=右が正、縦=下が正）。傾き＝回る速さ
bool InputMenu       (int playerId = 0);   // メニュー／中断（押した瞬間）
bool InputAim        (int playerId = 0);   // 構える（押されている間）
bool InputHolster    (int playerId = 0);   // 武器を出す／しまう（押した瞬間）
// 確認用の切り替え（当たり判定・ゾンビの体力・観察用カメラ・無敵・ゾンビを出す・計測の CSV）はキーではなく F6 のボタン（DebugSwitches）
bool InputDebugCameraUp(int playerId = 0);     // 観察用カメラを上げる（押されている間）
bool InputDebugCameraDown(int playerId = 0);   // 観察用カメラを下げる（押されている間）
bool InputUiUp       (int playerId = 0);   // 画面の部品の選択を上へ（押した瞬間）
bool InputUiDown     (int playerId = 0);   // 画面の部品の選択を下へ（押した瞬間）
bool InputUiDecide   (int playerId = 0);   // 選んでいる部品を決定する（押した瞬間）
bool InputUiLeft     (int playerId = 0);   // 選んでいる部品の値を下げる（押した瞬間。設定画面のスライダー）
bool InputUiRight    (int playerId = 0);   // 選んでいる部品の値を上げる（押した瞬間）
bool InputInteract    (int playerId = 0);   // 見ている物を使う（拾う。押した瞬間）
bool InputReload      (int playerId = 0);   // 弾を込める（押した瞬間）
bool InputFlashlight  (int playerId = 0);   // 銃の懐中電灯を付ける／消す（押した瞬間）
bool InputInventory   (int playerId = 0);   // カバンの画面を開く／閉じる（押した瞬間）
bool InputQuickPlace  (int playerId = 0);   // 拾った物をおまかせでカバンへ入れる（押した瞬間。拾った物を持っている間だけ読む）
