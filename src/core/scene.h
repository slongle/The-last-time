#pragma once

#include "accelerator/bvh.h"
#include "accelerator/embreebvh.h"
#include "camera.h"
#include "primitive.h"
#include "texture.h"
#include "shape/triangle.h"
#include "light/environment.h"

class Scene {
public:
    void Setup();

    bool Intersect(Ray& ray, HitRecord& hitRec) const;
    bool IntersectTr(Ray& ray, HitRecord& hitRec, Spectrum& transmittance) const;
    bool Occlude(Ray& ray) const;
    bool OccludeTr(Ray& ray, Spectrum& transmittance) const;

    Spectrum SampleLight(LightRecord& lightRec, const Float2& s, const std::shared_ptr<Medium> medium = nullptr) const;
    Spectrum EvalLight(bool hit, const Ray& ray, const HitRecord& hitRec) const;
    Spectrum EvalPdfLight(bool hit, const Ray& ray, const HitRecord& hitRec, LightRecord& lightRec) const;
    std::string ToString() const;

    std::shared_ptr<BVH> m_bvh = nullptr;
    std::shared_ptr<EmbreeBVH> m_embreeBvh = nullptr;
    std::vector<Primitive> m_primitives;
    std::vector<std::shared_ptr<Light>> m_lights;
    std::shared_ptr<EnvironmentLight> m_environmentLight = nullptr;
    Bounds m_bounds;


public:
    void AddMesh(const std::string& name, const std::shared_ptr<Mesh>& mesh);
    void AddShape(const std::string& name, const std::shared_ptr<Shape>& shape);    
    void AddMedium(const std::string& name, const std::shared_ptr<Medium>& medium);
    void AddBSDF(const std::string& name, const std::shared_ptr<BSDF>& bsdf);
    void AddFloatTexture(const std::string& name, const std::shared_ptr<Texture<float>>& texture);
    void AddSpectrumTexture(const std::string& name, const std::shared_ptr<Texture<Spectrum>>& texture);

    std::shared_ptr<Mesh> GetMesh(const std::string& name);
    std::shared_ptr<Shape> GetShape(const std::string& name);
    std::shared_ptr<Medium> GetMedium(const std::string& name);
    std::shared_ptr<BSDF> GetBSDF(const std::string& name);
    std::shared_ptr<Texture<float>> GetFloatTexture(const std::string& name);
    std::shared_ptr<Texture<Spectrum>> GetSpectrumTexture(const std::string& name);
    
    std::vector<std::shared_ptr<Mesh>> m_orderedMeshes;
    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;
    std::unordered_map<std::string, std::shared_ptr<Shape>> m_shapes;
    std::unordered_map<std::string, std::shared_ptr<Medium>> m_media;
    std::unordered_map<std::string, std::shared_ptr<BSDF>> m_BSDFs;
    std::unordered_map<std::string, std::shared_ptr<Texture<float>>> m_floatTextures;
    std::unordered_map<std::string, std::shared_ptr<Texture<Spectrum>>> m_spectrumTextures;
};