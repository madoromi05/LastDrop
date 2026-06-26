#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <string_view>
#include "InputDef.h"
#include "InputDeviceCentor.h"
#include "InputCollector.h"

// ────────────────────────────────────────────────
// 最上位クラス：Input
//
// 外部から触るのはこのクラスだけでよい。
//
// ─ 初期化の流れ ─
//   Input input;
//   auto kb = input.AddDevice<KeyboardDevice>("Keyboard");
//   auto fmt = kb->AddInputButtonFormat<KeyboardButtonFormat>("Keys");
//   fmt->AddButton("W", 'w');
//   fmt->AddButton("Space", ' ');
//
//   auto col = input.AddCollector("InGame");
//   col->AddButtonToButtonWork("MoveUp", "Keyboard", "W");
//   col->AddButtonToButtonWork("Jump",   "Keyboard", "Space");
// ────────────────────────────────────────────────
class Input
{
public:
    // ─────────────────────────────────────────
    // デバイス管理
    // ─────────────────────────────────────────

    template<typename DeviceType>
    std::shared_ptr<DeviceType> AddDevice(std::string_view a_deviceName)
    {
        return m_deviceCentor.AddDevice<DeviceType>(a_deviceName);
    }

    template<typename DeviceType>
    std::shared_ptr<DeviceType> GetDevice()
    {
        return m_deviceCentor.GetDevice<DeviceType>();
    }

    std::shared_ptr<IInputDeviceBase> GetDevice(std::string_view a_deviceName)
    {
        return m_deviceCentor.GetDevice(a_deviceName);
    }

    const std::unordered_map<std::string, std::shared_ptr<IInputDeviceBase>>& GetAllDevice() const
    {
        return m_deviceCentor.GetAllDevice();
    }

    // ─────────────────────────────────────────
    // コレクター管理（シーン別割り当て）
    // ─────────────────────────────────────────

    std::shared_ptr<InputCollector> AddCollector(std::string_view a_collectorName)
    {
        auto sp = std::make_shared<InputCollector>();
        sp->SetName(a_collectorName);
        sp->SetActive(true);
        m_umNameToCollector[a_collectorName.data()] = sp;
        return sp;
    }

    std::shared_ptr<InputCollector> GetCollector(std::string_view a_collectorName)
    {
        auto itr = m_umNameToCollector.find(a_collectorName.data());
        return (itr != m_umNameToCollector.end()) ? itr->second : nullptr;
    }

    std::unordered_map<std::string, std::shared_ptr<InputCollector>>& GetAllCollector()
    {
        return m_umNameToCollector;
    }

    // ─────────────────────────────────────────
    // ワーク結果の取得
    //
    // Update() 後に有効。アクティブなコレクターの
    // 全ワークを OR 合成した結果を返す。
    // ─────────────────────────────────────────

    short GetButtonWorkState(std::string_view a_work) const
    {
        auto itr = m_umWorkToButtonResult.find(a_work.data());
        return (itr != m_umWorkToButtonResult.end()) ? itr->second : 0;
    }

    bool IsButtonWorkFree(std::string_view a_work) const { return GetButtonWorkState(a_work) & KeyStatus::Free; }
    bool IsButtonWorkPress(std::string_view a_work) const { return GetButtonWorkState(a_work) & KeyStatus::Press; }
    bool IsButtonWorkHold(std::string_view a_work) const { return GetButtonWorkState(a_work) & KeyStatus::Hold; }
    bool IsButtonWorkRelease(std::string_view a_work) const { return GetButtonWorkState(a_work) & KeyStatus::Release; }

    AxisStatus GetAxisWorkStatus(std::string_view a_work) const
    {
        auto itr = m_umWorkToAxisResult.find(a_work.data());
        return (itr != m_umWorkToAxisResult.end()) ? itr->second : AxisStatus();
    }

    // ─────────────────────────────────────────
    // 毎フレーム更新
    //
    // 1. 全デバイスを更新（物理入力を読む）
    // 2. アクティブなコレクターを走査し
    //    ワーク → ボタン/軸 の結果を OR 合成してキャッシュ
    // ─────────────────────────────────────────
    void Update()
    {
        m_deviceCentor.Update();

        // 前フレームの結果をクリア
        m_umWorkToButtonResult.clear();
        m_umWorkToAxisResult.clear();

        for (auto&& collector : m_umNameToCollector)
        {
            if (!collector.second->CheckActive()) continue;

            // ボタンワークの合成
            for (auto&& work : collector.second->GetAllButtonWork())
            {
                for (auto&& deviceToButtons : work.second)
                {
                    auto device = m_deviceCentor.GetDevice(deviceToButtons.first);
                    if (!device) continue;

                    for (auto&& button : deviceToButtons.second)
                    {
                        // OR で合成：どれか1つでも Press/Hold なら反映される
                        m_umWorkToButtonResult[work.first] |= device->GetButtonState(button);
                    }
                }
            }

            // 軸ワークの合成（複数デバイスを加算）
            for (auto&& work : collector.second->GetAllAxisWork())
            {
                for (auto&& deviceToAxises : work.second)
                {
                    auto device = m_deviceCentor.GetDevice(deviceToAxises.first);
                    if (!device) continue;

                    for (auto&& axis : deviceToAxises.second)
                    {
                        m_umWorkToAxisResult[work.first] += device->GetAxisState(axis);
                    }
                }
            }
        }
    }

private:
    InputDeviceCentor m_deviceCentor;
    std::unordered_map<std::string, std::shared_ptr<InputCollector>> m_umNameToCollector;
    std::unordered_map<std::string, short>      m_umWorkToButtonResult;
    std::unordered_map<std::string, AxisStatus> m_umWorkToAxisResult;
};