#pragma once
#include <memory>
#include "GameLib/Input/Input.h"
#include "GameLib/Input/KeyboardDevice.h"
#include "GameLib/Input/MouseDevice.h"
#include "GameLib/Input/JoystickDevice.h"

// ────────────────────────────────────────────────
// Pad：記事の Input システムをこのゲーム用に組み立てるクラス
//
// Robo.cpp などのゲームコードは従来通り
//   Pad::instance()->isOn( Pad::JUMP, 0 )
// で呼べる。内部では Input クラスを使っている。
// ────────────────────────────────────────────────
class Pad
{
public:
    // ワーク名（文字列で管理。変えたければここを書き換えるだけ）
    static constexpr const char* WORK_UP = "Up";
    static constexpr const char* WORK_DOWN = "Down";
    static constexpr const char* WORK_LEFT = "Left";
    static constexpr const char* WORK_RIGHT = "Right";
    static constexpr const char* WORK_JUMP = "Jump";
    static constexpr const char* WORK_FIRE = "Fire";

    // 従来互換の列挙型（Robo.cpp などを無変更にするため残す）
    enum Button { UP, DOWN, LEFT, RIGHT, JUMP, FIRE };

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
    short getWorkState(const char* workName) const;
    bool  isWorkFree(const char* workName) const;
    bool  isWorkPress(const char* workName) const;
    bool  isWorkHold(const char* workName) const;
    bool  isWorkRelease(const char* workName) const;

    // ─────────────────────────────────────────
    // キーコンフィグ：ワークへのボタン追加・削除
    // 例: addButtonToWork("Jump", "Keyboard", "Space", 0)
    // ─────────────────────────────────────────
    void addButtonToWork(const char* work, const char* device, const char* button, int playerId);
    void removeButtonFromWork(const char* work, const char* device, const char* button, int playerId);

    // コレクター（シーン別マップ）の有効/無効切替
    void setCollectorActive(const char* collectorName, bool active);

    // Input インスタンスへの直接アクセス（上級者向け）
    ::Input& getInput(int playerId);

private:
    static const int MAX_PLAYERS = 2;

    Pad();
    ~Pad();

    void setupDevicesAndBindings();

    // プレイヤーごとに独立した Input インスタンスを持つ
    ::Input m_inputs[MAX_PLAYERS];

    // ワーク名 → Button 列挙（従来 API の変換用）
    static const char* buttonToWorkName(Button b);

    static Pad* mInstance;
};