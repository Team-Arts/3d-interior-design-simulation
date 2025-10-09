#pragma once

#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>
#include <wrl.h>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;
using std::shared_ptr;
using std::string;
using std::vector;

class Camera;
class InteriorObject;

class Scene
{
public:
    Scene(const string &sceneName);
    ~Scene();

    bool Initialize();
    void Update(float deltaTime);
    void Render(ComPtr<ID3D11DeviceContext> context, const Camera &camera);

    // 오브젝트 관리
    void AddObject(shared_ptr<InteriorObject> object);
    void RemoveObject(const string &objectID);
    shared_ptr<InteriorObject> FindObject(const string &objectID);

    // Getter
    const string &GetName() const { return m_sceneName; }
    const vector<shared_ptr<InteriorObject>> &GetObjects() const { return m_objects; }

private:
    string m_sceneName;
    vector<shared_ptr<InteriorObject>> m_objects;
};
} // namespace visualnnz