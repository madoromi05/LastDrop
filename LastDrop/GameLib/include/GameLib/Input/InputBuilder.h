#pragma once
#include <memory>
#include <string>
#include "Input.h"
#include "InputEnums.h"
#include "KeyboardDevice.h"
#include "MouseDevice.h"
#include "JoystickDevice.h"
#include "GamePadDevice.h"
#include "Mouse.h"

// ────────────────────────────────────────────────
// InputBuilder：Input を組み立てるためのゲーム側向け API
//
// デバイス名・フォーマット名・ボタン名・ワーク名などの
// 文字列は InputBuilder.cpp の中だけで管理する
// 呼び出し側（Pad.cpp 等）は列挙型・キーコードだけで
// 入力を組み立てられる
//
// 使い方：
//   InputBuilder builder(input);
//   builder.AddKeyboardDevice();
//   builder.AddKeyboardKey('w');
//   builder.AddCollector(CollectorId::InGame);
//   builder.BindKeyboard(CollectorId::InGame, WorkId::Up, 'w');
// ────────────────────────────────────────────────
class InputBuilder
{
public:
    explicit InputBuilder(::Input& a_input);

    // ─────────────────────────────────────────
    // デバイス登録
    // ─────────────────────────────────────────
    std::shared_ptr<KeyboardDevice> AddKeyboardDevice();
    std::shared_ptr<MouseDevice>    AddMouseDevice();
    std::shared_ptr<JoystickDevice> AddJoystickDevice();
    std::shared_ptr<GamePadDevice> AddGamePadDevice();

    // ─────────────────────────────────────────
    // 物理ボタン／軸の登録（名前は内部で自動生成される）
    // ─────────────────────────────────────────
    void AddKeyboardKey(char a_asciiKey);
    void AddMouseButton(GameLib::Input::Mouse::Button a_button);
    void AddMouseAxis(GameLib::Input::MouseAxisKind a_axis);
    void AddJoystickButton(int a_buttonCode);
    void AddJoystickAxis(GameLib::Input::JoystickAxisKind a_axis);

    // ─────────────────────────────────────────
    // コレクター
    // ─────────────────────────────────────────
    std::shared_ptr<InputCollector> AddCollector(GameLib::Input::CollectorId a_id);
    void SetCollectorActive(GameLib::Input::CollectorId a_id, bool a_active);

    // ─────────────────────────────────────────
    // ワークへのバインド／解除
    //
    // Bind 系はデバイス未登録なら自動生成し、物理キーも自動登録する
    // ゲーム側は AddXxxDevice / AddXxxKey を先に呼ばなくてよい
    // ─────────────────────────────────────────
    void BindKeyboard(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, char a_asciiKey);
    void BindMouse(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, GameLib::Input::Mouse::Button a_button);
    void BindJoystickButton(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, int a_buttonCode);
    // 軸ワーク（視点操作など、押下ではなく量を読む操作）へマウスの軸を割り当てる
    void BindMouseAxis(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, GameLib::Input::MouseAxisKind a_axis);
    void UnbindMouseAxis(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, GameLib::Input::MouseAxisKind a_axis);
    // パッド（GamePad）。ボタンは GamePad::Button の番号、スティックは軸ワークへ
    void BindGamePadButton(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, int a_buttonCode);
    void UnbindGamePadButton(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, int a_buttonCode);
    void BindGamePadAxis(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, GameLib::Input::GamePadAxisKind a_axis);
    void UnbindGamePadAxis(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, GameLib::Input::GamePadAxisKind a_axis);

    void UnbindKeyboard(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, char a_asciiKey);
    void UnbindMouse(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, GameLib::Input::Mouse::Button a_button);
    void UnbindJoystickButton(GameLib::Input::CollectorId a_collector, GameLib::Input::WorkId a_work, int a_buttonCode);

    // ─────────────────────────────────────────
    // レコード単位の適用（InputConfig から使う）
    // デバイス種別で上の Bind/Unbind へ振り分ける
    // ─────────────────────────────────────────
    void Bind(const GameLib::Input::BindingRecord& a_record);
    void Unbind(const GameLib::Input::BindingRecord& a_record);

private:
    // 各デバイス／フォーマット／コレクターを「無ければ生成、あれば取得」する
    // これにより Bind だけで入力系一式が組み上がる
    std::shared_ptr<KeyboardButtonFormat> ensureKeyboardFormat();
    std::shared_ptr<MouseButtonFormatInt> ensureMouseButtonFormat();
    std::shared_ptr<MouseAxisFormat>      ensureMouseAxisFormat();
    std::shared_ptr<JoystickButtonFormat> ensureJoystickButtonFormat();
    std::shared_ptr<GamePadButtonFormat>   ensureGamePadButtonFormat();
    std::shared_ptr<GamePadAxisFormat>     ensureGamePadAxisFormat();
    std::shared_ptr<InputCollector>       ensureCollector(GameLib::Input::CollectorId a_collector);

    ::Input& m_input;

    // AddXxxDevice() で作られたフォーマットを保持しておき、
    // AddXxxKey/Button/Axis() から参照する
    std::shared_ptr<KeyboardButtonFormat> m_keyboardKeys;
    std::shared_ptr<MouseButtonFormatInt> m_mouseButtons;
    std::shared_ptr<MouseAxisFormat>      m_mouseAxes;
    std::shared_ptr<JoystickButtonFormat> m_joystickButtons;
    std::shared_ptr<JoystickAxisFormat>   m_joystickAxes;
    std::shared_ptr<GamePadButtonFormat>   m_gamePadButtons;
    std::shared_ptr<GamePadAxisFormat>     m_gamePadAxes;
};

// ────────────────────────────────────────────────
// ワーク状態の問い合わせ（読み取り専用なのでフリー関数にしている）
//
// WorkId → ワーク名文字列の変換は InputBuilder.cpp が管理する
// ────────────────────────────────────────────────
namespace GameLib {
namespace Input {

// プレイヤー番号 → 入力設定ファイルのパス
// ファイル名という「文字列」も InputBuilder.cpp 側で一元管理し、
// ゲーム側（Pad.cpp 等）は番号だけを渡す
const char* InputConfigFilePath(int a_playerId);

// 操作（WorkId）の名前（例："Fire"）。保存ファイル・計測の印で使う。文字列リテラルなので持ち続けてよい
// 名前も InputBuilder.cpp 側で一元管理し、ゲーム側（Pad.cpp 等）は WorkId だけを渡す
const char* WorkIdName(WorkId a_work);

// 割り当て 1 件を、画面に見せるキーの名前にする（例："W"・"Shift"・"Mouse Left"・"Mouse"）。設定画面の操作の一覧で使う
// キーの名前も InputBuilder.cpp 側で一元管理し、ゲーム側は割り当て（BindingRecord）を渡すだけにする
std::string BindingDisplayName(const BindingRecord& a_record);

// パッドの割り当て 1 件を、PlayStation の表記で画面に見せる名前にする（例："R2"・"×"・"左スティック"）。設定画面のパッドの操作の一覧で使う
// 遊ぶパッドに DualSense を想定しているので PlayStation の表記にする。パッドの割り当て（GamePadButton・GamePadAxis）でなければ空
std::string GamePadBindingDisplayName(const BindingRecord& a_record);

// プレイヤー番号 → デフォルト割当表（先頭ポインタ）。要素数は a_outCount に返す
// 「どのキーを何に割り当てるか」という入力データも GameLib 側が持ち、
// ゲーム側（Pad.cpp）はキーコードも char も一切書かず番号で呼ぶだけにする
// 該当プレイヤーの表が無ければ nullptr / 0 を返す
const BindingRecord* DefaultBindings(int a_playerId, int* a_outCount);

short GetWorkState(const ::Input& a_input, WorkId a_work);
bool  IsWorkFree(const ::Input& a_input, WorkId a_work);
bool  IsWorkPress(const ::Input& a_input, WorkId a_work);
bool  IsWorkHold(const ::Input& a_input, WorkId a_work);
bool  IsWorkRelease(const ::Input& a_input, WorkId a_work);
AxisStatus GetAxisWorkState(const ::Input& a_input, WorkId a_work);	// 軸ワークの今フレームの量（横・縦。マウスなら移動した画素数）。割り当てが無ければ 0

}
}
