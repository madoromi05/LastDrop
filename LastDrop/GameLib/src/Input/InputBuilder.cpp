#include "GameLib/Input/InputBuilder.h"
#include "GameLib/Input/Joystick.h"
#include "GameLib/Input/Keyboard.h"
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
        default:                  return "";
        }
    }

    std::string WorkName(WorkId a_work)
    {
        switch (a_work)
        {
        case WorkId::Up:    return "Up";
        case WorkId::Down:  return "Down";
        case WorkId::Left:  return "Left";
        case WorkId::Right: return "Right";
        case WorkId::Jump:  return "Jump";
        case WorkId::Fire:  return "Fire";
        case WorkId::Wire:     return "Wire";
        case WorkId::WireReel: return "WireReel";
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

    // Player 0：キーボード＋マウス＋ジョイスティック
    const BindingRecord kDefaultsP0[] = {
        { CollectorId::InGame, WorkId::Up,    DeviceKind::Keyboard, 'w' },
        { CollectorId::InGame, WorkId::Up,    DeviceKind::Joystick, Joystick::BUTTON_UP },
        { CollectorId::InGame, WorkId::Down,  DeviceKind::Keyboard, 's' },
        { CollectorId::InGame, WorkId::Down,  DeviceKind::Joystick, Joystick::BUTTON_DOWN },
        { CollectorId::InGame, WorkId::Left,  DeviceKind::Keyboard, 'a' },
        { CollectorId::InGame, WorkId::Left,  DeviceKind::Joystick, Joystick::BUTTON_LEFT },
        { CollectorId::InGame, WorkId::Right, DeviceKind::Keyboard, 'd' },
        { CollectorId::InGame, WorkId::Right, DeviceKind::Joystick, Joystick::BUTTON_RIGHT },
        { CollectorId::InGame, WorkId::Jump,  DeviceKind::Keyboard, ' ' },
        { CollectorId::InGame, WorkId::Jump,  DeviceKind::Joystick, 0 },              // A
        { CollectorId::InGame, WorkId::Fire,  DeviceKind::Mouse,    Mouse::BUTTON_LEFT },
        { CollectorId::InGame, WorkId::Fire,  DeviceKind::Joystick, 1 },              // B
        { CollectorId::InGame, WorkId::Wire,  DeviceKind::Keyboard, 'q' },
        { CollectorId::InGame, WorkId::Wire,  DeviceKind::Mouse,    Mouse::BUTTON_RIGHT },
        { CollectorId::InGame, WorkId::Wire,  DeviceKind::Joystick, 5 },              // RB
        { CollectorId::InGame, WorkId::WireReel, DeviceKind::Keyboard, 'e' },
        { CollectorId::InGame, WorkId::WireReel, DeviceKind::Joystick, 3 },           // Y
        { CollectorId::InGame, WorkId::Dodge, DeviceKind::Keyboard, ' ' },
        { CollectorId::InGame, WorkId::Dodge, DeviceKind::Joystick, 4 },              // LB
        { CollectorId::InGame, WorkId::Run,   DeviceKind::Keyboard, Keyboard::KEY_SHIFT },
        { CollectorId::InGame, WorkId::Run,   DeviceKind::Joystick, 2 },              // X
        { CollectorId::InGame, WorkId::Look,  DeviceKind::MouseAxis, static_cast<int>(MouseAxisKind::Move) },
        { CollectorId::InGame, WorkId::Menu,  DeviceKind::Keyboard, Keyboard::KEY_ESC },
        { CollectorId::InGame, WorkId::Menu,  DeviceKind::Joystick, 7 },              // Start
        // 右クリックは Wire（v4 アクション）と共有する。両方を使うシーンが無いため（Space の Jump/Dodge と同じ扱い）。
        { CollectorId::InGame, WorkId::Aim,   DeviceKind::Mouse,    Mouse::BUTTON_RIGHT },
        // 数字キーで主武器を出し入れする（Escape from Tarkov と同じ）。
        { CollectorId::InGame, WorkId::Holster, DeviceKind::Keyboard, '1' },
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
        { CollectorId::InGame, WorkId::UiDecide, DeviceKind::Joystick, 0 },  // A
        // 見ている物を使う（Escape from Tarkov と同じ F キーではなく、よくある E キー）。E は旧アクションの WireReel と共有
        // （WireReel を使うシーンはもう無い）。
        { CollectorId::InGame, WorkId::Interact, DeviceKind::Keyboard, 'e' },
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
