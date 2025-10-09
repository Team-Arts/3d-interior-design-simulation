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
    // ������Ʈ�� ������Ʈ ����
    if (m_transform)
    {
        m_transform->UpdateMatrix();
    }
}

void InteriorObject::Render(ComPtr<ID3D11DeviceContext> context, const Camera& camera)
{
    if (m_meshRenderer && m_transform)
    {
        // ����, ��, �������� ����� ���̴��� �����Ͽ� ������
        Matrix model = m_transform->GetModelMatrix();
        Matrix view = camera.GetViewMatrix();
        Matrix projection = camera.GetProjectionMatrix();

        // MeshRenderer::Render�� context�� �����Ƿ�, 
        // ��� ������ MeshRenderer ���ο��� ó���ϰų� ���̴��� ���� �����ؾ� ��
        m_meshRenderer->Render(context);
    }
}

}