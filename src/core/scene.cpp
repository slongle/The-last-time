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
    //assert(m_BSDFs.count(name) == 1);
    if (m_BSDFs.count(name)) {
        return m_BSDFs[name];
    }
    else {
        return std::shared_ptr<BSDF>(nullptr);
    }
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
    assert(!m_lights.empty() || m_environmentLight);
    std::cout << "Building BVH" << std::endl;
    //m_bvh = std::shared_ptr<BVH>(new BVH(m_primitives));
    //m_bvh->Build();
    m_embreeBvh = std::shared_ptr<EmbreeBVH>(new EmbreeBVH(m_orderedMeshes, m_primitives));
    std::cout << "BVH Done" << std::endl;
}

bool Scene::Intersect(Ray& ray, HitRecord& hitRec) const
{
    return m_embreeBvh->Intersect(ray, hitRec);
    return m_bvh->Intersect(ray, hitRec);
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
    return m_embreeBvh->Occlude(ray);
    return m_bvh->Occlude(ray);
    HitRecord hitRec;
    for (const Primitive& primitive : m_primitives) {
        if (primitive.m_shape->Intersect(ray, hitRec)) {
            return true;
        }
    }
    return false;
}

Spectrum Scene::SampleLight(LightRecord& lightRec, const Float2& _s) const
{
    Float2 s(_s);
    uint32_t lightNum = m_lights.size();
    /* Randomly pick an emitter */
    uint32_t lightIdx = std::min(uint32_t(lightNum * s.x), lightNum - 1);
    float lightChoosePdf = 1.f / lightNum;
    s.x = s.x * lightNum - lightIdx;
    const auto& light = m_lights[lightIdx];
    Spectrum emission = light->Sample(lightRec, s);

    if (lightRec.m_pdf != 0) {
        if (Occlude(lightRec.m_shadowRay)) {
            return Spectrum(0.0f);
        }                            
        lightRec.m_pdf *= lightChoosePdf;
        emission /= lightChoosePdf;
        return emission;
    }
    else {
        return Spectrum(0.0f);
    }
}

std::string Scene::ToString() const
{
    return fmt::format("Scene\n# of primitives : {0}\n# of lights : {1}", 
        m_primitives.size(), m_lights.size());
}
