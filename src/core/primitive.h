#pragma once

#include "global.h"
#include "vector.h"
#include "spectrum.h"
#include "record.h"
#include "camera.h"
#include "sampling.h"
#include "texture.h"

class AreaLight;

class Shape {    
public:
    virtual bool Intersect(Ray& ray, HitRecord& hitRec) const = 0;
    virtual void SetGeometryRecord(GeometryRecord& geoRec) const = 0;
    virtual void Sample(GeometryRecord& geoRec, Float2& s) const = 0;
    virtual float Pdf(GeometryRecord& geoRec) const = 0;
    virtual float Area() const = 0;
    virtual Bounds GetBounds() const = 0;
};

class BSDF {
public:
    BSDF(const std::shared_ptr<Texture<float>>& alpha) : m_alpha(alpha) {}

    // Return f * |cos| / pdf
    virtual Spectrum Sample(MaterialRecord& matRec, Float2 s) const;
    // Return f * |cos|
    virtual Spectrum Eval(MaterialRecord& matRec) const = 0;
    // Return pdf
    virtual float Pdf(MaterialRecord& matRec) const;
    // Return f * |cos|
    virtual Spectrum EvalPdf(MaterialRecord& matRec) const = 0;

    virtual bool IsDelta(const Float2& st) const;
    virtual bool IsTransparent() const;
protected:
    std::shared_ptr<Texture<float>> m_alpha;
};

class PhaseFunction {
public:
    // Return f * |cos|
    virtual Spectrum EvalPdf(PhaseFunctionRecord& phaseRec) const = 0;
    // Return f * |cos| / pdf
    virtual Spectrum Sample(PhaseFunctionRecord& phaseRec, Float2& s) const = 0;
};

class Medium {
public:
    Medium(const std::shared_ptr<PhaseFunction>& pf) :m_phaseFunction(pf) {}

    virtual Spectrum Sample(const Ray& ray, MediumRecord& mediumRec, Sampler& sampler) const = 0;
    virtual Spectrum Transmittance(const Ray& ray, Sampler& sampler) const = 0;

    std::shared_ptr<PhaseFunction> m_phaseFunction;
};

class MediumInterface {
public:
    MediumInterface(
        std::shared_ptr<Medium> insideMedium,
        std::shared_ptr<Medium> outsideMedium)
        :m_insideMedium(insideMedium), m_outsideMedium(outsideMedium) {}

    std::shared_ptr<Medium> m_insideMedium;
    std::shared_ptr<Medium> m_outsideMedium;
};

class Light {
public:
    Light(const MediumInterface& mi) :m_mediumInterface(mi) {}

    // Return radiance / p(omega)
    virtual Spectrum Sample(LightRecord& lightRec, Float2& s) const = 0;
    // Return radiance
    virtual Spectrum Eval(LightRecord& lightRec) const = 0;
    // Return p(omega) = p(area) * dist^2 / cos
    virtual float Pdf(LightRecord& lightRec) const = 0;
    // Return radiance
    virtual Spectrum EvalPdf(LightRecord& lightRec) const = 0;
    // Sample photon's position, direction and flux
    virtual Spectrum SamplePhoton(Float2& s1, Float2& s2, Ray& ray) const = 0;

    virtual bool IsDelta() const { return false; }

    MediumInterface m_mediumInterface;
};

class Primitive {
public:
    Primitive(
        std::shared_ptr<Shape> shape,
        std::shared_ptr<BSDF> bsdf,
        std::shared_ptr<AreaLight> areaLight,
        MediumInterface mi)
        :m_shape(shape), m_bsdf(bsdf), m_areaLight(areaLight), m_mediumInterface(mi)
    {}

    Primitive(
        std::shared_ptr<Shape> shape,
        std::shared_ptr<BSDF> bsdf,
        MediumInterface mi)
        :m_shape(shape), m_bsdf(bsdf), m_mediumInterface(mi)
    {}
    
    bool IsAreaLight() const { return m_areaLight != nullptr; }

    std::shared_ptr<Shape> m_shape;
    std::shared_ptr<BSDF> m_bsdf;
    std::shared_ptr<AreaLight> m_areaLight;
    MediumInterface m_mediumInterface;
};