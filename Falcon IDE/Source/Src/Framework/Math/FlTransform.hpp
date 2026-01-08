#pragma once

class FlTransform : public std::enable_shared_from_this<FlTransform>
{
public:
    FlTransform() = default;
    ~FlTransform() { OnDestroy(); }

    // ----------- Getter -----------
    const Math::Vector3& GetLocalPosition() const noexcept { return m_localPosition; }
    const Math::Quaternion& GetLocalRotation() const noexcept { return m_localRotation; }
    const Math::Vector3& GetLocalScale() const noexcept { return m_localScale; }
    Math::Vector3 GetWorldPosition() const noexcept { return GetWorldMatrix().Translation(); }
    Math::Quaternion GetWorldRotation() const noexcept { return Math::Quaternion::CreateFromRotationMatrix(GetWorldMatrix()); }
    const Math::Matrix& GetWorldMatrix() const noexcept { return CreateWorldMatrix(); }
    Math::Matrix& WorkWorldMatrix() const noexcept { return CreateWorldMatrix(); }

    // ----------- Setter -----------
    void SetLocalPosition(const Math::Vector3& pos) { m_localPosition = pos; MarkDirty(); }
    void SetLocalRotation(const Math::Quaternion& rot) { m_localRotation = rot; MarkDirty(); }
    void SetLocalScale(const Math::Vector3& scale) { m_localScale = scale; MarkDirty(); }
    void SetWorldPosition(const Math::Vector3& pos)
    {
        auto world{ Math::Matrix::CreateTranslation(pos) };
        if (const auto parent{ m_parent.lock() }) world *= parent->GetWorldMatrix().Invert();
        m_localPosition = world.Translation();
        MarkDirty();
    }

    void SetWorldRotation(const Math::Quaternion& rot)
    {
        auto worldRot{ Math::Matrix::CreateFromQuaternion(rot) };
        if (const auto parent{ m_parent.lock() }) worldRot *= parent->GetWorldMatrix().Invert();
        m_localRotation = Math::Quaternion::CreateFromRotationMatrix(worldRot);
        MarkDirty();
    }

    void SetLocalMatrix(Math::Matrix& mat)
    {
        auto s{ Def::Vec3 }, t{ Def::Vec3 };
        auto r{ Math::Quaternion{} };
        mat.Decompose(s, r, t);
        m_localScale = s;
        m_localRotation = r;
        m_localPosition = t;
        MarkDirty();
    }

    void SetWorldMatrix(Math::Matrix& mat)
    {
        if (const auto parent{ m_parent.lock() })
        {
            Math::Matrix local = mat * parent->GetWorldMatrix().Invert();
            SetLocalMatrix(local);
        }
        else SetLocalMatrix(mat);
    }

    // ----------- Hierarchy -----------

    void AddChild(const std::shared_ptr<FlTransform>& child)
    {
        if (!child) return;
        if (child.get() == this) return;

        // 既に存在するなら追加しない
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end()) return;

        child->m_parent = shared_from_this();
        m_children.push_back(child);
        child->MarkDirty();
    }

    bool RemoveChild(const std::shared_ptr<FlTransform>& child)
    {
        auto it = std::remove(m_children.begin(), m_children.end(), child);
        if (it != m_children.end())
        {
            m_children.erase(it, m_children.end());
            if (child)
            {
                child->m_parent.reset();
                child->MarkDirty();
            }
            return true;
        }
        return false;
    }

    void SetParent(const std::shared_ptr<FlTransform>& newParent)
    {
        if (newParent && newParent.get() == this) return;
        // 既存の親から削除
        if (auto old = m_parent.lock())
        {
            old->RemoveChild(shared_from_this());
        }

        // 新しい親へ追加
        if (newParent)
        {
            newParent->AddChild(shared_from_this());
        }
        else
        {
            m_parent.reset();
        }

        MarkDirty();
    }

    const std::vector<std::shared_ptr<FlTransform>>& GetChildren() const { return m_children; }
    const std::weak_ptr<FlTransform>& GetParent() const { return m_parent; }

private:

    void OnDestroy()
    {
        if (auto parent = m_parent.lock())
        {
            parent->RemoveChild(shared_from_this());
        }

        for (auto& child : m_children)
        {
            if (child)
            {
                child->m_parent.reset();
                child->MarkDirty();
            }
        }

        m_children.clear();
    }

    void MarkDirty()
    {
        if (m_isDirty) return;
        m_isDirty = true;
        for (auto& child : m_children)
        {
            if (child) child->MarkDirty();
        }
    }

    Math::Matrix& CreateWorldMatrix() const noexcept
    {
        if (m_isDirty)
        {
            auto local{ Math::Matrix::CreateScale(m_localScale) *
                Math::Matrix::CreateFromQuaternion(m_localRotation) *
                Math::Matrix::CreateTranslation(m_localPosition) };

            if (const auto parent{ m_parent.lock() })
            {
                if (parent.get() == this)
                {
                    m_isDirty = false;
                    return m_worldMatrix = local;
                }
                m_worldMatrix = parent->GetWorldMatrix() * local;
            }
            else
                m_worldMatrix = local;

            m_isDirty = false;
        }
        return m_worldMatrix;
    }

private:
    // ローカル変換情報
    Math::Vector3    m_localPosition = Math::Vector3::Zero;
    Math::Quaternion m_localRotation = Math::Quaternion::Identity;
    Math::Vector3    m_localScale = Math::Vector3::One;

    // キャッシュされたワールド行列
    mutable Math::Matrix m_worldMatrix = Def::Mat;
    mutable bool m_isDirty = true;

    // 階層構造
    std::weak_ptr<FlTransform> m_parent;
    std::vector<std::shared_ptr<FlTransform>> m_children;
};