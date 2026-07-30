#include "GameLib/Input/InputConfig.h"
#include "GameLib/PseudoXml/Document.h"
#include "GameLib/PseudoXml/Element.h"
#include "GameLib/PseudoXml/Attribute.h"
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>

using namespace GameLib;
using namespace GameLib::Input;

// ────────────────────────────────────────────────
// レコードの同一判定（collector/work/device/code の完全一致）
// コード変更などを反映させるためにある
// ローカルに保存させている
// ────────────────────────────────────────────────
namespace
{
    bool SameBinding(const BindingRecord& a, const BindingRecord& b)
    {
        return a.collector == b.collector
            && a.work == b.work
            && a.device == b.device
            && a.code == b.code;
    }

    // 属性名の想定文字列（保存/読込で共有）
    const char* kAttrCollector = "collector";
    const char* kAttrWork = "work";
    const char* kAttrDevice = "device";
    const char* kAttrCode = "code";
    const char* kRootName = "InputConfig";
    const char* kBindingName = "Binding";
}

void InputConfig::clear()
{
    m_records.clear();
}

void InputConfig::loadDefaults(const BindingRecord* a_records, int a_count)
{
    m_records.assign(a_records, a_records + a_count);
}

void InputConfig::addBinding(const BindingRecord& a_record)
{
    for (const auto& r : m_records)
    {
        if (SameBinding(r, a_record)) return; // 重複は追加しない
    }
    m_records.push_back(a_record);
}

void InputConfig::removeBinding(const BindingRecord& a_record)
{
    for (auto it = m_records.begin(); it != m_records.end(); ++it)
    {
        if (SameBinding(*it, a_record))
        {
            m_records.erase(it);
            return;
        }
    }
}

void InputConfig::rebind(CollectorId a_collector, WorkId a_work, DeviceKind a_device,
                         int a_oldCode, int a_newCode)
{
    BindingRecord oldRec{ a_collector, a_work, a_device, a_oldCode };
    BindingRecord newRec{ a_collector, a_work, a_device, a_newCode };
    removeBinding(oldRec);
    addBinding(newRec);
}

// ────────────────────────────────────────────────
// 保存：PseudoXml でテキスト化し、std::ofstream で書き出す
//
//   <InputConfig>
//     <Binding collector="0" work="0" device="0" code="119"/>
//     ...
//   </InputConfig>
// ────────────────────────────────────────────────
bool InputConfig::save(const char* a_path) const
{
    PseudoXml::Document doc = PseudoXml::Document::create();
    PseudoXml::Element root = doc.root();
    root.setName(kRootName);
    root.setChildNumber(static_cast<int>(m_records.size()));

    for (int i = 0; i < static_cast<int>(m_records.size()); ++i)
    {
        const BindingRecord& rec = m_records[i];
        PseudoXml::Element e = root.child(i);
        e.setName(kBindingName);
        e.setAttributeNumber(4);
        e.attribute(0).set(kAttrCollector, static_cast<int>(rec.collector));
        e.attribute(1).set(kAttrWork, static_cast<int>(rec.work));
        e.attribute(2).set(kAttrDevice, static_cast<int>(rec.device));
        e.attribute(3).set(kAttrCode, rec.code);
    }

    std::string text;
    doc.convertToString(&text);

    std::ofstream ofs(a_path, std::ios::binary);
    if (!ofs) return false;
    ofs.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(ofs);
}

// ────────────────────────────────────────────────
// 読込：ファイルを std::ifstream で読み、PseudoXml で解析する
// ファイルが無い／壊れている場合は false を返す（呼び側で loadDefaults にフォールバック）
// ────────────────────────────────────────────────
bool InputConfig::load(const char* a_path)
{
    std::ifstream ifs(a_path, std::ios::binary);
    if (!ifs) return false;

    std::ostringstream buffer;
    buffer << ifs.rdbuf();
    std::string text = buffer.str();
    if (text.empty()) return false;

    PseudoXml::Document doc = PseudoXml::Document::create(text.data(), static_cast<int>(text.size()));
    if (!doc || doc.isError()) return false;

    PseudoXml::ConstElement root = doc.root();
    if (!root || std::strcmp(root.name(), kRootName) != 0) return false;

    std::vector<BindingRecord> loaded;
    const int childCount = root.childNumber();
    for (int i = 0; i < childCount; ++i)
    {
        PseudoXml::ConstElement e = root.child(i);
        BindingRecord rec{ CollectorId::InGame, WorkId::Up, DeviceKind::Keyboard, 0 };

        const int attrCount = e.attributeNumber();
        for (int j = 0; j < attrCount; ++j)
        {
            PseudoXml::ConstAttribute a = e.attribute(j);
            const char* name = a.name();
            if (std::strcmp(name, kAttrCollector) == 0)   rec.collector = static_cast<CollectorId>(a.getIntValue());
            else if (std::strcmp(name, kAttrWork) == 0)   rec.work = static_cast<WorkId>(a.getIntValue());
            else if (std::strcmp(name, kAttrDevice) == 0) rec.device = static_cast<DeviceKind>(a.getIntValue());
            else if (std::strcmp(name, kAttrCode) == 0)   rec.code = a.getIntValue();
        }
        loaded.push_back(rec);
    }

    m_records.swap(loaded);
    return true;
}
