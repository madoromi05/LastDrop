#pragma once
#include <cstdint>

// ────────────────────────────────────────────────
// フォーマット層：ボタンの4状態（ビットフラグ）
//   Free    : 押されていない
//   Press   : 押した瞬間（このフレームだけ）
//   Hold    : 押し続けている
//   Release : 離した瞬間（このフレームだけ）
// ────────────────────────────────────────────────
enum KeyStatus
{
    Free = 1 << 0,      // 0001
    Press = 1 << 1,     // 0010
    Hold = 1 << 2,      // 0100
    Release = 1 << 3,  // 1000
};

// ────────────────────────────────────────────────
// フォーマット層：軸入力（アナログスティック・マウス移動など）
// ────────────────────────────────────────────────
struct AxisStatus
{
    AxisStatus() : m_width(0.f), m_height(0.f) {}
    AxisStatus(float a_width, float a_height)
        : m_width(a_width), m_height(a_height) {}

    float m_width;   // 横方向（右が正）
    float m_height;  // 縦方向（下が正）

    AxisStatus operator+(const AxisStatus& a_value) const {
        return AxisStatus(m_width + a_value.m_width, m_height + a_value.m_height);
    }
    AxisStatus operator-(const AxisStatus& a_value) const {
        return AxisStatus(m_width - a_value.m_width, m_height - a_value.m_height);
    }
    AxisStatus& operator+=(const AxisStatus& a_value) {
        m_width += a_value.m_width;
        m_height += a_value.m_height;
        return *this;
    }
};

// ────────────────────────────────────────────────
// クラスごとに一意な整数 ID を振るユーティリティ。
// テンプレートクラスの型判別に使う。
// ────────────────────────────────────────────────
using inputTypeID = uint32_t;

class InputTypeManager
{
public:
    // 呼ぶたびに 1 ずつ増える ID を返す。スレッド安全でなくてよい（シングルスレッド前提）。
    static inputTypeID GenerateID()
    {
        static inputTypeID s_counter = 0;
        return ++s_counter;
    }
};