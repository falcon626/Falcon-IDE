#pragma once

/// <summary> =Flyweight= </summary>
template<typename T>
class BaseBasicResourceManager
{
public:
    virtual ~BaseBasicResourceManager() = default;

    virtual const bool Load(const std::string& path)PURE;

   const std::shared_ptr<T> Get(const std::string& path, const std::string& guid) noexcept
   {
       // マップからパスを検索
       auto it{ m_resources.find(guid) };

       // もし見つかったら（ロード済みなら）、既存のポインタを返す
       if (it != m_resources.end()) return it->second;

       // 見つからなければ（未ロードなら）、新規にロードする
       else if (!Load(path)) return nullptr;

       // ロードできたのなら
       else return m_resources[guid];
   }

   void Clear() { m_resources.clear(); }
protected:
    // ファイルパスをキーに、リソースの共有ポインタを管理するマップ
    std::unordered_map<std::string, std::shared_ptr<T>> m_resources;
};