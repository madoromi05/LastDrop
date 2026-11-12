#pragma once
#include <vector>
#include "InputEnums.h"

// ────────────────────────────────────────────────
// InputConfig：バインド表（BindingRecord の集合）の「唯一の正」
//
// ・デフォルト割当の保持（loadDefaults）
// ・実行時の付け替え（add / remove / rebind）… キーコンフィグの土台
// ・ファイルへの保存／読込（save / load）
//
// 実行時の入力状態そのもの（Free/Press/…）は持たない
// 「どのキーが何に割り当たっているか」という設定だけを扱う
// 設定を実際の Input に反映するのは InputBuilder::Bind の役目
//
// 保存形式は PseudoXml（人が読める）。ディスク I/O は std::fstream で行い、
// GameLib::FileIO を通さない（設定ファイル欠如で Halt しないようにするため）
// ────────────────────────────────────────────────
namespace GameLib {
namespace Input {

class InputConfig
{
public:
    // 全消去
    void clear();

    // デフォルト表で置き換える（clear してからコピー）
    void loadDefaults(const BindingRecord* a_records, int a_count);

    // 実行時の付け替え（キーコンフィグ用）
    void addBinding(const BindingRecord& a_record);     // 同一レコードは重複追加しない
    void removeBinding(const BindingRecord& a_record);
    void rebind(CollectorId a_collector, WorkId a_work, DeviceKind a_device,
                int a_oldCode, int a_newCode);

    // 既定の表にある操作のうち、今の表に 1 つも割り当てが無い操作へ、既定の行を足す
    // 保存した後に操作（WorkId）を足しても、古いファイルを読んだまま新しい操作が効くようにする（load の後に呼ぶ）
    void addMissingWorks(const BindingRecord* a_defaults, int a_count);

    // 永続化。load はファイルが無ければ false（呼び側が loadDefaults にフォールバックする）
    bool save(const char* a_path) const;
    bool load(const char* a_path);

    const std::vector<BindingRecord>& records() const { return m_records; }

private:
    std::vector<BindingRecord> m_records;
};

}
}
