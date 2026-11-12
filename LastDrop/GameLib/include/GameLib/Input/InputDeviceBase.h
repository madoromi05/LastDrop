#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>
#include "InputDef.h"
#include "InputFormatBase.h"

// ────────────────────────────────────────────────
// デバイス層：インターフェース
//
// IInputDeviceBase は型消去のためのインターフェース
// InputDeviceCentor が shared_ptr<IInputDeviceBase> で全デバイスを
// 同じコンテナに入れるために必要
// ────────────────────────────────────────────────
class IInputDeviceBase
{
public:
    virtual ~IInputDeviceBase() = default;

    virtual void Update() = 0;	// 毎フレーム呼ぶ更新処理。フォーマット層を通して物理デバイスを読む

    // ボタン名でビットフラグ (KeyStatus) を取得
    virtual short GetButtonState(std::string_view a_buttonName) = 0;
    virtual bool IsButtonFree(std::string_view a_buttonName) = 0;
    virtual bool IsButtonPress(std::string_view a_buttonName) = 0;
    virtual bool IsButtonHold(std::string_view a_buttonName) = 0;
    virtual bool IsButtonRelease(std::string_view a_buttonName) = 0;

    virtual const std::unordered_map<std::string, short>& GetAllButtonResult() = 0;	// 全ボタン結果の参照（InputCollector が走査するために使う）

    // 軸名で AxisStatus を取得
    virtual AxisStatus GetAxisState(std::string_view a_axisName) = 0;
    virtual const std::unordered_map<std::string, AxisStatus>& GetAllAxisResult() = 0;
};


// ────────────────────────────────────────────────
// デバイス層：テンプレート実装
//
// KeyCodeType = このデバイスが使うキーコードの型
//
// 複数の Format を持てる
// 例: キーボードデバイスに「通常キーフォーマット」「ファンクションキーフォーマット」を
//     両方登録するといった拡張が可能
// ────────────────────────────────────────────────
template<typename KeyCodeType>
class InputDeviceBase : public IInputDeviceBase
{
public:
    // ─────────────────────────────────────────
    // 毎フレームの更新
    //
    // 各フォーマットの各ボタンについて
    // 「前フレームの状態」と「今フレームの物理押下フラグ」から
    // Free / Press / Hold / Release を決定してキャッシュする
    //
    // ビットフラグ遷移表:
    //   前がFree  + 今押下 → Free クリア, Press セット, Hold セット
    //   前がHold  + 今押下 → Press クリア, Hold セット（継続）
    //   前がHold  + 今解放 → Hold クリア, Release セット, Free セット
    //   前がRelease + 今解放 → Release クリア, Free セット
    // ─────────────────────────────────────────
    void Update() override final
    {
        // ボタン更新
        for (auto&& format : m_umNameToButtonFormat)
        {
            for (auto&& button : format.second->GetButtonNameToKeyCode())
            {
                short result = m_umButtonToResult[button.first];
                if (format.second->CheckButtonState(button.second))
                {
                    // 押されている
                    if (result & KeyStatus::Free)
                    {
                        result |= KeyStatus::Press;	// 前フレームが Free → 今フレームは Press
                    }
                    else
                    {
                        result &= ~KeyStatus::Press;	// 前フレームも押していた → Press を落として Hold を維持
                    }
                    result &= ~KeyStatus::Free;
                    result |= KeyStatus::Hold;
                }
                else
                {
                    // 押されていない
                    if (result & KeyStatus::Hold)
                    {
                        result |= KeyStatus::Release;	// 前フレームが Hold → 今フレームは Release
                    }
                    else
                    {
                        result &= ~KeyStatus::Release;	// 前フレームも離していた → Release を落として Free を維持
                    }
                    result &= ~KeyStatus::Hold;
                    result |= KeyStatus::Free;
                }
                m_umButtonToResult[button.first] = result;
            }
        }

        // 軸更新（毎フレームそのまま上書き）
        for (auto&& format : m_umNameToAxisFormat)
        {
            for (auto&& axis : format.second->GetAxisNameToKeyCode())
            {
                m_umAxisToResult[axis.first] = format.second->CheckAxisState(axis.second);
            }
        }
    }

    short GetButtonState(std::string_view a_buttonName) override final
    {
        auto itr = m_umButtonToResult.find(a_buttonName.data());
        return (itr != m_umButtonToResult.end()) ? itr->second : 0;
    }

    bool IsButtonFree(std::string_view a_buttonName) override final
    {
        return GetButtonState(a_buttonName) & KeyStatus::Free;
    }

    bool IsButtonPress(std::string_view a_buttonName) override final
    {
        return GetButtonState(a_buttonName) & KeyStatus::Press;
    }

    bool IsButtonHold(std::string_view a_buttonName) override final
    {
        return GetButtonState(a_buttonName) & KeyStatus::Hold;
    }

    bool IsButtonRelease(std::string_view a_buttonName) override final
    {
        return GetButtonState(a_buttonName) & KeyStatus::Release;
    }

    const std::unordered_map<std::string, short>& GetAllButtonResult() override final
    {
        return m_umButtonToResult;
    }

    AxisStatus GetAxisState(std::string_view a_axisName) override final
    {
        auto itr = m_umAxisToResult.find(a_axisName.data());
        return (itr != m_umAxisToResult.end()) ? itr->second : AxisStatus();
    }

    const std::unordered_map<std::string, AxisStatus>& GetAllAxisResult() override final
    {
        return m_umAxisToResult;
    }

    // ─────────────────────────────────────────
    // フォーマットの登録
    // FormatType = InputButtonFormatBase<KeyCodeType> の派生クラス
    // ─────────────────────────────────────────
    template<typename FormatType>
    std::shared_ptr<FormatType> AddInputButtonFormat(std::string_view a_formatName)
    {
        auto sp = std::make_shared<FormatType>();
        inputTypeID type = InputButtonFormatBase<KeyCodeType>::GetTypeID();
        m_umFormatTypeToName[type] = a_formatName.data();
        m_umNameToFormatType[a_formatName.data()] = type;
        m_umNameToButtonFormat[a_formatName.data()] = sp;
        return sp;
    }

    template<typename FormatType>
    std::shared_ptr<FormatType> AddInputAxisFormat(std::string_view a_formatName)
    {
        auto sp = std::make_shared<FormatType>();
        inputTypeID type = InputAxisFormatBase<KeyCodeType>::GetTypeID();
        m_umFormatTypeToName[type] = a_formatName.data();
        m_umNameToFormatType[a_formatName.data()] = type;
        m_umNameToAxisFormat[a_formatName.data()] = sp;
        return sp;
    }

    // デバイスクラスごとの一意 ID
    static inputTypeID GetTypeID()
    {
        if (!m_deviceTypeID) {
            m_deviceTypeID = InputTypeManager::GenerateID();
        }
        return m_deviceTypeID;
    }

protected:
    std::unordered_map<std::string, std::shared_ptr<InputButtonFormatBase<KeyCodeType>>> m_umNameToButtonFormat;
    std::unordered_map<std::string, std::shared_ptr<InputAxisFormatBase<KeyCodeType>>>   m_umNameToAxisFormat;
    std::unordered_map<inputTypeID, std::string> m_umFormatTypeToName;
    std::unordered_map<std::string, inputTypeID> m_umNameToFormatType;
    std::unordered_map<std::string, short>       m_umButtonToResult;
    std::unordered_map<std::string, AxisStatus>  m_umAxisToResult;

    static inputTypeID m_deviceTypeID;
};

template<typename KeyCodeType>
inputTypeID InputDeviceBase<KeyCodeType>::m_deviceTypeID = 0;