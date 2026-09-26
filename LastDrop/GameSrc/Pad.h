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
// Robo.cpp などのゲームコードは従来通り
//   Pad::instance()->isOn( Pad::JUMP, 0 )
// で呼べる。内部では Input クラスと InputBuilder を使っている。
//
// デバイス名・ボタン名・ワーク名などの文字列は GameLib::Input::InputBuilder
// （GameLib/src/Input/InputBuilder.cpp）が一元管理しており、
// Pad はキーコードと列挙型（WorkId / DeviceKind 等）だけを扱う。
// ────────────────────────────────────────────────
class Pad
{
public:
    // 従来互換の列挙型（Robo.cpp などを無変更にするため残す）
    enum Button { UP, DOWN, LEFT, RIGHT, JUMP, FIRE, WIRE, WIREREEL, DODGE, RUN, MENU, AIM, HOLSTER };

    static Pad* instance();
    static void create();
    static void destroy();

    // 毎フレーム Main で呼ぶ
    void update();

    // ─────────────────────────────────────────
    // 従来互換 API（Robo.cpp 等を変更せずに使える）
    // ─────────────────────────────────────────
    bool isOn(Button b, int playerId) const;
    bool isTriggered(Button b, int playerId) const;

    // ─────────────────────────────────────────
    // 新 API：4状態を直接取得したい場合
    // ─────────────────────────────────────────
    short getWorkState(GameLib::Input::WorkId work, int playerId) const;
    bool  isWorkFree(GameLib::Input::WorkId work, int playerId) const;
    bool  isWorkPress(GameLib::Input::WorkId work, int playerId) const;
    bool  isWorkHold(GameLib::Input::WorkId work, int playerId) const;
    bool  isWorkRelease(GameLib::Input::WorkId work, int playerId) const;
    AxisStatus getAxisWorkState(GameLib::Input::WorkId work, int playerId) const; // 軸ワークの今フレームの量

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

    // コレクター（シーン別マップ）の有効/無効切替
    void setCollectorActive(GameLib::Input::CollectorId collector, bool active);

    // Input インスタンスへの直接アクセス（上級者向け）
    ::Input& getInput(int playerId);

private:
    static const int MAX_PLAYERS = 2;

    Pad();
    ~Pad();

    void setupDevicesAndBindings();

    // 設定を読み込む（ファイルが無ければ GameLib のデフォルト表）→ Input へ反映
    void loadOrDefault(int playerId);
    // 現在の設定（m_configs）を builder 経由で Input に一括反映する
    void applyConfig(int playerId);

    // プレイヤーごとに独立した Input インスタンスを持つ
    ::Input m_inputs[MAX_PLAYERS];

    // プレイヤーごとの組み立て役（デバイス生成・キー登録・割当を自己完結）。
    // Input への参照を握るので生成後は破棄しない。
    std::unique_ptr<InputBuilder> m_builders[MAX_PLAYERS];

    // プレイヤーごとの割当設定（唯一の正。保存/読込・キーコンフィグの対象）
    GameLib::Input::InputConfig m_configs[MAX_PLAYERS];

    // Button 列挙 → WorkId への変換（従来 API の互換用）
    static GameLib::Input::WorkId buttonToWorkId(Button b);

    static Pad* mInstance;
};

// ────────────────────────────────────────────────
// ゲームコード向けショートカット（アクション名＝関数名）
//
// 文字列も enum も書かずに InputJump() のように入力を問い合わせる。
// ゲームロジック側は「何の操作か」だけを書けばよく、
// デバイス・ワーク名・状態種別は一切意識しなくてよい。
//
// 状態の意味は操作の性質に合わせて固定している：
//   移動系（Up/Down/Left/Right/Jump） … 押されている間ずっと true
//   Fire                              … 押した瞬間だけ true（トリガー）
//
// 内部では Pad::instance() の従来 API に委譲しているだけなので、
// 割り当て変更やデバイス追加の影響はここには波及しない。
// ────────────────────────────────────────────────
bool InputUp   (int playerId = 0);
bool InputDown (int playerId = 0);
bool InputLeft (int playerId = 0);
bool InputRight(int playerId = 0);
bool InputJump (int playerId = 0);
bool InputFire (int playerId = 0);
bool InputWire       (int playerId = 0);   // ワイヤーのグラップル開始（押した瞬間）
bool InputWireRelease(int playerId = 0);   // ワイヤー離脱（Wireボタンを離した瞬間）
bool InputWireReel   (int playerId = 0);   // 引き寄せ（押されている間・自然長を縮める）
bool InputDodge      (int playerId = 0);   // 回避ステップ（押した瞬間）
bool InputRun        (int playerId = 0);   // 走る（押されている間）
AxisStatus InputLook (int playerId = 0);   // 視点操作の量（横=右が正、縦=下が正。マウスなら画素数）
bool InputMenu       (int playerId = 0);   // メニュー／中断（押した瞬間）
bool InputAim        (int playerId = 0);   // 構える（押されている間）
bool InputHolster    (int playerId = 0);   // 武器を出す／しまう（押した瞬間）
