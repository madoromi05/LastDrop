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
    const char* kAttrVersion = "version";
    const char* kRootName = "InputConfig";
    const char* kBindingName = "Binding";
    const int kVersion = 1;
    const int kBindingAttributeCount = 4;

    // 読んだ番号が、今ある列挙の範囲に入っているか（古い・壊れたファイルの行を飛ばすため）。
    bool IsKnownRecord(int a_collector, int a_work, int a_device)
    {
        return a_collector == static_cast<int>(CollectorId::InGame)
            && a_work >= 0 && a_work < kWorkIdCount
            && a_device >= 0 && a_device <= static_cast<int>(DeviceKind::MouseAxis);
    }

    // 1 行（<Binding .../>）を読む。4 つの属性がそろい、番号が範囲に入っていれば true。
    bool ReadBinding(PseudoXml::ConstElement a_element, BindingRecord* a_out)
    {
        if (!a_element.name() || std::strcmp(a_element.name(), kBindingName) != 0) return false;
        int collector = -1;
        int work = -1;
        int device = -1;
        int code = 0;
        int found = 0;
        for (int j = 0; j < a_element.attributeNumber(); ++j)
        {
            PseudoXml::ConstAttribute a = a_element.attribute(j);
            const char* name = a.name();
            if (std::strcmp(name, kAttrCollector) == 0)   { collector = a.getIntValue(); ++found; }
            else if (std::strcmp(name, kAttrWork) == 0)   { work = a.getIntValue(); ++found; }
            else if (std::strcmp(name, kAttrDevice) == 0) { device = a.getIntValue(); ++found; }
            else if (std::strcmp(name, kAttrCode) == 0)   { code = a.getIntValue(); ++found; }
        }
        if (found != kBindingAttributeCount || !IsKnownRecord(collector, work, device)) return false;
        *a_out = BindingRecord{ static_cast<CollectorId>(collector), static_cast<WorkId>(work), static_cast<DeviceKind>(device), code };
        return true;
    }
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

void InputConfig::addMissingWorks(const BindingRecord* a_defaults, int a_count)
{
    // 先に「ファイルに割り当てがある操作」を決めてから足す（足した既定の行で、同じ操作の 2 行目を飛ばさないように）。
    std::vector<BindingRecord> missing;
    for (int i = 0; i < a_count; ++i)
    {
        const BindingRecord& def = a_defaults[i];
        bool assigned = false;
        for (const auto& r : m_records)
        {
            if (r.collector == def.collector && r.work == def.work)
            {
                assigned = true;
                break;
            }
        }
        if (!assigned) missing.push_back(def);
    }
    for (const auto& r : missing)
    {
        addBinding(r);
    }
}

// ────────────────────────────────────────────────
// 保存：PseudoXml でテキスト化し、std::ofstream で書き出す
//
//   <InputConfig version="1">
//     <Binding collector="0" work="0" device="0" code="119"/>
//     ...
//   </InputConfig>
//
// PseudoXml のテキストの一番外（doc.root）は名前を書かない入れ物なので、その子に InputConfig の要素を作る
// （GameSettings の settings.xml と同じ形）。一番外に名前を付けても書き出されず、読むときにも名前は無い。
// ────────────────────────────────────────────────
bool InputConfig::save(const char* a_path) const
{
    PseudoXml::Document doc = PseudoXml::Document::create();
    PseudoXml::Element root = doc.root();
    root.setChildNumber(1);
    PseudoXml::Element config = root.child(0);
    config.setName(kRootName);
    config.setAttributeNumber(1);
    config.attribute(0).set(kAttrVersion, kVersion);
    config.setChildNumber(static_cast<int>(m_records.size()));

    for (int i = 0; i < static_cast<int>(m_records.size()); ++i)
    {
        const BindingRecord& rec = m_records[i];
        PseudoXml::Element e = config.child(i);
        e.setName(kBindingName);
        e.setAttributeNumber(kBindingAttributeCount);
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
// ファイルが無い／壊れている／InputConfig の要素が無い場合は false を返す（呼び側で loadDefaults にフォールバック）
// 一番外の入れ物の子から InputConfig の要素を探す（save と同じ形）。読めない行（属性が欠けている・知らない番号）は飛ばす。
// version は今 1 だけなので、読むが比べない（形を変えたときに、ここで読み分ける）。
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
    for (int r = 0; r < root.childNumber(); ++r)
    {
        PseudoXml::ConstElement config = root.child(r);
        if (!config.name() || std::strcmp(config.name(), kRootName) != 0) continue;

        std::vector<BindingRecord> loaded;
        for (int i = 0; i < config.childNumber(); ++i)
        {
            BindingRecord rec{};
            if (ReadBinding(config.child(i), &rec)) loaded.push_back(rec);
        }
        m_records.swap(loaded);
        return true;
    }
    return false;
}
