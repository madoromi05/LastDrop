#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

// ────────────────────────────────────────────────
// ワーク層：シーン別の入力割り当てを管理するクラス
//
// 「ワーク名 → {デバイス名 → {ボタン名の集合}}」という
// 3重マップで割り当てを保持する。
//
// 使い方：
//   auto col = input.AddCollector("InGame");
//   col->AddButtonToButtonWork("Attack", "Keyboard", "Enter");
//   col->AddButtonToButtonWork("Attack", "Mouse",    "Left");
//   // → "Attack" は Keyboard の Enter か Mouse の Left どちらでも発動
//
// SetActive(false) でシーン切替時に無効化できる。
// ────────────────────────────────────────────────
class InputCollector
{
public:
    bool CheckActive() const { return m_bActive; }
    void SetActive(bool a_bActive) { m_bActive = a_bActive; }

    std::string_view GetName() const { return m_name; }
    void SetName(std::string_view a_name) { m_name = a_name; }

    // ── ボタンワーク ──────────────────────────────

    // ワークを（空で）登録する
    void AddButtonWork(std::string_view a_work)
    {
        m_umWorkToButtons[a_work.data()];
    }

    // ワークにボタンを追加する
    // a_work   : ワーク名（例: "Jump"）
    // a_device : デバイス名（例: "Keyboard"）
    // a_button : ボタン名（例: "Space"）
    void AddButtonToButtonWork(std::string_view a_work,
        std::string_view a_device,
        std::string_view a_button)
    {
        m_umWorkToButtons[a_work.data()][a_device.data()].emplace(a_button.data());
    }

    void RemoveButtonWork(std::string_view a_work)
    {
        m_umWorkToButtons.erase(a_work.data());
    }

    void RemoveButtonFromButtonWork(std::string_view a_work,
        std::string_view a_device,
        std::string_view a_button)
    {
        m_umWorkToButtons[a_work.data()][a_device.data()].erase(a_button.data());
    }

    const std::unordered_map<std::string,
        std::unordered_map<std::string,
        std::unordered_set<std::string>>>& GetAllButtonWork() const
    {
        return m_umWorkToButtons;
    }

    // ── 軸ワーク ──────────────────────────────────

    void AddAxisWork(std::string_view a_work)
    {
        m_umWorkToAxises[a_work.data()];
    }

    void AddAxisToAxisWork(std::string_view a_work,
        std::string_view a_device,
        std::string_view a_axis)
    {
        m_umWorkToAxises[a_work.data()][a_device.data()].emplace(a_axis.data());
    }

    void RemoveAxisWork(std::string_view a_work)
    {
        m_umWorkToAxises.erase(a_work.data());
    }

    void RemoveAxisFromAxisWork(std::string_view a_work,
        std::string_view a_device,
        std::string_view a_axis)
    {
        m_umWorkToAxises[a_work.data()][a_device.data()].erase(a_axis.data());
    }

    const std::unordered_map<std::string,
        std::unordered_map<std::string,
        std::unordered_set<std::string>>>& GetAllAxisWork() const
    {
        return m_umWorkToAxises;
    }

private:
    bool        m_bActive = false;
    std::string m_name;

    // ワーク名 → デバイス名 → ボタン名の集合
    std::unordered_map<std::string,
        std::unordered_map<std::string,
        std::unordered_set<std::string>>> m_umWorkToButtons;

    // ワーク名 → デバイス名 → 軸名の集合
    std::unordered_map<std::string,
        std::unordered_map<std::string,
        std::unordered_set<std::string>>> m_umWorkToAxises;
};