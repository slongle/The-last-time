#pragma once

#include "transform.h"
#include "sampler.h"

class Medium;

class Camera {
public:
    Camera() {}
    Camera(const Transform& c2w, const float& fov, const std::shared_ptr<Medium>& m)
        :m_cameraToWorld(c2w), m_fov(fov), m_medium(m)
    {}

    void Setup(const uint32_t& w, const uint32_t& h) {
        Transform w2s = Scale(0.5f * w, 0.5f * h, 1.f) * Translate(1.f, 1.f, 0.f) *
            Perspective(m_fov, w, h) * Inverse(m_cameraToWorld);
        m_screenToWorld = Inverse(w2s);
    }

    void GenerateRay(const Float2& pos, Sampler& sampler, Ray& ray) const {
        Float3 p = m_screenToWorld.TransformPoint(Float3(pos + sampler.Next2D(), -0.1f));
        Float3 o = m_cameraToWorld.TransformPoint(Float3(0, 0, 0));
        ray = Ray(o, Normalize(p - o), m_medium);
    }

    Transform m_screenToWorld, m_cameraToWorld;
    float m_fov;
    std::shared_ptr<Medium> m_medium;
};