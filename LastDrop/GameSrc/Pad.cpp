#include "Pad.h"
#include "GameLib/GameLib.h"
#include "GameLib/Input/Mouse.h"
using namespace GameLib::Input;

Pad* Pad::mInstance = nullptr;

void Pad::create() { ASSERT(!mInstance); mInstance = NEW Pad(); }
void Pad::destroy() { ASSERT(mInstance);  SAFE_DELETE(mInstance); }
Pad* Pad::instance() { return mInstance; }

// ────────────────────────────────────────────────
// コンストラクタ：デバイスとデフォルト割り当てを構築する
// ────────────────────────────────────────────────
Pad::Pad()
{
    setupDevicesAndBindings();
}

Pad::~Pad() {}

void Pad::setupDevicesAndBindings()
{
    // ── Player 0 ──────────────────────────────

    // デバイスを登録
    auto kb0 = m_inputs[0].AddDevice<KeyboardDevice>("Keyboard");
    auto mo0 = m_inputs[0].AddDevice<MouseDevice>("Mouse");
    auto joy0 = m_inputs[0].AddDevice<JoystickDevice>("Joystick");

    // キーボードのボタンフォーマットを登録
    auto kbFmt0 = kb0->AddInputButtonFormat<KeyboardButtonFormat>("Keys");
    kbFmt0->AddButton("W", 'w');
    kbFmt0->AddButton("S", 's');
    kbFmt0->AddButton("A", 'a');
    kbFmt0->AddButton("D", 'd');
    kbFmt0->AddButton("Space", ' ');

    // マウスのボタンフォーマットを登録
    auto moFmt0 = mo0->AddInputButtonFormat<MouseButtonFormatInt>("Buttons");
    moFmt0->AddButton("Left", static_cast<int>(Mouse::BUTTON_LEFT));
    moFmt0->AddButton("Right", static_cast<int>(Mouse::BUTTON_RIGHT));

    // マウスの軸フォーマットを登録
    auto moAxis0 = mo0->AddInputAxisFormat<MouseAxisFormat>("Axes");
    moAxis0->AddAxis("Move", 0);
    moAxis0->AddAxis("Wheel", 1);

    // ジョイスティックのフォーマットを登録（プレイヤー 0）
    auto joyFmt0 = joy0->AddInputButtonFormat<JoystickButtonFormat>("Buttons");
    joyFmt0->AddButton("Up", Joystick::BUTTON_UP);
    joyFmt0->AddButton("Down", Joystick::BUTTON_DOWN);
    joyFmt0->AddButton("Left", Joystick::BUTTON_LEFT);
    joyFmt0->AddButton("Right", Joystick::BUTTON_RIGHT);
    joyFmt0->AddButton("A", 0);
    joyFmt0->AddButton("B", 1);

    // ジョイスティック軸
    auto joyAxis0 = joy0->AddInputAxisFormat<JoystickAxisFormat>("Axes");
    joyAxis0->AddAxis("LeftStick", 0);

    // コレクター（ゲーム中の割り当て）
    auto col0 = m_inputs[0].AddCollector("InGame");
    col0->AddButtonToButtonWork(WORK_UP, "Keyboard", "W");
    col0->AddButtonToButtonWork(WORK_UP, "Joystick", "Up");
    col0->AddButtonToButtonWork(WORK_DOWN, "Keyboard", "S");
    col0->AddButtonToButtonWork(WORK_DOWN, "Joystick", "Down");
    col0->AddButtonToButtonWork(WORK_LEFT, "Keyboard", "A");
    col0->AddButtonToButtonWork(WORK_LEFT, "Joystick", "Left");
    col0->AddButtonToButtonWork(WORK_RIGHT, "Keyboard", "D");
    col0->AddButtonToButtonWork(WORK_RIGHT, "Joystick", "Right");
    col0->AddButtonToButtonWork(WORK_JUMP, "Keyboard", "Space");
    col0->AddButtonToButtonWork(WORK_JUMP, "Joystick", "A");
    col0->AddButtonToButtonWork(WORK_FIRE, "Mouse", "Left");
    col0->AddButtonToButtonWork(WORK_FIRE, "Joystick", "B");

    // ── Player 1 ──────────────────────────────

    auto kb1 = m_inputs[1].AddDevice<KeyboardDevice>("Keyboard");
    auto joy1 = m_inputs[1].AddDevice<JoystickDevice>("Joystick");

    auto kbFmt1 = kb1->AddInputButtonFormat<KeyboardButtonFormat>("Keys");
    kbFmt1->AddButton("I", 'i');
    kbFmt1->AddButton("M", 'm');
    kbFmt1->AddButton("J", 'j');
    kbFmt1->AddButton("K", 'k');
    kbFmt1->AddButton("L", 'l');
    kbFmt1->AddButton("Comma", ',');

    // ジョイスティック（プレイヤー 1 = インデックス 1）
    auto joyFmt1 = joy1->AddInputButtonFormat<JoystickButtonFormat>("Buttons");
    // JoystickButtonFormat のデフォルト引数は 0 なので、1 を渡す必要がある
    // → AddInputButtonFormat は引数なしで make_shared するため、別途 playerId を設定できるよう
    //    下記のように直接 shared_ptr を差し替える（または JoystickDevice に id を持たせる）
    // ※ シンプルにするため Player1 の Joystick は省略し、キーボードのみとする
    //    （JoystickButtonFormat にプレイヤー ID を渡す設計改善は拡張ポイント）

    auto col1 = m_inputs[1].AddCollector("InGame");
    col1->AddButtonToButtonWork(WORK_UP, "Keyboard", "I");
    col1->AddButtonToButtonWork(WORK_DOWN, "Keyboard", "M");
    col1->AddButtonToButtonWork(WORK_LEFT, "Keyboard", "J");
    col1->AddButtonToButtonWork(WORK_RIGHT, "Keyboard", "K");
    col1->AddButtonToButtonWork(WORK_JUMP, "Keyboard", "L");
    col1->AddButtonToButtonWork(WORK_FIRE, "Keyboard", "Comma");
}

// ────────────────────────────────────────────────
// 毎フレーム更新
// ────────────────────────────────────────────────
void Pad::update()
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        m_inputs[i].Update();
    }
}

// ────────────────────────────────────────────────
// Button 列挙 → ワーク名への変換
// ────────────────────────────────────────────────
const char* Pad::buttonToWorkName(Button b)
{
    switch (b)
    {
    case UP:    return WORK_UP;
    case DOWN:  return WORK_DOWN;
    case LEFT:  return WORK_LEFT;
    case RIGHT: return WORK_RIGHT;
    case JUMP:  return WORK_JUMP;
    case FIRE:  return WORK_FIRE;
    default:    return "";
    }
}

// ────────────────────────────────────────────────
// 従来互換 API
// ────────────────────────────────────────────────
bool Pad::isOn(Button b, int playerId) const
{
    const char* work = buttonToWorkName(b);
    short s = m_inputs[playerId].GetButtonWorkState(work);
    return (s & KeyStatus::Press) || (s & KeyStatus::Hold);
}

bool Pad::isTriggered(Button b, int playerId) const
{
    return m_inputs[playerId].GetButtonWorkState(buttonToWorkName(b)) & KeyStatus::Press;
}

// ────────────────────────────────────────────────
// 新 API
// ────────────────────────────────────────────────
short Pad::getWorkState(const char* workName) const
{
    return m_inputs[0].GetButtonWorkState(workName);
}

bool Pad::isWorkFree(const char* w) const { return m_inputs[0].IsButtonWorkFree(w); }
bool Pad::isWorkPress(const char* w) const { return m_inputs[0].IsButtonWorkPress(w); }
bool Pad::isWorkHold(const char* w) const { return m_inputs[0].IsButtonWorkHold(w); }
bool Pad::isWorkRelease(const char* w) const { return m_inputs[0].IsButtonWorkRelease(w); }

// ────────────────────────────────────────────────
// キーコンフィグ
// ────────────────────────────────────────────────
void Pad::addButtonToWork(const char* work, const char* device, const char* button, int playerId)
{
    auto col = m_inputs[playerId].GetCollector("InGame");
    if (col) col->AddButtonToButtonWork(work, device, button);
}

void Pad::removeButtonFromWork(const char* work, const char* device, const char* button, int playerId)
{
    auto col = m_inputs[playerId].GetCollector("InGame");
    if (col) col->RemoveButtonFromButtonWork(work, device, button);
}

void Pad::setCollectorActive(const char* collectorName, bool active)
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        auto col = m_inputs[i].GetCollector(collectorName);
        if (col) col->SetActive(active);
    }
}

Input& Pad::getInput(int playerId)
{
    return m_inputs[playerId];
}