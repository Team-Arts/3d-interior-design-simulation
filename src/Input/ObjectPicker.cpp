#include "ObjectPicker.h"
#include "../Scene/InteriorObject.h"
#include "../Character/Camera.h"
#include "../Components/Transform.h"
#include <algorithm>

namespace visualnnz
{
using namespace DirectX::SimpleMath;
using namespace std;

ObjectPicker::ObjectPicker() = default;
ObjectPicker::~ObjectPicker() = default;

Ray ObjectPicker::CreatePickingRay(int mouseX, int mouseY, 
                                  int screenWidth, int screenHeight,
                                  const Camera& camera)
{
    // ���콺 ��ǥ(��ũ�� ��ǥ��)�� NDC ��ǥ�� ��ȯ
    float ndcX = (2.0f * mouseX) / screenWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / screenHeight;

    // �� ���������� ���� ����
    Vector3 rayDirView = Vector3(ndcX, ndcY, 1.0f);

    // ���� ����� ������� ���� �� �������� ��ȯ
    Matrix projMatrix = camera.GetProjectionMatrix();
    Matrix invProj = projMatrix.Invert();
    rayDirView = Vector3::Transform(rayDirView, invProj);
    rayDirView.z = 1.0f;
    rayDirView.Normalize();

    // �� ����� ������� ���� ���� �������� ��ȯ
    Matrix viewMatrix = camera.GetViewMatrix();
    Matrix invView = viewMatrix.Invert();
    
    Vector3 rayOriginWorld = Vector3::Transform(Vector3::Zero, invView);
    Vector3 rayDirWorld = Vector3::TransformNormal(rayDirView, invView);
    rayDirWorld.Normalize();

    Ray ray;
    ray.position = rayOriginWorld;
    ray.direction = rayDirWorld;

    return ray;
}

PickResult ObjectPicker::PickObject(const Ray& ray, 
                                   const vector<shared_ptr<InteriorObject>>& objects)
{
    PickResult result;
    float closestDistance = FLT_MAX;

    for (const auto& obj : objects)
    {
        if (!obj) continue;

        // ������ �ٿ�� �ڽ� �˻� (�����δ� �޽��� �ٿ�� �ڽ��� ����ؾ� ��)
        Vector3 position = obj->GetTransform()->GetPosition();
        Vector3 scale = obj->GetTransform()->GetScale();
        
        Vector3 minBounds = position - scale * 0.5f;
        Vector3 maxBounds = position + scale * 0.5f;

        float distance;
        if (RayIntersectsBoundingBox(ray, minBounds, maxBounds, distance))
        {
            if (distance < closestDistance)
            {
                closestDistance = distance;
                result.object = obj;
                result.hitPoint = ray.position + ray.direction * distance;
                result.distance = distance;
                result.hit = true;
            }
        }
    }

    return result;
}

PickResult ObjectPicker::PickObjectAtScreenPos(int mouseX, int mouseY,
                                              int screenWidth, int screenHeight,
                                              const Camera& camera,
                                              const vector<shared_ptr<InteriorObject>>& objects)
{
    Ray ray = CreatePickingRay(mouseX, mouseY, screenWidth, screenHeight, camera);
    return PickObject(ray, objects);
}

bool ObjectPicker::RayIntersectsBoundingBox(const Ray& ray, 
                                           const Vector3& minBounds, 
                                           const Vector3& maxBounds,
                                           float& distance)
{
    Vector3 invDir = Vector3(1.0f / ray.direction.x, 1.0f / ray.direction.y, 1.0f / ray.direction.z);
    
    Vector3 t1 = (minBounds - ray.position) * invDir;
    Vector3 t2 = (maxBounds - ray.position) * invDir;
    
    Vector3 tMin = Vector3::Min(t1, t2);
    Vector3 tMax = Vector3::Max(t1, t2);
    
    // initializer_list ��� ���� �� ���
    float tNear = max(max(tMin.x, tMin.y), tMin.z);
    float tFar = min(min(tMax.x, tMax.y), tMax.z);
    
    if (tNear > tFar || tFar < 0) return false;
    
    distance = tNear > 0 ? tNear : tFar;
    return true;
}

} // namespace visualnnz