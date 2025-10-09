#include "InteriorObject.h"
#include "../Components/Transform.h"
#include "../Components/MeshRenderer.h"
#include "../Character/Camera.h"

namespace visualnnz
{
using namespace std;

InteriorObject::InteriorObject()
    : m_isSelected(false)
    , m_isPickedUp(false)
{
    m_transform = make_unique<Transform>();
    m_meshRenderer = make_unique<MeshRenderer>();
}

InteriorObject::~InteriorObject()
{
}

bool InteriorObject::Initialize(ComPtr<ID3D11Device> device, 
    const string& objectID, 
    const string& objectType)
{
    m_objectID = objectID;
    m_objectType = objectType;
    
    return m_meshRenderer->Initialize(device);
}

void InteriorObject::Update(float deltaTime)
{
    // 오브젝트별 업데이트 로직
    if (m_transform)
    {
        m_transform->UpdateMatrix();
    }
}

void InteriorObject::Render(ComPtr<ID3D11DeviceContext> context, const Camera& camera)
{
    if (m_meshRenderer && m_transform)
    {
        // 월드, 뷰, 프로젝션 행렬을 셰이더에 전달하여 렌더링
        Matrix model = m_transform->GetModelMatrix();
        Matrix view = camera.GetViewMatrix();
        Matrix projection = camera.GetProjectionMatrix();

        // MeshRenderer::Render는 context만 받으므로, 
        // 행렬 전달은 MeshRenderer 내부에서 처리하거나 셰이더에 직접 전달해야 함
        m_meshRenderer->Render(context);
    }
}

}