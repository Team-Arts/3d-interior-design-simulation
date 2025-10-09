#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <d3d11.h>
#include <wrl.h>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;
using std::shared_ptr;
using std::string;
using std::unordered_map;

class Scene;
class Camera;

class SceneManager
{
public:
    static SceneManager &GetInstance();

    bool Initialize();
    void Update(float deltaTime);
    void Render(ComPtr<ID3D11DeviceContext> context, const Camera &camera);

    // ¾À °ü¸®
    void AddScene(const string &sceneName, shared_ptr<Scene> scene);
    void SetActiveScene(const string &sceneName);
    shared_ptr<Scene> GetActiveScene() const { return m_activeScene; }
    shared_ptr<Scene> GetScene(const string &sceneName);

private:
    SceneManager() = default;
    ~SceneManager() = default;
    SceneManager(const SceneManager &) = delete;
    SceneManager &operator=(const SceneManager &) = delete;

    unordered_map<string, shared_ptr<Scene>> m_scenes;
    shared_ptr<Scene> m_activeScene;
};
} // namespace visualnnz