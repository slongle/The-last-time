#pragma once

#include "vector.h"
#include "sampling.h"
#include "camera.h"

class Primitive;
class BSDF;

class GeometryRecord {
public:
    Float2 m_uv;
    Float3 m_p;
    Float2 m_st;
    Float3 m_ng, m_ns;
    float m_pdf;
};

class HitRecord {
public:

    std::shared_ptr<Medium> GetMedium(const Float3& d) const;
    std::shared_ptr<BSDF> GetBSDF() const;

    float m_t;
    GeometryRecord m_geoRec;
    const Primitive* m_primitive = nullptr;
};

enum TransportMode {
    Radiance, 
    Importance
};

class MaterialRecord {
public:
    MaterialRecord() :m_eta(1) {}
    MaterialRecord(const Float3& wi, const Float3& ns, const Float2& st, 
        const TransportMode& mode = Radiance)
        :m_frame(ns), m_wi(m_frame.ToLocal(wi)), m_st(st), m_mode(mode), m_eta(1) {}
    MaterialRecord(const Float3& wi, const Float3& wo, const Float3& ns, const Float2& st, 
        const TransportMode& mode = Radiance)
        :m_frame(ns), m_wi(m_frame.ToLocal(wi)), m_wo(m_frame.ToLocal(wo)), m_st(st), m_mode(mode), m_eta(1) {}

    Float3 ToLocal(const Float3& v) const { return m_frame.ToLocal(v); }
    Float3 ToWorld(const Float3& v) const { return m_frame.ToWorld(v); }

    Frame m_frame;
    Float2 m_st;

    Float3 m_wo, m_wi;
    float m_pdf;
    float m_eta;

    TransportMode m_mode;
};

class PhaseFunctionRecord {
public:
    PhaseFunctionRecord() {}
    PhaseFunctionRecord(const Float3& wi) :m_wi(wi) {}
    PhaseFunctionRecord(const Float3& wi, const Float3& wo) :m_wi(wi), m_wo(wo) {}

    Float3 m_wo, m_wi;
    float m_pdf;
};

class MediumRecord {
public:
    MediumRecord() {}

    bool m_internal = false;
    float m_t;
    float m_pdf;
    Float3 m_p;
};

class LightRecord {
public:
    LightRecord() {}
    LightRecord(const Float3& _ref) :m_ref(_ref) {}
    LightRecord(const Float3& _ref, const GeometryRecord& _geoRec) 
        :m_ref(_ref), m_geoRec(_geoRec) {}
    LightRecord(const Ray& ray) :m_ref(ray.o), m_wi(ray.d) {}

    Float3 m_ref;

    GeometryRecord m_geoRec;
    Float3 m_wi;
    float m_pdf;
    Ray m_shadowRay;    
};

class DebugRecord { 
public:
    DebugRecord() :m_debugRay(false), m_debugKDTree(false) {}

    void SetDebugRay(const Float2& pos) {
        m_debugRay = true;
        m_rasterPosition = pos;
    }

    bool m_debugRay;
    Float2 m_rasterPosition;
    bool m_debugKDTree;
};