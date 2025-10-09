#include "SceneManager.h"
#include "Scene.h"

namespace visualnnz
{
using namespace std;

    SceneManager& SceneManager::GetInstance()
    {
        static SceneManager instance;
        return instance;
    }

    bool SceneManager::Initialize()
    {
        return true;
    }

    void SceneManager::Update(float deltaTime)
    {
        if (m_activeScene)
        {
            m_activeScene->Update(deltaTime);
        }
    }

    void SceneManager::Render(ComPtr<ID3D11DeviceContext> context, const Camera& camera)
    {
        if (m_activeScene)
        {
            m_activeScene->Render(context, camera);
        }
    }

    void SceneManager::AddScene(const string& sceneName, shared_ptr<Scene> scene)
    {
        if (scene)
        {
            m_scenes[sceneName] = scene;
        }
    }

    void SceneManager::SetActiveScene(const string& sceneName)
    {
        auto it = m_scenes.find(sceneName);
        if (it != m_scenes.end())
        {
            m_activeScene = it->second;
        }
    }

    shared_ptr<Scene> SceneManager::GetScene(const string& sceneName)
    {
        auto it = m_scenes.find(sceneName);
        return (it != m_scenes.end()) ? it->second : nullptr;
    }
}