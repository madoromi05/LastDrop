#include "Pad.h"
#include "GameLib/GameLib.h"
using namespace GameLib::Input;

Pad* Pad::mInstance = nullptr;

void Pad::create() { ASSERT(!mInstance); mInstance = NEW Pad(); }
void Pad::destroy() { ASSERT(mInstance);  SAFE_DELETE(mInstance); }
Pad* Pad::instance() { return mInstance; }

// ────────────────────────────────────────────────
// コンストラクタ：builder を生成し、設定を読み込んで Input に反映する
// ────────────────────────────────────────────────
Pad::Pad()
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        m_builders[i] = std::make_unique<InputBuilder>(m_inputs[i]);
    }
    setupDevicesAndBindings();
}

Pad::~Pad() {}

void Pad::setupDevicesAndBindings()
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        loadOrDefault(i);
    }
}

void Pad::loadOrDefault(int playerId)
{
    // 保存済み設定があればそれを、無ければ GameLib 側のデフォルト表を採用する。
    // デフォルト割当（どのキーを何に）は GameLib が持つので Pad は番号で引くだけ。
    if (!m_configs[playerId].load(InputConfigFilePath(playerId)))
    {
        int count = 0;
        const BindingRecord* defaults = DefaultBindings(playerId, &count);
        m_configs[playerId].loadDefaults(defaults, count);
    }
    applyConfig(playerId);
}

void Pad::applyConfig(int playerId)
{
    // 設定（唯一の正）を実際の Input に反映する。
    // Bind がデバイス生成・物理キー登録・割当まで自己完結する。
    for (const auto& record : m_configs[playerId].records())
    {
        m_builders[playerId]->Bind(record);
    }
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
// Button 列挙 → WorkId への変換
// ────────────────────────────────────────────────
WorkId Pad::buttonToWorkId(Button b)
{
    switch (b)
    {
    case UP:    return WorkId::Up;
    case DOWN:  return WorkId::Down;
    case LEFT:  return WorkId::Left;
    case RIGHT: return WorkId::Right;
    case JUMP:  return WorkId::Jump;
    case FIRE:  return WorkId::Fire;
    case WIRE:     return WorkId::Wire;
    case WIREREEL: return WorkId::WireReel;
    case DODGE:    return WorkId::Dodge;
    case RUN:      return WorkId::Run;
    case MENU:     return WorkId::Menu;
    case AIM:      return WorkId::Aim;
    case HOLSTER:  return WorkId::Holster;
    case DEBUG_COLLISION: return WorkId::DebugCollision;
    default:    return WorkId::Up;
    }
}

// ────────────────────────────────────────────────
// 従来互換 API
// ────────────────────────────────────────────────
bool Pad::isOn(Button b, int playerId) const
{
    WorkId work = buttonToWorkId(b);
    return IsWorkPress(m_inputs[playerId], work) || IsWorkHold(m_inputs[playerId], work);
}

bool Pad::isTriggered(Button b, int playerId) const
{
    return IsWorkPress(m_inputs[playerId], buttonToWorkId(b));
}

// ────────────────────────────────────────────────
// 新 API
// ────────────────────────────────────────────────
short Pad::getWorkState(WorkId work, int playerId) const { return GetWorkState(m_inputs[playerId], work); }
bool  Pad::isWorkFree(WorkId work, int playerId) const { return IsWorkFree(m_inputs[playerId], work); }
bool  Pad::isWorkPress(WorkId work, int playerId) const { return IsWorkPress(m_inputs[playerId], work); }
bool  Pad::isWorkHold(WorkId work, int playerId) const { return IsWorkHold(m_inputs[playerId], work); }
bool  Pad::isWorkRelease(WorkId work, int playerId) const { return IsWorkRelease(m_inputs[playerId], work); }
AxisStatus Pad::getAxisWorkState(WorkId work, int playerId) const { return GetAxisWorkState(m_inputs[playerId], work); }

// ────────────────────────────────────────────────
// キーコンフィグ
//
// 設定（m_configs）を書き換え → Input へ反映 → ファイルへ保存、を1か所に集約。
// UI 側は「どの操作にどのキー」を渡すだけでよく、保存や反映は意識しなくてよい。
// ────────────────────────────────────────────────
void Pad::addKeyboardBindingToWork(WorkId work, char asciiKey, int playerId)
{
    BindingRecord record{ CollectorId::InGame, work, DeviceKind::Keyboard, asciiKey };
    m_configs[playerId].addBinding(record);
    m_builders[playerId]->Bind(record);
    m_configs[playerId].save(InputConfigFilePath(playerId));
}

void Pad::addMouseBindingToWork(WorkId work, Mouse::Button button, int playerId)
{
    BindingRecord record{ CollectorId::InGame, work, DeviceKind::Mouse, static_cast<int>(button) };
    m_configs[playerId].addBinding(record);
    m_builders[playerId]->Bind(record);
    m_configs[playerId].save(InputConfigFilePath(playerId));
}

void Pad::addJoystickBindingToWork(WorkId work, int buttonCode, int playerId)
{
    BindingRecord record{ CollectorId::InGame, work, DeviceKind::Joystick, buttonCode };
    m_configs[playerId].addBinding(record);
    m_builders[playerId]->Bind(record);
    m_configs[playerId].save(InputConfigFilePath(playerId));
}

void Pad::removeKeyboardBindingFromWork(WorkId work, char asciiKey, int playerId)
{
    BindingRecord record{ CollectorId::InGame, work, DeviceKind::Keyboard, asciiKey };
    m_configs[playerId].removeBinding(record);
    m_builders[playerId]->Unbind(record);
    m_configs[playerId].save(InputConfigFilePath(playerId));
}

void Pad::removeMouseBindingFromWork(WorkId work, Mouse::Button button, int playerId)
{
    BindingRecord record{ CollectorId::InGame, work, DeviceKind::Mouse, static_cast<int>(button) };
    m_configs[playerId].removeBinding(record);
    m_builders[playerId]->Unbind(record);
    m_configs[playerId].save(InputConfigFilePath(playerId));
}

void Pad::removeJoystickBindingFromWork(WorkId work, int buttonCode, int playerId)
{
    BindingRecord record{ CollectorId::InGame, work, DeviceKind::Joystick, buttonCode };
    m_configs[playerId].removeBinding(record);
    m_builders[playerId]->Unbind(record);
    m_configs[playerId].save(InputConfigFilePath(playerId));
}

void Pad::setCollectorActive(CollectorId collector, bool active)
{
    for (int i = 0; i < MAX_PLAYERS; ++i)
    {
        m_builders[i]->SetCollectorActive(collector, active);
    }
}

Input& Pad::getInput(int playerId)
{
    return m_inputs[playerId];
}

// ────────────────────────────────────────────────
// ゲームコード向けショートカット（アクション名＝関数名）
//   移動系は「押されている間」、Fire は「押した瞬間」を返す。
// ────────────────────────────────────────────────
bool InputUp   (int playerId) { return Pad::instance()->isOn(Pad::UP,    playerId); }
bool InputDown (int playerId) { return Pad::instance()->isOn(Pad::DOWN,  playerId); }
bool InputLeft (int playerId) { return Pad::instance()->isOn(Pad::LEFT,  playerId); }
bool InputRight(int playerId) { return Pad::instance()->isOn(Pad::RIGHT, playerId); }
bool InputJump (int playerId) { return Pad::instance()->isOn(Pad::JUMP,  playerId); }
bool InputFire (int playerId) { return Pad::instance()->isTriggered(Pad::FIRE, playerId); }
bool InputWire       (int playerId) { return Pad::instance()->isTriggered(Pad::WIRE, playerId); }
bool InputWireRelease(int playerId) { return Pad::instance()->isWorkRelease(WorkId::Wire, playerId); }
bool InputWireReel   (int playerId) { return Pad::instance()->isOn(Pad::WIREREEL, playerId); }
bool InputDodge      (int playerId) { return Pad::instance()->isTriggered(Pad::DODGE, playerId); }
bool InputRun        (int playerId) { return Pad::instance()->isOn(Pad::RUN, playerId); }
AxisStatus InputLook (int playerId) { return Pad::instance()->getAxisWorkState(WorkId::Look, playerId); }
bool InputMenu       (int playerId) { return Pad::instance()->isTriggered(Pad::MENU, playerId); }
bool InputAim        (int playerId) { return Pad::instance()->isOn(Pad::AIM, playerId); }
bool InputHolster    (int playerId) { return Pad::instance()->isTriggered(Pad::HOLSTER, playerId); }
bool InputDebugCollision(int playerId) { return Pad::instance()->isTriggered(Pad::DEBUG_COLLISION, playerId); }
