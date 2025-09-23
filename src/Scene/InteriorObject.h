#pragma once

//#include "../Components/Transform.h"
//#include "../Components/MeshRenderer.h"
#include <memory>
#include <string>
// #include <vector>
namespace visualnnz
{
using namespace std;

// ���� ����
class Transform;
class MeshRenderer;

// �ǳ� ������Ʈ Ŭ����
class InteriorObject
{
public:
    // OB �䱸���� ������ ���� �޼���
    void SetPickedUp(bool picked) { m_isPickedUp = picked; } // OB-001
    bool IsPickedUp() const { return m_isPickedUp; }
    void SetSelected(bool selected) { m_isSelected = selected; }

    // ������Ʈ ����
    Transform *GetTransform() const { return m_transform.get(); }
    MeshRenderer *GetMeshRenderer() const { return m_meshRenderer.get(); }

    // OB-004-3 ������ ���� �޼���
    const string &GetObjectID() const { return m_objectID; }

private:
    unique_ptr<Transform> m_transform;
    unique_ptr<MeshRenderer> m_meshRenderer;
    string m_objectID;
    string m_objectType; // "chair", "table", "sofa" ��
    bool m_isSelected;
    bool m_isPickedUp; // OB-001, OB-003��
};
} // namespace visualnnz