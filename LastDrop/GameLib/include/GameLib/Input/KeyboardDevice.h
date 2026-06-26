#pragma once
#include "InputDeviceBase.h"
#include "Manager.h"
#include "Keyboard.h"
using namespace GameLib::Input;

// ────────────────────────────────────────────────
// キーボード用フォーマット
//
// KeyCodeType = int（GameLib の isOn/isTriggered の引数）
// ────────────────────────────────────────────────
class KeyboardButtonFormat : public InputButtonFormatBase<int>
{
public:
    // GameLib のキーボードに問い合わせて今押されているか返す
    bool CheckButtonState(int a_keyCode) override
    {
        return Manager::instance().keyboard().isOn(a_keyCode);
    }
};

// ────────────────────────────────────────────────
// キーボードデバイス
// ────────────────────────────────────────────────
class KeyboardDevice : public InputDeviceBase<int>
{
    // 特に追加実装なし。AddInputButtonFormat<KeyboardButtonFormat>() で使う。
};