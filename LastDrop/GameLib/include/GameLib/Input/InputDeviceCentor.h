#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>
#include "InputDeviceBase.h"

// ────────────────────────────────────────────────
// デバイス層：全デバイスを一元管理するコンテナ
//
// Input クラスが内部に持つ。
// デバイスの追加・取得・一括更新を担う。
// ────────────────────────────────────────────────
class InputDeviceCentor
{
public:
    // DeviceType = InputDeviceBase<T> の派生クラス（例: KeyboardDevice）
    template<typename DeviceType>
    std::shared_ptr<DeviceType> AddDevice(std::string_view a_deviceName)
    {
        auto sp = std::make_shared<DeviceType>();
        m_umNameToDevice[a_deviceName.data()] = sp;
        m_umDeviceTypeToName[DeviceType::GetTypeID()] = a_deviceName.data();
        return sp;
    }

    // 型でデバイスを取得（登録済みでなければ nullptr）
    template<typename DeviceType>
    std::shared_ptr<DeviceType> GetDevice()
    {
        inputTypeID type = DeviceType::GetTypeID();
        auto itr = m_umDeviceTypeToName.find(type);
        if (itr != m_umDeviceTypeToName.end())
        {
            return std::static_pointer_cast<DeviceType>(m_umNameToDevice[itr->second]);
        }
        return nullptr;
    }

    // 名前でデバイスを取得（InputCollector が使う）
    std::shared_ptr<IInputDeviceBase> GetDevice(std::string_view a_name)
    {
        auto itr = m_umNameToDevice.find(a_name.data());
        return (itr != m_umNameToDevice.end()) ? itr->second : nullptr;
    }

    const std::unordered_map<std::string, std::shared_ptr<IInputDeviceBase>>& GetAllDevice() const
    {
        return m_umNameToDevice;
    }

    // 全デバイスを更新（毎フレーム Input::Update() から呼ばれる）
    void Update()
    {
        for (auto&& device : m_umNameToDevice)
        {
            device.second->Update();
        }
    }

private:
    std::unordered_map<std::string, std::shared_ptr<IInputDeviceBase>> m_umNameToDevice;
    std::unordered_map<inputTypeID, std::string>                        m_umDeviceTypeToName;
};