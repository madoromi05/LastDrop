#include "GameSrc/Core/Pad.h"
#include "GameLib/GameLib.h"
#include "GameLib/Profiling/Profiler.h"
using namespace GameLib::Input;

namespace {
    // 計測の印を付ける操作の持ち主（自キャラを動かすのは 0 番だけ）。
    const int kProfiledPlayer = 0;
}

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
    // 保存済み設定を読めたときも、保存した後に足した操作（ファイルに無い操作）はデフォルト表で補う。
    int count = 0;
    const BindingRecord* defaults = DefaultBindings(playerId, &count);
    if (m_configs[playerId].load(InputConfigFilePath(playerId)))
    {
        m_configs[playerId].addMissingWorks(defaults, count);
    }
    else
    {
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
#ifdef ZF_EDITOR
    // 押した操作を計測の印にする（どのボタンを押した時に重いかを、CSV・スパイクの JSON・右上の表示で見るため）。
    // 名前は GameLib が持つ（ここには文字列を書かない）。割り当てが無い操作・軸の操作（Look）は押されたことにならない。
    for (int work = 0; work < kWorkIdCount; ++work)
    {
        WorkId id = static_cast<WorkId>(work);
        if (IsWorkPress(m_inputs[kProfiledPlayer], id))
        {
            PROFILE_EVENT(WorkIdName(id));
        }
    }
#endif
}

// ────────────────────────────────────────────────
// 操作（WorkId）の読み方
// ────────────────────────────────────────────────
bool Pad::isOn(WorkId work, int playerId) const
{
    return IsWorkPress(m_inputs[playerId], work) || IsWorkHold(m_inputs[playerId], work);
}

bool Pad::isTriggered(WorkId work, int playerId) const
{
    return IsWorkPress(m_inputs[playerId], work);
}

bool Pad::isWorkRelease(WorkId work, int playerId) const { return IsWorkRelease(m_inputs[playerId], work); }
AxisStatus Pad::getAxisWorkState(WorkId work, int playerId) const { return GetAxisWorkState(m_inputs[playerId], work); }
const std::vector<BindingRecord>& Pad::bindings(int playerId) const { return m_configs[playerId].records(); }

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

// ────────────────────────────────────────────────
// ゲームコード向けショートカット（アクション名＝関数名）
//   移動系は「押されている間」、Fire は「押した瞬間」を返す。
// ────────────────────────────────────────────────
bool InputUp   (int playerId) { return Pad::instance()->isOn(WorkId::Up, playerId); }
bool InputDown (int playerId) { return Pad::instance()->isOn(WorkId::Down, playerId); }
bool InputLeft (int playerId) { return Pad::instance()->isOn(WorkId::Left, playerId); }
bool InputRight(int playerId) { return Pad::instance()->isOn(WorkId::Right, playerId); }
bool InputJump (int playerId) { return Pad::instance()->isOn(WorkId::Jump, playerId); }
bool InputFire (int playerId) { return Pad::instance()->isTriggered(WorkId::Fire, playerId); }
bool InputFireHeld   (int playerId) { return Pad::instance()->isOn(WorkId::Fire, playerId); }
bool InputDodge      (int playerId) { return Pad::instance()->isTriggered(WorkId::Dodge, playerId); }
bool InputRun        (int playerId) { return Pad::instance()->isOn(WorkId::Run, playerId); }
AxisStatus InputLook (int playerId) { return Pad::instance()->getAxisWorkState(WorkId::Look, playerId); }
AxisStatus InputMove (int playerId) { return Pad::instance()->getAxisWorkState(WorkId::Move, playerId); }
AxisStatus InputLookStick(int playerId) { return Pad::instance()->getAxisWorkState(WorkId::LookStick, playerId); }
bool InputMenu       (int playerId) { return Pad::instance()->isTriggered(WorkId::Menu, playerId); }
bool InputAim        (int playerId) { return Pad::instance()->isOn(WorkId::Aim, playerId); }
bool InputHolster    (int playerId) { return Pad::instance()->isTriggered(WorkId::Holster, playerId); }
bool InputDebugCollision(int playerId) { return Pad::instance()->isTriggered(WorkId::DebugCollision, playerId); }
bool InputDebugPickupCollision(int playerId) { return Pad::instance()->isTriggered(WorkId::DebugPickupCollision, playerId); }
bool InputDebugZombieHealth(int playerId) { return Pad::instance()->isTriggered(WorkId::DebugZombieHealth, playerId); }
bool InputDebugSpawnZombie(int playerId) { return Pad::instance()->isTriggered(WorkId::DebugSpawnZombie, playerId); }
bool InputDebugFreeCamera(int playerId) { return Pad::instance()->isTriggered(WorkId::DebugFreeCamera, playerId); }
bool InputDebugCameraUp(int playerId)   { return Pad::instance()->isOn(WorkId::DebugCameraUp, playerId); }
bool InputDebugCameraDown(int playerId) { return Pad::instance()->isOn(WorkId::DebugCameraDown, playerId); }
bool InputUiUp(int playerId)     { return Pad::instance()->isTriggered(WorkId::UiUp, playerId); }
bool InputUiDown(int playerId)   { return Pad::instance()->isTriggered(WorkId::UiDown, playerId); }
bool InputUiDecide(int playerId) { return Pad::instance()->isTriggered(WorkId::UiDecide, playerId); }
bool InputUiLeft(int playerId)   { return Pad::instance()->isTriggered(WorkId::UiLeft, playerId); }
bool InputUiRight(int playerId)  { return Pad::instance()->isTriggered(WorkId::UiRight, playerId); }
bool InputInteract(int playerId) { return Pad::instance()->isTriggered(WorkId::Interact, playerId); }
bool InputReload(int playerId)   { return Pad::instance()->isTriggered(WorkId::Reload, playerId); }
bool InputFlashlight(int playerId) { return Pad::instance()->isTriggered(WorkId::Flashlight, playerId); }
bool InputInventory(int playerId) { return Pad::instance()->isTriggered(WorkId::Inventory, playerId); }
bool InputQuickPlace(int playerId) { return Pad::instance()->isTriggered(WorkId::QuickPlace, playerId); }
bool InputDebugInvincible(int playerId) { return Pad::instance()->isTriggered(WorkId::DebugInvincible, playerId); }
bool InputDebugRecordPerf(int playerId) { return Pad::instance()->isTriggered(WorkId::DebugRecordPerf, playerId); }
