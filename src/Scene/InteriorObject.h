#pragma once

#include <memory>
#include <string>
#include <d3d11.h>
#include <wrl.h>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;
using std::unique_ptr;
using std::string;

// 전방 선언
class Transform;
class MeshRenderer;
class Camera;

// 인테리어 오브젝트 클래스
class InteriorObject
{
public:
    InteriorObject();
    ~InteriorObject();
    
    bool Initialize(ComPtr<ID3D11Device> device, 
        const string& objectID, 
        const string& objectType);

    // OB 요구사항 관련된 상태 메서드
    void SetPickedUp(bool picked) { m_isPickedUp = picked; } // OB-001
    bool IsPickedUp() const { return m_isPickedUp; }
    void SetSelected(bool selected) { m_isSelected = selected; }
    bool IsSelected() const { return m_isSelected; }

    // 컴포넌트 접근
    Transform *GetTransform() const { return m_transform.get(); }
    MeshRenderer *GetMeshRenderer() const { return m_meshRenderer.get(); }
    
    // OB-004-3 오브젝트 식별 메서드
    const string &GetObjectID() const { return m_objectID; }
    const string &GetObjectType() const { return m_objectType; }

    void Update(float deltaTime);
    void Render(ComPtr<ID3D11DeviceContext> context, const Camera& camera);

private:
    unique_ptr<Transform> m_transform;
    unique_ptr<MeshRenderer> m_meshRenderer;
    string m_objectID;
    string m_objectType; // "chair", "table", "sofa" 등
    bool m_isSelected;
    bool m_isPickedUp; // OB-001, OB-003용
};
} // namespace visualnnz