#include "scene.h"

std::shared_ptr<Mesh> Scene::GetMesh(const std::string& name)
{
    assert(m_meshes.count(name) == 1);
    return m_meshes[name];
}

std::shared_ptr<Shape> Scene::GetShape(const std::string& name)
{
    assert(m_shapes.count(name) == 1);
    return m_shapes[name];
}

std::shared_ptr<BSDF> Scene::GetBSDF(const std::string& name)
{
    assert(m_BSDFs.count(name) == 1);
    return m_BSDFs[name];
}

std::shared_ptr<Texture<float>> Scene::GetFloatTexture(const std::string& name)
{
    assert(m_floatTextures.count(name) == 1);
    return m_floatTextures[name];    
}

std::shared_ptr<Texture<Spectrum>> Scene::GetSpectrumTexture(const std::string& name)
{
    assert(m_spectrumTextures.count(name) == 1);
    return m_spectrumTextures[name];
}


void Scene::AddMesh(const std::string& name, const std::shared_ptr<Mesh>& mesh)
{
    assert(m_meshes.count(name) == 0 && m_shapes.count(name) == 0);
    m_meshes[name] = mesh;
}

void Scene::AddShape(const std::string& name, const std::shared_ptr<Shape>& shape)
{
    assert(m_meshes.count(name) == 0 && m_shapes.count(name) == 0);
    m_shapes[name] = shape;
}

void Scene::AddBSDF(const std::string& name, const std::shared_ptr<BSDF>& bsdf)
{
    assert(m_BSDFs.count(name) == 0);
    m_BSDFs[name] = bsdf;
}

void Scene::AddFloatTexture(const std::string& name, const std::shared_ptr<Texture<float>>& texture)
{
    assert(m_floatTextures.count(name) == 0);
    m_floatTextures[name] = texture;
}

void Scene::AddSpectrumTexture(const std::string& name, const std::shared_ptr<Texture<Spectrum>>& texture)
{
    assert(m_spectrumTextures.count(name) == 0);
    m_spectrumTextures[name] = texture;
}

void Scene::Setup()
{
    assert(!m_lights.empty());
}

bool Scene::Intersect(Ray& ray, HitRecord& hitRec) const
{
    bool hit = false;
    for (const Primitive& primitive : m_primitives) {
        if (primitive.m_shape->Intersect(ray, hitRec)) {
            ray.tMax = hitRec.m_t;
            hitRec.m_primitive = &primitive;
            hit = true;
        }
    }
    if (hit) {
        hitRec.m_primitive->m_shape->SetGeometryRecord(hitRec.m_geoRec);
    }
    return hit;
}

bool Scene::Occlude(Ray& ray) const
{
    HitRecord hitRec;
    for (const Primitive& primitive : m_primitives) {
        if (primitive.m_shape->Intersect(ray, hitRec)) {
            return true;
        }
    }
    return false;
}
