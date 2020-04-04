#pragma once

#include "camera.h"
#include "primitive.h"
#include "shape/triangle.h"
#include "texture.h"

class Scene {
public:

    void Setup();

    bool Intersect(Ray& ray, HitRecord& hitRec) const;
    bool Occlude(Ray& ray) const;

    std::vector<Primitive> m_primitives;
    std::vector<std::shared_ptr<Light>> m_lights;

    void AddMesh(const std::string& name, const std::shared_ptr<Mesh>& mesh);
    void AddShape(const std::string& name, const std::shared_ptr<Shape>& shape);
    void AddBSDF(const std::string& name, const std::shared_ptr<BSDF>& bsdf);
    void AddFloatTexture(const std::string& name, const std::shared_ptr<Texture<float>>& texture);
    void AddSpectrumTexture(const std::string& name, const std::shared_ptr<Texture<Spectrum>>& texture);

    std::shared_ptr<Mesh> GetMesh(const std::string& name);
    std::shared_ptr<Shape> GetShape(const std::string& name);
    std::shared_ptr<BSDF> GetBSDF(const std::string& name);
    std::shared_ptr<Texture<float>> GetFloatTexture(const std::string& name);
    std::shared_ptr<Texture<Spectrum>> GetSpectrumTexture(const std::string& name);
    
    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;
    std::unordered_map<std::string, std::shared_ptr<Shape>> m_shapes;
    std::unordered_map<std::string, std::shared_ptr<BSDF>> m_BSDFs;
    std::unordered_map<std::string, std::shared_ptr<Texture<float>>> m_floatTextures;
    std::unordered_map<std::string, std::shared_ptr<Texture<Spectrum>>> m_spectrumTextures;
};