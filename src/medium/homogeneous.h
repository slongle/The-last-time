#pragma once

#include "core/primitive.h"

class HomogeneousMedium :public Medium {
public:
    HomogeneousMedium(
        const std::shared_ptr<PhaseFunction>& pf,
        const Spectrum& sigmaA,
        const Spectrum& sigmaS)
        :Medium(pf), m_sigmaA(sigmaA), m_sigmaS(sigmaS), m_sigmaT(sigmaA + sigmaS) {}

    HomogeneousMedium(
        const std::shared_ptr<PhaseFunction>& pf,
        const Spectrum& density,
        const Spectrum& albedo,
        const float& scale)
        :Medium(pf), m_density(density), m_albedo(albedo), m_scale(scale)
    {
        m_sigmaT = density * scale;
        m_sigmaS = m_sigmaT * m_albedo;
        m_sigmaA = m_sigmaT - m_sigmaS;
    }

    Spectrum Sample(const Ray& ray, MediumRecord& mediumRec, Sampler& sampler) const {
        float maxT = std::min(ray.tMax, std::numeric_limits<float>::max()), t;

        uint32_t channel = std::min(int(sampler.Next1D() * 3), 2);
        t = -std::log(1 - sampler.Next1D()) / m_sigmaT[channel];
        mediumRec.m_internal = t < maxT;
        t = std::min(t, maxT);
        Spectrum Tr = Exp(m_sigmaT * (-t));

        float pdf;
        if (mediumRec.m_internal) {
            mediumRec.m_p = ray(t);
            pdf = (Tr * m_sigmaT).Average();
        }
        else {
            pdf = Tr.Average();
        }

        mediumRec.m_t = t;
        mediumRec.m_pdf = pdf;
        return mediumRec.m_internal ? m_sigmaS * Tr / pdf : Tr / pdf;
    }

    Spectrum Transmittance(const Ray& ray, Sampler& sampler) const {
        float t = std::min(ray.tMax, std::numeric_limits<float>::max());
        Spectrum Tr = Exp(m_sigmaT * (-t));
        return Tr;
    }

    Spectrum m_sigmaA, m_sigmaS, m_sigmaT;
    Spectrum m_density, m_albedo;
    float m_scale;
};