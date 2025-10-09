#pragma once

#include <memory>
#include <vector>
#include <directxtk/SimpleMath.h>

#include "AppBase.h"

namespace visualnnz
{
using DirectX::SimpleMath::Vector3;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::wstring;

// ���� ����
class Renderer;
class InteriorObject;
class Mesh;
class Shader;

// AppBase�� ��ӹ޾� ���� ���α׷� ������ �����ϴ� Ŭ����
class MainApp : public AppBase
{
public:
    MainApp();
    virtual ~MainApp() override;

    // AppBase ���� �Լ� �������̵�
    virtual bool Initialize(HINSTANCE hInstance, int nCmdShow) override;
    virtual void Update(float deltaTime) override;
    virtual void Render() override;

protected:
    // AppBase���� �䱸�ϴ� ���� �Լ��� ����
    virtual bool InitializeGraphics() override;
    virtual void CreateTestObjects() override;
    virtual void InitializeManagers() override;

    // �̺�Ʈ �ڵ鷯 �������̵� (�ʿ��� �͸�)
    virtual void OnModelSpawn(const std::string& modelName) override;

private:
    // MainApp ���� �����
    unique_ptr<Renderer> m_renderer;

    // �׽�Ʈ ������Ʈ��
    unique_ptr<InteriorObject> m_testCube;
    shared_ptr<Mesh> m_cubeMesh;
    shared_ptr<Shader> m_basicShader;
};

} // namespace visualnnz
