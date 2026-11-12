#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include "InputDef.h"

// ────────────────────────────────────────────────
// フォーマット層：ボタン入力の抽象基底クラス
//
// 「あるデバイスの、あるキーコードが今押されているか」を
// bool で返すのがこのクラスの責務
//
// KeyCodeType = デバイスごとに違うキーコードの型
// ────────────────────────────────────────────────
template<typename KeyCodeType>
class InputButtonFormatBase
{
public:
    virtual ~InputButtonFormatBase() = default;

    // ボタン名とキーコードの対応を登録する
    void AddButton(std::string_view a_buttonName, KeyCodeType a_keyCode)
    {
        m_umButtonNameToKeyCode[a_buttonName.data()] = a_keyCode;
    }

    void RemoveButton(std::string_view a_buttonName)
    {
        m_umButtonNameToKeyCode.erase(a_buttonName.data());
    }

    virtual bool CheckButtonState(KeyCodeType a_keyCode) = 0;	// 「このキーコードは今押されているか」 ← デバイスごとに実装する

    // テンプレートクラスごとに一意な型 ID を返す（InputDeviceBase が使う）
    static inputTypeID GetTypeID()
    {
        if (!m_buttonFormatTypeID) {
            m_buttonFormatTypeID = InputTypeManager::GenerateID();
        }
        return m_buttonFormatTypeID;
    }

    const std::unordered_map<std::string, KeyCodeType>& GetButtonNameToKeyCode() const
    {
        return m_umButtonNameToKeyCode;
    }

private:
    std::unordered_map<std::string, KeyCodeType> m_umButtonNameToKeyCode;
    static inputTypeID m_buttonFormatTypeID;
};

template<typename KeyCodeType>
inputTypeID InputButtonFormatBase<KeyCodeType>::m_buttonFormatTypeID = 0;


// ────────────────────────────────────────────────
// フォーマット層：軸入力の抽象基底クラス
//
// 「あるデバイスの、ある軸の AxisStatus を返す」が責務
// ────────────────────────────────────────────────
template<typename KeyCodeType>
class InputAxisFormatBase
{
public:
    virtual ~InputAxisFormatBase() = default;

    void AddAxis(std::string_view a_axisName, KeyCodeType a_keyCode)
    {
        m_umAxisNameToKeyCode[a_axisName.data()] = a_keyCode;
    }

    void RemoveAxis(std::string_view a_axisName)
    {
        m_umAxisNameToKeyCode.erase(a_axisName.data());
    }

    virtual AxisStatus CheckAxisState(KeyCodeType a_keyCode) = 0;	// 「この軸コードの現在値を AxisStatus で返す」 ← デバイスごとに実装する

    static inputTypeID GetTypeID()
    {
        if (!m_axisFormatTypeID) {
            m_axisFormatTypeID = InputTypeManager::GenerateID();
        }
        return m_axisFormatTypeID;
    }

    const std::unordered_map<std::string, KeyCodeType>& GetAxisNameToKeyCode() const
    {
        return m_umAxisNameToKeyCode;
    }

private:
    std::unordered_map<std::string, KeyCodeType> m_umAxisNameToKeyCode;
    static inputTypeID m_axisFormatTypeID;
};

template<typename KeyCodeType>
inputTypeID InputAxisFormatBase<KeyCodeType>::m_axisFormatTypeID = 0;