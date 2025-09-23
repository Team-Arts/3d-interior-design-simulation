#pragma once

//#include "../Components/Transform.h"
//#include "../Components/MeshRenderer.h"
#include <memory>
#include <string>
// #include <vector>
namespace visualnnz
{
using namespace std;

// 전방 선언
class Transform;
class MeshRenderer;

// 실내 오브젝트 클래스
class InteriorObject
{
public:
    // OB 요구사항 구현을 위한 메서드
    void SetPickedUp(bool picked) { m_isPickedUp = picked; } // OB-001
    bool IsPickedUp() const { return m_isPickedUp; }
    void SetSelected(bool selected) { m_isSelected = selected; }

    // 컴포넌트 접근
    Transform *GetTransform() const { return m_transform.get(); }
    MeshRenderer *GetMeshRenderer() const { return m_meshRenderer.get(); }

    // OB-004-3 삭제를 위한 메서드
    const string &GetObjectID() const { return m_objectID; }

private:
    unique_ptr<Transform> m_transform;
    unique_ptr<MeshRenderer> m_meshRenderer;
    string m_objectID;
    string m_objectType; // "chair", "table", "sofa" 등
    bool m_isSelected;
    bool m_isPickedUp; // OB-001, OB-003용
};
} // namespace visualnnz