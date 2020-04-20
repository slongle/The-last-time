#include "parse.h"

#include <fstream>

#include "scene.h"

#include "shape/triangle.h"
#include "bsdf/matte.h"
#include "bsdf/dielectric.h"
#include "bsdf/conductor.h"
#include "bsdf/blendbsdf.h"
#include "light/arealight.h"
#include "light/environment.h"
#include "integrator/pathtracer.h"
#include "integrator/pathguider.h"
#include "texture/consttexture.h"
#include "texture/imagemap.h"

#include "ext/json/json.hpp"
using json = nlohmann::json;

Spectrum ArrayToSpectrum(const json::value_type& node) {
    return Spectrum(node[0], node[1], node[2]);
}

Spectrum GetSpectrum(const json::value_type& node, const std::string name, const Spectrum& defaultValue) {    
    return node.contains(name) ? Spectrum(node[name][0], node[name][1], node[name][2]) : defaultValue;
}

Float3 GetFloat3(const json::value_type& node, const std::string name, const Float3& defaultValue) {
    return node.contains(name) ? Float3(node[name][0], node[name][1], node[name][2]) : defaultValue;
}

float GetFloat(const json::value_type& node, const std::string name, const float& defaultValue) {
    return node.contains(name) ? node[name] : defaultValue;
}

int GetInt(const json::value_type& node, const std::string name, const int& defaultValue) {
    return node.contains(name) ? node[name] : defaultValue;
}

std::shared_ptr<Texture<Spectrum>> 
GetSpectrumTexture(const json::value_type& node, const std::string name, 
    const Spectrum& defaultValue, const std::shared_ptr<Scene>& scene) 
{
    if (!node.contains(name)) {
        return std::shared_ptr<Texture<Spectrum>>(new ConstTexture<Spectrum>(defaultValue));
    }
    else {
        if (!node[name].is_string()) {
            Spectrum c = Spectrum(node[name][0], node[name][1], node[name][2]);
            return std::shared_ptr<Texture<Spectrum>>(new ConstTexture<Spectrum>(c));
        }
        else {
            return scene->GetSpectrumTexture(node[name]);
        }
    }
}

std::shared_ptr<Texture<float>>
GetFloatTexture(const json::value_type& node, const std::string name, 
    const float& defaultValue, const std::shared_ptr<Scene>& scene) 
{
    if (!node.contains(name)) {
        return std::shared_ptr<Texture<float>>(new ConstTexture<float>(defaultValue));
    }
    else {
        if (!node[name].is_string()) {
            return std::shared_ptr<Texture<float>>(new ConstTexture<float>(node[name]));
        }
        else {
            return scene->GetFloatTexture(node[name]);
        }
    }
}

std::string GetString(const json::value_type& node, const std::string name, const std::string& defaultValue) {
    return node.contains(name) ? node[name] : defaultValue;
}

void Parse(const std::string& filename, Renderer& renderer)
{
    std::filesystem::path path(filename);
    GetFileResolver()->assign(path.parent_path());

    std::ifstream io(filename);
    json sceneFile = json::parse(io);
    io.close();

    auto scene = std::make_shared<Scene>();

    // Texture
    {
        int textureNum = sceneFile["textures"].size();
        std::cout << "# of textures : " << textureNum << std::endl;
        for (auto& textureProperties : sceneFile["textures"]) {
            std::string textureName = textureProperties["name"];
            std::string type = textureProperties["type"];
            std::string filename = textureProperties["filename"];
            if (type == "float") {
                scene->AddFloatTexture(textureName, CreateFloatImageTexture(filename));
            }
            else {
                scene->AddSpectrumTexture(textureName, CreateSpectrumImageTexture(filename));
            }
        }
    }

    // BSDF
    {
        int BSDFNum = sceneFile["bsdfs"].size();
        std::cout << "# of BSDFs : " << BSDFNum << std::endl;
        for (auto& bsdfProperties : sceneFile["bsdfs"]) 
        {
            std::string BSDFName = bsdfProperties["name"];
            std::string type = bsdfProperties["type"];
            auto alpha = GetFloatTexture(bsdfProperties, "alpha", 1.f, scene);
            BSDF* bsdf = nullptr;
            if (type == "matte")
            {
                auto reflectance = GetSpectrumTexture(bsdfProperties, "reflectance", Spectrum(1.f), scene);                
                bsdf = new Matte(reflectance, alpha);
            }
            else if (type == "smooth_dielectric") {
                auto reflectance = GetSpectrumTexture(bsdfProperties, "reflectance", Spectrum(1.f), scene);
                auto transmittance = GetSpectrumTexture(bsdfProperties, "transmittance", Spectrum(1.f), scene);
                float eta = GetFloat(bsdfProperties, "eta", 1.5f);
                bsdf = new SmoothDielectric(reflectance, transmittance, eta, alpha);
            }
            else if (type == "smooth_conductor") {
                auto reflectance = GetSpectrumTexture(bsdfProperties, "reflectance", Spectrum(1.f), scene);
                std::string materialName = GetString(bsdfProperties, "material", "Al");
                Spectrum k(GetFileResolver()->string() + "/spds/" + materialName + ".k.spd");
                Spectrum eta(GetFileResolver()->string() + "/spds/" + materialName + ".eta.spd");
                bsdf = new SmoothConductor(reflectance, eta, k, alpha);
                //std::cout << k << std::endl;
                //std::cout << eta << std::endl;
            }
            else if (type == "blend") {
                auto weight = GetFloatTexture(bsdfProperties, "weight", 0.5f, scene);
                std::string bsdf1Name = bsdfProperties["bsdf1"];
                std::string bsdf2Name = bsdfProperties["bsdf2"];
                std::shared_ptr<BSDF> bsdf1 = scene->GetBSDF(bsdf1Name);
                std::shared_ptr<BSDF> bsdf2 = scene->GetBSDF(bsdf2Name);
                bsdf = new BlendBSDF(weight, bsdf1, bsdf2, alpha);
            }
            else {
                assert(false);
            }
            scene->AddBSDF(BSDFName, std::shared_ptr<BSDF>(bsdf));
        }
    }

    // Shapes
    {
        int shapeNum = sceneFile["shapes"].size();
        std::cout << "# of shapes : " << shapeNum << std::endl;
        for (auto& shape : sceneFile["shapes"])
        {
            std::string shapeName = shape["name"];
            std::string type = shape["type"];
            if (type == "mesh") {
                std::string filename = shape["filename"];
                std::shared_ptr<Mesh> mesh = LoadMesh(filename);
                scene->AddMesh(shapeName, mesh);
            } 
            else{
                assert(false);
            }
        }
    }

    // Primitives
    {
        int primitiveNum = sceneFile["primitives"].size();
        std::cout << "# of primitives : " << primitiveNum << std::endl;
        for (auto& primitiveProperties : sceneFile["primitives"]) 
        {
            std::string shapeName = primitiveProperties["shape"];
            std::vector<std::shared_ptr<Shape>> shapes;
            if (scene->m_meshes.count(shapeName)) {
                std::shared_ptr<Mesh> mesh = scene->GetMesh(shapeName);
                scene->m_orderedMeshes.push_back(mesh);
                shapes.reserve(mesh->m_triangleNum);
                for (uint32_t i = 0; i < mesh->m_triangleNum; i++){
                    shapes.emplace_back(new Triangle(mesh, i));
                }
            }
            else {
                
            }

            std::string bsdfName = primitiveProperties["bsdf"];
            std::shared_ptr<BSDF> bsdf = scene->GetBSDF(bsdfName);

            if (primitiveProperties.count("area_light")) {
                auto& areaLightProperties = primitiveProperties["area_light"];
                Spectrum radiance = GetSpectrum(areaLightProperties, "radiance", Spectrum(1.f));
                float scale = GetFloat(areaLightProperties, "scale", 1.f);
                for (const auto& shape : shapes) {
                    std::shared_ptr<AreaLight> areaLight(new AreaLight(radiance * scale, shape));
                    scene->m_primitives.emplace_back(shape, bsdf, areaLight);
                    scene->m_lights.push_back(areaLight);
                }
            }
            else {
                for (const auto& shape : shapes) {                    
                    scene->m_primitives.emplace_back(shape, bsdf);
                }
            }
        }
    }

    // Lights
    {
        int lightNum = sceneFile["lights"].size();
        std::cout << "# of lights : " << lightNum << std::endl;
        for (auto& lightProperties : sceneFile["lights"])
        {
            std::string lightType = lightProperties["type"];
            if (lightType == "environment") {
                auto texture = GetSpectrumTexture(lightProperties, "texture", Spectrum(1.f), scene);
                auto light = std::shared_ptr<EnvironmentLight>(new EnvironmentLight(texture));
                scene->m_lights.push_back(light);
                scene->m_environmentLight = light;
            }
            else {
                assert(false);
            }            
        }
    }

    std::shared_ptr<Camera> camera = nullptr;
    // Camera
    {
        auto& cameraProperties = sceneFile["camera"];
        float fov = GetFloat(cameraProperties, "fov", 25.f);
        Float3 pos = GetFloat3(cameraProperties["transform"], "position", Float3(0, 1, 0));
        Float3 look = GetFloat3(cameraProperties["transform"], "lookat", Float3(0, 0, 0));
        Float3 up = GetFloat3(cameraProperties["transform"], "up", Float3(0, 0, 1));        
        camera = std::make_shared<Camera>(LookAt(pos, look, up), fov);
    }

    std::shared_ptr<Framebuffer> buffer = nullptr;
    // Framebuffer
    {
        auto& framebuffer = sceneFile["framebuffer"];
        int width = framebuffer["resolution"][0];
        int height = framebuffer["resolution"][1];
        float scale = GetFloat(framebuffer, "scale", 1.f);
        width = int(width * scale);
        height = int(height * scale);
        std::string filename = framebuffer["filename"];
        buffer = std::make_shared<Framebuffer>(filename, width, height);
    }

    std::shared_ptr<Integrator> integrator = nullptr;
    // Integrator
    {
        auto& integratorProperties = sceneFile["integrator"];
        std::string type = integratorProperties["type"];
        if (type == "path_tracer") {
            int maxBounce = GetInt(integratorProperties, "max_bounce", 10);
            int spp = GetInt(integratorProperties, "spp", 1);
            integrator = std::make_shared<PathIntegrator>(scene, camera, buffer, maxBounce, spp);
        }
        else if (type == "path_guider") {
            int maxBounce = GetInt(integratorProperties, "max_bounce", 10);
            int initSpp = GetInt(integratorProperties, "init_spp", 1);
            int maxIteration = GetInt(integratorProperties, "max_iteration", 1);
            integrator = std::make_shared<PathGuiderIntegrator>(scene, camera, buffer, maxBounce, initSpp, maxIteration);
        }
        else {
            assert(false);
        }
    }

    renderer.m_buffer = buffer;
    renderer.m_scene = scene;
    renderer.m_integrator = integrator;
}
