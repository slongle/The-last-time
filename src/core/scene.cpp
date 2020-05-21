#include "scene.h"
#include "light/arealight.h"
#include "light/environment.h"

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

std::shared_ptr<Medium> Scene::GetMedium(const std::string& name)
{
    if (m_media.count(name)) {
        return m_media[name];
    }
    else {
        return std::shared_ptr<Medium>(nullptr);
    }
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

void Scene::AddMedium(const std::string& name, const std::shared_ptr<Medium>& medium)
{
    assert(m_media.count(name) == 0);
    m_media[name] = medium;
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
    assert(!m_lights.empty() || !m_environmentLights.empty());
    std::cout << "Building BVH" << std::endl;
    for (Primitive& p: m_primitives) {
        m_bounds = Union(m_bounds, p.m_shape->GetBounds());
    }
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

bool Scene::IntersectTr(Ray& ray, HitRecord& hitRec, Spectrum& transmittance) const
{
    transmittance = Spectrum(1.f);
    while (true) {
        bool hit = Intersect(ray, hitRec);
        if (ray.m_medium) {
            transmittance *= ray.m_medium->Transmittance(ray);
        }
        if (!hit) {
            return false;
        }
        if (hitRec.GetBSDF()) {
            return true;
        }
        ray = Ray(hitRec.m_geoRec.m_p, ray.d, hitRec.GetMedium(ray.d));
    }
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

bool Scene::OccludeTr(Ray& _ray, Spectrum& transmittance) const
{
    Ray ray = _ray;
    float t = _ray.tMax;
    transmittance = Spectrum(1.f);
    while (true) {
        HitRecord hitRec;
        bool hit = Intersect(ray, hitRec);
        if (hit && hitRec.GetBSDF()) {
            transmittance = Spectrum(0.f);
            return true;
        }

        if (ray.m_medium) {
            transmittance *= ray.m_medium->Transmittance(ray);
        }

        if (!hit) {
            return false;
        }

        t -= ray.tMax;
        ray = Ray(hitRec.m_geoRec.m_p, ray.d, Ray::epsilon, t, hitRec.GetMedium(ray.d));
    }
}

Spectrum Scene::SampleLight(LightRecord& lightRec, const Float2& _s, const std::shared_ptr<Medium> medium) const
{
    Float2 s(_s);
    uint32_t lightNum = m_lights.size();
    /* Randomly pick an emitter */
    uint32_t lightIdx = std::min(uint32_t(lightNum * s.x), lightNum - 1);
    float lightChoosePdf = 1.f / lightNum;
    s.x = s.x * lightNum - lightIdx;
    const auto& light = m_lights[lightIdx];
    Spectrum emission = light->Sample(lightRec, s);
    lightRec.m_shadowRay.m_medium = medium;

    if (lightRec.m_pdf != 0) {
        Spectrum transmittance(0.f);
        if (OccludeTr(lightRec.m_shadowRay, transmittance)) {
            return Spectrum(0.0f);
        }
        emission *= transmittance;
        lightRec.m_pdf *= lightChoosePdf;
        emission /= lightChoosePdf;
        return emission;
    }
    else {
        return Spectrum(0.0f);
    }
}

Spectrum Scene::EvalLight(bool hit, const Ray& ray, const HitRecord& hitRec) const
{
    Spectrum emission(0.f);
    if (hit) {
        if (hitRec.m_primitive->IsAreaLight()) {
            LightRecord lightRec(ray.o, hitRec.m_geoRec);
            emission = hitRec.m_primitive->m_areaLight->Eval(lightRec);
        }
    }
    else {
        // Environment Light
        for (const auto& envLight : m_environmentLights) {
            emission += envLight->Eval(ray);
        }
    }
    return emission;
}

Spectrum Scene::EvalPdfLight(bool hit, const Ray& ray, const HitRecord& hitRec, LightRecord& lightRec) const
{
    Spectrum emission(0.f);
    if (hit) {
        if (hitRec.m_primitive->IsAreaLight()) {
            auto areaLight = hitRec.m_primitive->m_areaLight;
            lightRec = LightRecord(ray.o, hitRec.m_geoRec);
            emission = areaLight->EvalPdf(lightRec);
            lightRec.m_pdf /= m_lights.size();
        }
    }
    else {
        // Environment Light
        for (const auto& envLight : m_environmentLights) {
            lightRec = LightRecord(ray);
            emission = envLight->EvalPdf(lightRec);
            lightRec.m_pdf /= m_lights.size();
        }
    }
    return emission;
}

std::string Scene::ToString() const
{
    return fmt::format("Scene\n# of primitives : {0}\n# of lights : {1}\nBounds : \n{2}", 
        m_primitives.size(), m_lights.size(), m_bounds.ToString());
}
