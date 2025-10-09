#pragma once

#include <memory>
#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include <directxtk/SimpleMath.h>

namespace visualnnz
{
using Microsoft::WRL::ComPtr;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Ray;
using std::shared_ptr;
using std::vector;

class InteriorObject;
class Camera;

struct PickResult
{
    shared_ptr<InteriorObject> object;
    Vector3 hitPoint;
    float distance;
    bool hit;

    PickResult() : object(nullptr), hitPoint(Vector3::Zero), distance(0.0f), hit(false) {}
};

class ObjectPicker
{
public:
    ObjectPicker();
    ~ObjectPicker();

    // ���콺 ��ġ�κ��� ���� ����
    Ray CreatePickingRay(int mouseX, int mouseY, 
                        int screenWidth, int screenHeight,
                        const Camera& camera);

    // ���̿� ������Ʈ���� ���� �˻�
    PickResult PickObject(const Ray& ray, 
                         const vector<shared_ptr<InteriorObject>>& objects);

    // ���콺 ��ǥ�� ���� ������Ʈ ��ŷ
    PickResult PickObjectAtScreenPos(int mouseX, int mouseY,
                                   int screenWidth, int screenHeight,
                                   const Camera& camera,
                                   const vector<shared_ptr<InteriorObject>>& objects);

private:
    // ���̿� �ٿ�� �ڽ� ���� �˻�
    bool RayIntersectsBoundingBox(const Ray& ray, 
                                 const Vector3& minBounds, 
                                 const Vector3& maxBounds,
                                 float& distance);
};

} // namespace visualnnz