#pragma once
#include "InputDeviceBase.h"
#include "Manager.h"
#include "Joystick.h"
using namespace GameLib::Input;

// ────────────────────────────────────────────────
// ジョイスティックボタン用フォーマット
//
// コンストラクタでプレイヤー番号（ジョイスティック番号）を受け取る
// ────────────────────────────────────────────────
class JoystickButtonFormat : public InputButtonFormatBase<int>
{
public:
    explicit JoystickButtonFormat(int a_playerId = 0)
        : m_playerId(a_playerId) {}

    bool CheckButtonState(int a_keyCode) override
    {
        Manager m = Manager::instance();
        if (m.joystickNumber() <= m_playerId) return false;
        return m.joystick(m_playerId).isOn(a_keyCode);
    }

private:
    int m_playerId;
};

// ────────────────────────────────────────────────
// ジョイスティック軸用フォーマット
//
// a_keyCode = アナログスティックの「スティック番号」
//   0 → analog(0) を width, analog(1) を height（左スティック）
//   1 → analog(2) を width, analog(3) を height（右スティック）
// ────────────────────────────────────────────────
class JoystickAxisFormat : public InputAxisFormatBase<int>
{
public:
    explicit JoystickAxisFormat(int a_playerId = 0)
        : m_playerId(a_playerId) {}

    AxisStatus CheckAxisState(int a_keyCode) override
    {
        Manager m = Manager::instance();
        if (m.joystickNumber() <= m_playerId) return AxisStatus();
        Joystick j = m.joystick(m_playerId);

        int xIdx = a_keyCode * 2;      // スティック 0 → analog(0,1)
        int yIdx = a_keyCode * 2 + 1;
        float x = (j.analogNumber() > xIdx) ? static_cast<float>(j.analog(xIdx)) : 0.f;
        float y = (j.analogNumber() > yIdx) ? static_cast<float>(j.analog(yIdx)) : 0.f;
        return AxisStatus(x, y);
    }

private:
    int m_playerId;
};

// ────────────────────────────────────────────────
// ジョイスティックデバイス
// ────────────────────────────────────────────────
class JoystickDevice : public InputDeviceBase<int>
{
    // 特に追加実装なし
};
