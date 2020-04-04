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
};

class BSDF {    
public:

    // Return f * |cos| / pdf
    virtual Spectrum Sample(MaterialRecord& matRec, Float2& s) const;
    // Return f * |cos|
    virtual Spectrum Eval(MaterialRecord& matRec) const = 0;
    // Return pdf
    virtual float Pdf(MaterialRecord& matRec) const;
    // Return f * |cos|
    virtual Spectrum EvalPdf(MaterialRecord& matRec) const = 0;

    virtual bool IsDelta() const { return false; }
};

class Light {
public:
    // Return radiance / p(omega) = radiance / (p(area) * dist^2 / cos)
    virtual Spectrum Sample(LightRecord& lightRec, Float2& s) const = 0;
    // Return radiance
    virtual Spectrum Eval(LightRecord& lightRec) const = 0;
    // Return p(omega) = p(area) * dist^2 / cos
    virtual float Pdf(LightRecord& lightRec) const = 0;
    // Return radiance
    virtual Spectrum EvalPdf(LightRecord& lightRec) const = 0;

    virtual bool IsDelta() const { return false; }
};

class Primitive {
public:
    Primitive(
        std::shared_ptr<Shape> shape,
        std::shared_ptr<BSDF> bsdf,
        std::shared_ptr<AreaLight> areaLight)
        :m_shape(shape), m_bsdf(bsdf), m_areaLight(areaLight)
    {}

    Primitive(
        std::shared_ptr<Shape> shape,
        std::shared_ptr<BSDF> bsdf)
        :m_shape(shape), m_bsdf(bsdf)
    {}

    bool IsAreaLight() const { return m_areaLight != nullptr; }

    std::shared_ptr<Shape> m_shape;
    std::shared_ptr<BSDF> m_bsdf;
    std::shared_ptr<AreaLight> m_areaLight;
};