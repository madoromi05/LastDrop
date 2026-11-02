#include "GameLib/Input/InputBuilder.h"
#include "GameLib/Input/Joystick.h"
#include "GameLib/Input/Keyboard.h"
#include "GameLib/Input/GamePad.h"
#include <cctype>

using namespace GameLib::Input;

// ────────────────────────────────────────────────
// 文字列管理はすべてこのファイルの中に閉じ込める。
// ゲーム側（Pad.cpp 等）はこれらの文字列を一切知らなくてよい。
// ────────────────────────────────────────────────
namespace
{
    const char* kFormatKeys = "Keys";
    const char* kFormatButtons = "Buttons";
    const char* kFormatAxes = "Axes";

    std::string DeviceKindName(DeviceKind a_device)
    {
        switch (a_device)
        {
        case DeviceKind::Keyboard: return "Keyboard";
        case DeviceKind::Mouse:    return "Mouse";
        case DeviceKind::Joystick: return "Joystick";
        // ボタンと軸は 1 つのデバイス（マウスと同じく、種別だけ分けて同じ名前）。
        case DeviceKind::GamePadButton:
        case DeviceKind::GamePadAxis: return "GamePad";
        default:                  return "";
        }
    }

    // 文字列リテラルを返す（計測の印は名前のポインタを持ち続けるので、std::string にしない）。
    const char* WorkName(WorkId a_work)
    {
        switch (a_work)
        {
        case WorkId::Up:    return "Up";
        case WorkId::Down:  return "Down";
        case WorkId::Left:  return "Left";
        case WorkId::Right: return "Right";
        case WorkId::Jump:  return "Jump";
        case WorkId::Fire:  return "Fire";
        case WorkId::Dodge:    return "Dodge";
        case WorkId::Run:      return "Run";
        case WorkId::Look:     return "Look";
        case WorkId::Menu:     return "Menu";
        case WorkId::Aim:      return "Aim";
        case WorkId::Holster:  return "Holster";
        case WorkId::DebugCollision: return "DebugCollision";
        case WorkId::DebugPickupCollision: return "DebugPickupCollision";
        case WorkId::UiUp:     return "UiUp";
        case WorkId::UiDown:   return "UiDown";
        case WorkId::UiDecide: return "UiDecide";
        case WorkId::Interact: return "Interact";
        case WorkId::DebugZombieHealth: return "DebugZombieHealth";
        case WorkId::DebugSpawnZombie: return "DebugSpawnZombie";
        case WorkId::DebugFreeCamera: return "DebugFreeCamera";
        case WorkId::DebugCameraUp:   return "DebugCameraUp";
        case WorkId::DebugCameraDown: return "DebugCameraDown";
        case WorkId::Reload:   return "Reload";
        case WorkId::DebugInvincible: return "DebugInvincible";
        case WorkId::DebugRecordPerf: return "DebugRecordPerf";
        case WorkId::UiLeft:   return "UiLeft";
        case WorkId::UiRight:  return "UiRight";
        case WorkId::Flashlight: return "Flashlight";
        case WorkId::Move:       return "Move";
        case WorkId::LookStick:  return "LookStick";
        default:            return "";
        }
    }

    std::string CollectorName(CollectorId a_id)
    {
        switch (a_id)
        {
        case CollectorId::InGame: return "InGame";
        default:                  return "";
        }
    }

    // キーコードを GameLib のキーボードへ渡す値にする。
    // Keyboard::Key の特殊キー（0x80 以降）は char だと負になるので、符号なしで読み直す。
    int KeyboardKeyCode(char a_asciiKey)
    {
        return static_cast<unsigned char>(a_asciiKey);
    }

    // 'w' -> "W" のように大文字化する。空白やカンマなど印字しにくい文字・特殊キーは個別に名前をつける。
    std::string KeyboardKeyName(char a_asciiKey)
    {
        switch (KeyboardKeyCode(a_asciiKey))
        {
        case ' ': return "Space";
        case ',': return "Comma";
        case '.': return "Period";
        case Keyboard::KEY_SHIFT:   return "Shift";
        case Keyboard::KEY_CONTROL: return "Control";
        case Keyboard::KEY_ESC:     return "Esc";
        case Keyboard::KEY_F1:      return "F1";
        default:  return std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(a_asciiKey))));
        }
    }

    std::string MouseButtonName(Mouse::Button a_button)
    {
        switch (a_button)
        {
        case Mouse::BUTTON_LEFT:   return "Left";
        case Mouse::BUTTON_RIGHT:  return "Right";
        case Mouse::BUTTON_MIDDLE: return "Middle";
        default:                   return "";
        }
    }

    // 上下左右は Joystick::Button の固定コード、それ以外は 0 始まりの番号ボタン。
    std::string JoystickButtonName(int a_buttonCode)
    {
        switch (a_buttonCode)
        {
        case Joystick::BUTTON_UP:    return "Up";
        case Joystick::BUTTON_DOWN:  return "Down";
        case Joystick::BUTTON_LEFT:  return "Left";
        case Joystick::BUTTON_RIGHT: return "Right";
        default:                     return "Btn" + std::to_string(a_buttonCode);
        }
    }

    std::string MouseAxisName(MouseAxisKind a_axis)
    {
        switch (a_axis)
        {
        case MouseAxisKind::Move:  return "Move";
        case MouseAxisKind::Wheel: return "Wheel";
        default:                   return "";
        }
    }

    std::string JoystickAxisName(JoystickAxisKind a_axis)
    {
        switch (a_axis)
        {
        case JoystickAxisKind::LeftStick: return "LeftStick";
        default:                          return "";
        }
    }

    std::string GamePadButtonName(int a_buttonCode)
    {
        switch (a_buttonCode)
        {
        case GamePad::BUTTON_A:           return "A";
        case GamePad::BUTTON_B:           return "B";
        case GamePad::BUTTON_X:           return "X";
        case GamePad::BUTTON_Y:           return "Y";
        case GamePad::BUTTON_LB:          return "LB";
        case GamePad::BUTTON_RB:          return "RB";
        case GamePad::BUTTON_BACK:        return "Back";
        case GamePad::BUTTON_START:       return "Start";
        case GamePad::BUTTON_LEFT_THUMB:  return "LS";
        case GamePad::BUTTON_RIGHT_THUMB: return "RS";
        case GamePad::BUTTON_DPAD_UP:     return "Up";
        case GamePad::BUTTON_DPAD_DOWN:   return "Down";
        case GamePad::BUTTON_DPAD_LEFT:   return "Left";
        case GamePad::BUTTON_DPAD_RIGHT:  return "Right";
        case GamePad::BUTTON_LT:          return "LT";
        case GamePad::BUTTON_RT:          return "RT";
        default:                            return "";
        }
    }

    std::string GamePadAxisName(GamePadAxisKind a_axis)
    {
        switch (a_axis)
        {
        case GamePadAxisKind::LeftStick:  return "Left Stick";
        case GamePadAxisKind::RightStick: return "Right Stick";
        default:                         return "";
        }
    }

    // 入力設定ファイル名（プレイヤー番号で引く）
    const char* kInputConfigPaths[] = { "input_p0.xml", "input_p1.xml" };
    const int   kInputConfigPathCount = static_cast<int>(sizeof(kInputConfigPaths) / sizeof(kInputConfigPaths[0]));

    template<class T, int N>
    constexpr int countOf(const T(&)[N]) { return N; }

    // ────────────────────────────────────────────────
    // デフォルト割当表（このゲームの初期操作）。
    // 「手続きの羅列」ではなく「データの表」。行を足す／消すだけで割当が変わる。
    // ここが入力データの一元管理場所で、ゲーム側は番号で引くだけ。
    // ────────────────────────────────────────────────

    // Player 0：キーボード＋マウス＋パッド
    const BindingRecord kDefaultsP0[] = {
        { CollectorId::InGame, WorkId::Up,    DeviceKind::Keyboard, 'w' },
        { CollectorId::InGame, WorkId::Down,  DeviceKind::Keyboard, 's' },
        { CollectorId::InGame, WorkId::Left,  DeviceKind::Keyboard, 'a' },
        { CollectorId::InGame, WorkId::Right, DeviceKind::Keyboard, 'd' },
        { CollectorId::InGame, WorkId::Jump,  DeviceKind::Keyboard, ' ' },
        { CollectorId::InGame, WorkId::Fire,  DeviceKind::Mouse,    Mouse::BUTTON_LEFT },
        { CollectorId::InGame, WorkId::Dodge, DeviceKind::Keyboard, ' ' },
        { CollectorId::InGame, WorkId::Run,   DeviceKind::Keyboard, Keyboard::KEY_SHIFT },
        { CollectorId::InGame, WorkId::Look,  DeviceKind::MouseAxis, static_cast<int>(MouseAxisKind::Move) },
        { CollectorId::InGame, WorkId::Menu,  DeviceKind::Keyboard, Keyboard::KEY_ESC },
        { CollectorId::InGame, WorkId::Aim,   DeviceKind::Mouse,    Mouse::BUTTON_RIGHT },
        // 数字キーで主武器を出し入れする（Escape from Tarkov と同じ）。
        { CollectorId::InGame, WorkId::Holster, DeviceKind::Keyboard, '1' },
        // R で弾を込める（FPS でよくある割り当て。Escape from Tarkov・バイオハザードと同じ）。
        { CollectorId::InGame, WorkId::Reload,  DeviceKind::Keyboard, 'r' },
        // T で銃の懐中電灯を付ける／消す（Escape from Tarkov のタクティカルデバイスと同じ）。
        { CollectorId::InGame, WorkId::Flashlight, DeviceKind::Keyboard, 't' },
        // 開発中の確認用。遊ぶ操作と重ならないファンクションキーに置く。
        { CollectorId::InGame, WorkId::DebugCollision, DeviceKind::Keyboard, Keyboard::KEY_F1 },
        { CollectorId::InGame, WorkId::DebugPickupCollision, DeviceKind::Keyboard, Keyboard::KEY_F2 },
        { CollectorId::InGame, WorkId::DebugZombieHealth, DeviceKind::Keyboard, Keyboard::KEY_F3 },
        { CollectorId::InGame, WorkId::DebugSpawnZombie, DeviceKind::Keyboard, Keyboard::KEY_F9 },
        // 観察用カメラの上下は Space・Ctrl（Space は Jump/Dodge と共有。観察用カメラの間は自キャラを動かさないので重ならない）。
        { CollectorId::InGame, WorkId::DebugFreeCamera, DeviceKind::Keyboard, Keyboard::KEY_F4 },
        { CollectorId::InGame, WorkId::DebugCameraUp,   DeviceKind::Keyboard, ' ' },
        { CollectorId::InGame, WorkId::DebugCameraDown, DeviceKind::Keyboard, Keyboard::KEY_CONTROL },
        // 画面の部品（タイトル・クリア画面のボタン）の操作。矢印キーと WASD の両方で選べるようにする
        // （W/S は移動の Up/Down と共有。ボタンの画面ではキャラを動かさないので重ならない）。
        { CollectorId::InGame, WorkId::UiUp,     DeviceKind::Keyboard, Keyboard::KEY_UP },
        { CollectorId::InGame, WorkId::UiUp,     DeviceKind::Keyboard, 'w' },
        { CollectorId::InGame, WorkId::UiDown,   DeviceKind::Keyboard, Keyboard::KEY_DOWN },
        { CollectorId::InGame, WorkId::UiDown,   DeviceKind::Keyboard, 's' },
        { CollectorId::InGame, WorkId::UiDecide, DeviceKind::Keyboard, Keyboard::KEY_RETURN },
        { CollectorId::InGame, WorkId::UiDecide, DeviceKind::Keyboard, ' ' },
        // 設定画面のスライダーの値を変える。上下と同じく矢印キーと WASD の両方で
        // （A/D は移動の Left/Right と共有。設定の画面ではキャラを動かさないので重ならない）。
        { CollectorId::InGame, WorkId::UiLeft,   DeviceKind::Keyboard, Keyboard::KEY_LEFT },
        { CollectorId::InGame, WorkId::UiLeft,   DeviceKind::Keyboard, 'a' },
        { CollectorId::InGame, WorkId::UiRight,  DeviceKind::Keyboard, Keyboard::KEY_RIGHT },
        { CollectorId::InGame, WorkId::UiRight,  DeviceKind::Keyboard, 'd' },
        // 見ている物を使う（Escape from Tarkov と同じ F キーではなく、よくある E キー）。
        { CollectorId::InGame, WorkId::Interact, DeviceKind::Keyboard, 'e' },
		// デバッグ用。F5 で自キャラの無敵を切り替える（押した瞬間。Debug ビルドだけで効く）。
        { CollectorId::InGame, WorkId::DebugInvincible, DeviceKind::Keyboard, Keyboard::KEY_F5 },
        // 計測の CSV の記録を始める／止める（2026-10-23 決定。F6 は ImGui のエディタに空けておく）。
        { CollectorId::InGame, WorkId::DebugRecordPerf, DeviceKind::Keyboard, Keyboard::KEY_F7 },

        // ── パッド（ボタンは Xbox の並び。DualSense なども対応表で同じ並びに置き換えて読む） ──
        // タルコフは PC 版にパッド対応が無いので、よくある FPS の配置（LT 構える・RT 撃つ）に合わせる。
        // Jump・Dodge は今どこからも読まれていないので割り当てない（A は使う・決定に回す）。
        { CollectorId::InGame, WorkId::Move,       DeviceKind::GamePadAxis,   static_cast<int>(GamePadAxisKind::LeftStick) },
        { CollectorId::InGame, WorkId::LookStick,  DeviceKind::GamePadAxis,   static_cast<int>(GamePadAxisKind::RightStick) },
        { CollectorId::InGame, WorkId::Run,        DeviceKind::GamePadButton, GamePad::BUTTON_LEFT_THUMB },
        { CollectorId::InGame, WorkId::Fire,       DeviceKind::GamePadButton, GamePad::BUTTON_RT },
        { CollectorId::InGame, WorkId::Aim,        DeviceKind::GamePadButton, GamePad::BUTTON_LT },
        { CollectorId::InGame, WorkId::Holster,    DeviceKind::GamePadButton, GamePad::BUTTON_Y },
        { CollectorId::InGame, WorkId::Reload,     DeviceKind::GamePadButton, GamePad::BUTTON_X },
        { CollectorId::InGame, WorkId::Interact,   DeviceKind::GamePadButton, GamePad::BUTTON_A },
        { CollectorId::InGame, WorkId::Flashlight, DeviceKind::GamePadButton, GamePad::BUTTON_LB },
        { CollectorId::InGame, WorkId::Menu,       DeviceKind::GamePadButton, GamePad::BUTTON_START },
        // 画面の部品は十字キーで選び A で決める（A は Interact と共有。ボタンの画面ではキャラを動かさないので重ならない）。
        { CollectorId::InGame, WorkId::UiUp,       DeviceKind::GamePadButton, GamePad::BUTTON_DPAD_UP },
        { CollectorId::InGame, WorkId::UiDown,     DeviceKind::GamePadButton, GamePad::BUTTON_DPAD_DOWN },
        { CollectorId::InGame, WorkId::UiLeft,     DeviceKind::GamePadButton, GamePad::BUTTON_DPAD_LEFT },
        { CollectorId::InGame, WorkId::UiRight,    DeviceKind::GamePadButton, GamePad::BUTTON_DPAD_RIGHT },
        { CollectorId::InGame, WorkId::UiDecide,   DeviceKind::GamePadButton, GamePad::BUTTON_A },
    };

    // Player 1：キーボードのみ
    const BindingRecord kDefaultsP1[] = {
        { CollectorId::InGame, WorkId::Up,    DeviceKind::Keyboard, 'i' },
        { CollectorId::InGame, WorkId::Down,  DeviceKind::Keyboard, 'm' },
        { CollectorId::InGame, WorkId::Left,  DeviceKind::Keyboard, 'j' },
        { CollectorId::InGame, WorkId::Right, DeviceKind::Keyboard, 'k' },
        { CollectorId::InGame, WorkId::Jump,  DeviceKind::Keyboard, 'l' },
        { CollectorId::InGame, WorkId::Fire,  DeviceKind::Keyboard, ',' },
    };
}

const char* GameLib::Input::WorkIdName(WorkId a_work)
{
    return WorkName(a_work);
}

std::string GameLib::Input::BindingDisplayName(const BindingRecord& a_record)
{
    switch (a_record.device)
    {
    case DeviceKind::Keyboard:  return KeyboardKeyName(static_cast<char>(a_record.code));
    case DeviceKind::Mouse:     return "Mouse " + MouseButtonName(static_cast<Mouse::Button>(a_record.code));
    case DeviceKind::Joystick:  return "Pad " + JoystickButtonName(a_record.code);
    case DeviceKind::MouseAxis:
        // 視点はマウスを動かすことなので、軸の名前（Move）ではなくただ「Mouse」と見せる。
        return static_cast<MouseAxisKind>(a_record.code) == MouseAxisKind::Move ? "Mouse" : "Mouse " + MouseAxisName(static_cast<MouseAxisKind>(a_record.code));
    case DeviceKind::GamePadButton: return "Pad " + GamePadButtonName(a_record.code);
    case DeviceKind::GamePadAxis:   return "Pad " + GamePadAxisName(static_cast<GamePadAxisKind>(a_record.code));
    default:                    return "";
    }
}

const char* GameLib::Input::InputConfigFilePath(int a_playerId)
{
    if (a_playerId < 0 || a_playerId >= kInputConfigPathCount) return "";
    return kInputConfigPaths[a_playerId];
}

const BindingRecord* GameLib::Input::DefaultBindings(int a_playerId, int* a_outCount)
{
    switch (a_playerId)
    {
    case 0: *a_outCount = countOf(kDefaultsP0); return kDefaultsP0;
    case 1: *a_outCount = countOf(kDefaultsP1); return kDefaultsP1;
    default: *a_outCount = 0; return nullptr;
    }
}

InputBuilder::InputBuilder(::Input& a_input)
    : m_input(a_input)
{
}

std::shared_ptr<KeyboardDevice> InputBuilder::AddKeyboardDevice()
{
    auto device = m_input.AddDevice<KeyboardDevice>(DeviceKindName(DeviceKind::Keyboard));
    m_keyboardKeys = device->AddInputButtonFormat<KeyboardButtonFormat>(kFormatKeys);
    return device;
}

std::shared_ptr<MouseDevice> InputBuilder::AddMouseDevice()
{
    auto device = m_input.AddDevice<MouseDevice>(DeviceKindName(DeviceKind::Mouse));
    m_mouseButtons = device->AddInputButtonFormat<MouseButtonFormatInt>(kFormatButtons);
    m_mouseAxes = device->AddInputAxisFormat<MouseAxisFormat>(kFormatAxes);
    return device;
}

std::shared_ptr<JoystickDevice> InputBuilder::AddJoystickDevice(int a_joystickIndex)
{
    auto device = m_input.AddDevice<JoystickDevice>(DeviceKindName(DeviceKind::Joystick));
    m_joystickButtons = device->AddInputButtonFormat<JoystickButtonFormat>(kFormatButtons);
    m_joystickAxes = device->AddInputAxisFormat<JoystickAxisFormat>(kFormatAxes);
    (void)a_joystickIndex; // JoystickButtonFormat/JoystickAxisFormat のプレイヤーID指定は今後の拡張ポイント
    return device;
}

std::shared_ptr<GamePadDevice> InputBuilder::AddGamePadDevice()
{
    // ボタンと軸は同じデバイス名（"GamePad"）の下に置く（DeviceKindName は 2 つの種別に同じ名前を返す）。
    auto device = m_input.AddDevice<GamePadDevice>(DeviceKindName(DeviceKind::GamePadButton));
    m_gamePadButtons = device->AddInputButtonFormat<GamePadButtonFormat>(kFormatButtons);
    m_gamePadAxes = device->AddInputAxisFormat<GamePadAxisFormat>(kFormatAxes);
    return device;
}

void InputBuilder::AddKeyboardKey(char a_asciiKey)
{
    if (!m_keyboardKeys) return;
    m_keyboardKeys->AddButton(KeyboardKeyName(a_asciiKey), a_asciiKey);
}

void InputBuilder::AddMouseButton(Mouse::Button a_button)
{
    if (!m_mouseButtons) return;
    m_mouseButtons->AddButton(MouseButtonName(a_button), static_cast<int>(a_button));
}

void InputBuilder::AddMouseAxis(MouseAxisKind a_axis)
{
    if (!m_mouseAxes) return;
    m_mouseAxes->AddAxis(MouseAxisName(a_axis), static_cast<int>(a_axis));
}

void InputBuilder::AddJoystickButton(int a_buttonCode)
{
    if (!m_joystickButtons) return;
    m_joystickButtons->AddButton(JoystickButtonName(a_buttonCode), a_buttonCode);
}

void InputBuilder::AddJoystickAxis(JoystickAxisKind a_axis)
{
    if (!m_joystickAxes) return;
    m_joystickAxes->AddAxis(JoystickAxisName(a_axis), static_cast<int>(a_axis));
}

std::shared_ptr<InputCollector> InputBuilder::AddCollector(CollectorId a_id)
{
    return m_input.AddCollector(CollectorName(a_id));
}

void InputBuilder::SetCollectorActive(CollectorId a_id, bool a_active)
{
    auto col = m_input.GetCollector(CollectorName(a_id));
    if (col) col->SetActive(a_active);
}

// ─────────────────────────────────────────
// 「無ければ生成、あれば取得」ヘルパ
// Bind だけで入力系一式が組み上がるようにするための土台。
// ─────────────────────────────────────────
std::shared_ptr<KeyboardButtonFormat> InputBuilder::ensureKeyboardFormat()
{
    if (!m_keyboardKeys) AddKeyboardDevice();
    return m_keyboardKeys;
}

std::shared_ptr<MouseButtonFormatInt> InputBuilder::ensureMouseButtonFormat()
{
    if (!m_mouseButtons) AddMouseDevice();
    return m_mouseButtons;
}

std::shared_ptr<MouseAxisFormat> InputBuilder::ensureMouseAxisFormat()
{
    if (!m_mouseAxes) AddMouseDevice();
    return m_mouseAxes;
}

std::shared_ptr<JoystickButtonFormat> InputBuilder::ensureJoystickButtonFormat()
{
    if (!m_joystickButtons) AddJoystickDevice();
    return m_joystickButtons;
}

std::shared_ptr<GamePadButtonFormat> InputBuilder::ensureGamePadButtonFormat()
{
    if (!m_gamePadButtons) AddGamePadDevice();
    return m_gamePadButtons;
}

std::shared_ptr<GamePadAxisFormat> InputBuilder::ensureGamePadAxisFormat()
{
    if (!m_gamePadAxes) AddGamePadDevice();
    return m_gamePadAxes;
}

std::shared_ptr<InputCollector> InputBuilder::ensureCollector(CollectorId a_collector)
{
    auto col = m_input.GetCollector(CollectorName(a_collector));
    if (!col) col = m_input.AddCollector(CollectorName(a_collector));
    return col;
}

void InputBuilder::BindKeyboard(CollectorId a_collector, WorkId a_work, char a_asciiKey)
{
    // ① デバイス生成と物理キー登録をこの中で完結させる（二重宣言を撤廃）
    ensureKeyboardFormat()->AddButton(KeyboardKeyName(a_asciiKey), KeyboardKeyCode(a_asciiKey));
    ensureCollector(a_collector)->AddButtonToButtonWork(
        WorkName(a_work), DeviceKindName(DeviceKind::Keyboard), KeyboardKeyName(a_asciiKey));
}

void InputBuilder::BindMouse(CollectorId a_collector, WorkId a_work, Mouse::Button a_button)
{
    ensureMouseButtonFormat()->AddButton(MouseButtonName(a_button), static_cast<int>(a_button));
    ensureCollector(a_collector)->AddButtonToButtonWork(
        WorkName(a_work), DeviceKindName(DeviceKind::Mouse), MouseButtonName(a_button));
}

void InputBuilder::BindJoystickButton(CollectorId a_collector, WorkId a_work, int a_buttonCode)
{
    ensureJoystickButtonFormat()->AddButton(JoystickButtonName(a_buttonCode), a_buttonCode);
    ensureCollector(a_collector)->AddButtonToButtonWork(
        WorkName(a_work), DeviceKindName(DeviceKind::Joystick), JoystickButtonName(a_buttonCode));
}

void InputBuilder::BindMouseAxis(CollectorId a_collector, WorkId a_work, MouseAxisKind a_axis)
{
    // 軸はボタンと別の「軸ワーク」に登録する（値は押下状態ではなく移動量）。
    ensureMouseAxisFormat()->AddAxis(MouseAxisName(a_axis), static_cast<int>(a_axis));
    ensureCollector(a_collector)->AddAxisToAxisWork(
        WorkName(a_work), DeviceKindName(DeviceKind::Mouse), MouseAxisName(a_axis));
}

void InputBuilder::UnbindMouseAxis(CollectorId a_collector, WorkId a_work, MouseAxisKind a_axis)
{
    auto col = m_input.GetCollector(CollectorName(a_collector));
    if (col) col->RemoveAxisFromAxisWork(WorkName(a_work), DeviceKindName(DeviceKind::Mouse), MouseAxisName(a_axis));
}

void InputBuilder::BindGamePadButton(CollectorId a_collector, WorkId a_work, int a_buttonCode)
{
    ensureGamePadButtonFormat()->AddButton(GamePadButtonName(a_buttonCode), a_buttonCode);
    ensureCollector(a_collector)->AddButtonToButtonWork(
        WorkName(a_work), DeviceKindName(DeviceKind::GamePadButton), GamePadButtonName(a_buttonCode));
}

void InputBuilder::UnbindGamePadButton(CollectorId a_collector, WorkId a_work, int a_buttonCode)
{
    auto col = m_input.GetCollector(CollectorName(a_collector));
    if (col) col->RemoveButtonFromButtonWork(WorkName(a_work), DeviceKindName(DeviceKind::GamePadButton), GamePadButtonName(a_buttonCode));
}

void InputBuilder::BindGamePadAxis(CollectorId a_collector, WorkId a_work, GamePadAxisKind a_axis)
{
    // スティックはマウスの軸と同じく「軸ワーク」に登録する（値は押下状態ではなく傾き）。
    ensureGamePadAxisFormat()->AddAxis(GamePadAxisName(a_axis), static_cast<int>(a_axis));
    ensureCollector(a_collector)->AddAxisToAxisWork(
        WorkName(a_work), DeviceKindName(DeviceKind::GamePadAxis), GamePadAxisName(a_axis));
}

void InputBuilder::UnbindGamePadAxis(CollectorId a_collector, WorkId a_work, GamePadAxisKind a_axis)
{
    auto col = m_input.GetCollector(CollectorName(a_collector));
    if (col) col->RemoveAxisFromAxisWork(WorkName(a_work), DeviceKindName(DeviceKind::GamePadAxis), GamePadAxisName(a_axis));
}

void InputBuilder::UnbindKeyboard(CollectorId a_collector, WorkId a_work, char a_asciiKey)
{
    auto col = m_input.GetCollector(CollectorName(a_collector));
    if (col) col->RemoveButtonFromButtonWork(WorkName(a_work), DeviceKindName(DeviceKind::Keyboard), KeyboardKeyName(a_asciiKey));
}

void InputBuilder::UnbindMouse(CollectorId a_collector, WorkId a_work, Mouse::Button a_button)
{
    auto col = m_input.GetCollector(CollectorName(a_collector));
    if (col) col->RemoveButtonFromButtonWork(WorkName(a_work), DeviceKindName(DeviceKind::Mouse), MouseButtonName(a_button));
}

void InputBuilder::UnbindJoystickButton(CollectorId a_collector, WorkId a_work, int a_buttonCode)
{
    auto col = m_input.GetCollector(CollectorName(a_collector));
    if (col) col->RemoveButtonFromButtonWork(WorkName(a_work), DeviceKindName(DeviceKind::Joystick), JoystickButtonName(a_buttonCode));
}

// ─────────────────────────────────────────
// レコード1件を適用／解除（デバイス種別で振り分け）
// ─────────────────────────────────────────
void InputBuilder::Bind(const BindingRecord& a_record)
{
    switch (a_record.device)
    {
    case DeviceKind::Keyboard:
        BindKeyboard(a_record.collector, a_record.work, static_cast<char>(a_record.code));
        break;
    case DeviceKind::Mouse:
        BindMouse(a_record.collector, a_record.work, static_cast<Mouse::Button>(a_record.code));
        break;
    case DeviceKind::Joystick:
        BindJoystickButton(a_record.collector, a_record.work, a_record.code);
        break;
    case DeviceKind::MouseAxis:
        BindMouseAxis(a_record.collector, a_record.work, static_cast<MouseAxisKind>(a_record.code));
        break;
    case DeviceKind::GamePadButton:
        BindGamePadButton(a_record.collector, a_record.work, a_record.code);
        break;
    case DeviceKind::GamePadAxis:
        BindGamePadAxis(a_record.collector, a_record.work, static_cast<GamePadAxisKind>(a_record.code));
        break;
    }
}

void InputBuilder::Unbind(const BindingRecord& a_record)
{
    switch (a_record.device)
    {
    case DeviceKind::Keyboard:
        UnbindKeyboard(a_record.collector, a_record.work, static_cast<char>(a_record.code));
        break;
    case DeviceKind::Mouse:
        UnbindMouse(a_record.collector, a_record.work, static_cast<Mouse::Button>(a_record.code));
        break;
    case DeviceKind::Joystick:
        UnbindJoystickButton(a_record.collector, a_record.work, a_record.code);
        break;
    case DeviceKind::MouseAxis:
        UnbindMouseAxis(a_record.collector, a_record.work, static_cast<MouseAxisKind>(a_record.code));
        break;
    case DeviceKind::GamePadButton:
        UnbindGamePadButton(a_record.collector, a_record.work, a_record.code);
        break;
    case DeviceKind::GamePadAxis:
        UnbindGamePadAxis(a_record.collector, a_record.work, static_cast<GamePadAxisKind>(a_record.code));
        break;
    }
}

short GameLib::Input::GetWorkState(const ::Input& a_input, WorkId a_work)
{
    return a_input.GetButtonWorkState(WorkName(a_work));
}

bool GameLib::Input::IsWorkFree(const ::Input& a_input, WorkId a_work) { return a_input.IsButtonWorkFree(WorkName(a_work)); }
bool GameLib::Input::IsWorkPress(const ::Input& a_input, WorkId a_work) { return a_input.IsButtonWorkPress(WorkName(a_work)); }
bool GameLib::Input::IsWorkHold(const ::Input& a_input, WorkId a_work) { return a_input.IsButtonWorkHold(WorkName(a_work)); }
bool GameLib::Input::IsWorkRelease(const ::Input& a_input, WorkId a_work) { return a_input.IsButtonWorkRelease(WorkName(a_work)); }

AxisStatus GameLib::Input::GetAxisWorkState(const ::Input& a_input, WorkId a_work)
{
    return a_input.GetAxisWorkStatus(WorkName(a_work));
}
