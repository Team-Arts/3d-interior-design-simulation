#include "Scene.h"
#include "InteriorObject.h"

namespace visualnnz
{
using namespace std;
    Scene::Scene(const string& sceneName)
        : m_sceneName(sceneName)
    {
    }

    Scene::~Scene()
    {
    }

    bool Scene::Initialize()
    {
        // �� �ʱ�ȭ ����
        return true;
    }

    void Scene::Update(float deltaTime)
    {
        // ��� ������Ʈ ������Ʈ
        for (auto& object : m_objects)
        {
            if (object)
            {
                object->Update(deltaTime);
            }
        }
    }

    void Scene::Render(ComPtr<ID3D11DeviceContext> context, const Camera& camera)
    {
        // ��� ������Ʈ ������
        for (auto& object : m_objects)
        {
            if (object)
            {
                object->Render(context, camera);
            }
        }
    }

    void Scene::AddObject(shared_ptr<InteriorObject> object)
    {
        if (object)
        {
            m_objects.push_back(object);
        }
    }

    void Scene::RemoveObject(const string& objectID)
    {
        m_objects.erase(
            remove_if(m_objects.begin(), m_objects.end(),
                [&objectID](const shared_ptr<InteriorObject>& obj) {
                    return obj && obj->GetObjectID() == objectID;
                }),
            m_objects.end());
    }

    shared_ptr<InteriorObject> Scene::FindObject(const string& objectID)
    {
        for (auto& object : m_objects)
        {
            if (object && object->GetObjectID() == objectID)
            {
                return object;
            }
        }
        return nullptr;
    }
}