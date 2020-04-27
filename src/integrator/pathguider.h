#pragma once

#include "core/integrator.h"

#include <atomic>
#include <thread>
#include <mutex>

class DTreeNode {
public:
    DTreeNode(const float& w = 0) :m_weight(w), m_children{ -1,-1,-1,-1 } {}
    DTreeNode(const DTreeNode& s) :m_children(s.m_children) {
        m_weight.store(s.m_weight, std::memory_order_relaxed);
    }

    DTreeNode& operator = (const DTreeNode& s) {
        m_weight.store(s.m_weight, std::memory_order_relaxed);
        m_children = s.m_children;
        return *this;
    }
    
    bool IsLeaf() const { return m_children[0] == -1; }

    std::atomic<float> m_weight;

    std::array<int, 4> m_children;
};

class DTree {
public:
    DTree() {
        m_nodes.emplace_back();
        std::cout << m_nodes.size() << std::endl;
    }

    Float3 Sample(Sampler& sampler) const;
    float Pdf(const Float3& d) const;
    void AddSample(const Float3& s, const float& weight);
    std::array<float, 4> GetChildrenPDF(const uint32_t& idx) const;
    std::array<float, 4> GetChildrenCDF(const uint32_t& idx) const;
    void Subdivide(const float& threshold, const float& totalWeight);
    void Clear();
    uint32_t GetDepth() const { return m_depth; }
    static Float2 DirectionToCanonical(const Float3& d);
    static Float3 CanonicalToDirection(const Float2& p);
    
    std::vector<DTreeNode> m_nodes;

    uint32_t m_depth = 1;
};

class DTreeWrapper {
public:
    DTreeWrapper() :m_sampleNum(0), m_weight(0) {}
    DTreeWrapper(const DTreeWrapper& dtree)
        :m_train(dtree.m_train), m_render(dtree.m_render)
    {
        m_sampleNum.store(dtree.m_sampleNum, std::memory_order_relaxed);
        m_weight.store(dtree.m_weight, std::memory_order_relaxed);
    }
    DTreeWrapper(const DTreeWrapper& dtree, const uint32_t& sampleNum)
        :m_train(dtree.m_train), m_render(dtree.m_render), 
         m_sampleNum(sampleNum) 
    {
        m_weight.store(dtree.m_weight, std::memory_order_relaxed);
    }

    Float3 Sample(Sampler& sampler) const { return m_render.Sample(sampler); }
    float Pdf(const Float3& d) const { return m_render.Pdf(d); }
    uint32_t GetSampleNum() const { return  m_sampleNum; }
    void AddSample(const Float3& d, const float& w);    
    void Refine(const float& threshold);
    void Clear();

    uint32_t GetDepth() const { return m_depth; }

    DTree m_train;
    DTree m_render;

    std::atomic<uint32_t> m_sampleNum;
    std::atomic<float> m_weight;

    uint32_t m_depth = 1;
};

class SDTreeNode {
public:
    SDTreeNode() {}
    SDTreeNode(const SDTreeNode& s) 
        :m_bounds(s.m_bounds), m_axis(s.m_axis), 
        m_children{ s.m_children }, m_dtree(s.m_dtree) {}

    SDTreeNode(const Bounds& bounds) :m_bounds(bounds), m_axis(-1), m_children{ -1,-1 } {}
    SDTreeNode(const Bounds& bounds, const DTreeWrapper& dtree, const uint32_t& sampleNum)
        :m_bounds(bounds), m_axis(-1), m_children{ -1,-1 }, m_dtree(dtree, sampleNum) {}
    
    bool IsLeaf() const { return m_children[0] == -1; }
    
    uint32_t GetSampleNum() const { return m_dtree.GetSampleNum(); }

    DTreeWrapper m_dtree;

    Bounds m_bounds;
    int m_axis;
    std::array<int, 2> m_children;
};

class SDTree {
public:
    SDTree() {}
    SDTree(const Bounds& bounds) { m_nodes.push_back(SDTreeNode(bounds)); }
    
    DTreeWrapper* GetDTreeWrapper(const Float3& p);
    void RefineSTree(const uint32_t& limit);
    void RefineDTree(const float& threshold);
    void Subdivide(const uint32_t& idx, const uint32_t& limit);

    std::string ToString() const {
        return fmt::format("# leaf in STree : {0}\nAverage depth in DTree : {1}\nMax depth in DTree : {2}",
            m_leafNum, m_averageDTreeDepth, m_maxDTreeDepth);
    }

    std::vector<SDTreeNode> m_nodes;
    // Statistics
    uint32_t m_leafNum = 1;
    uint32_t m_maxDTreeDepth = 1;
    uint32_t m_averageDTreeDepth = 1;
};

class GuideMaterialRecord :public MaterialRecord {
public:
    GuideMaterialRecord(const MaterialRecord& matRec, std::shared_ptr<BSDF> bsdf, DTreeWrapper* dtree)
        :MaterialRecord(matRec), m_bsdf(bsdf), m_dtree(dtree) {}

    float m_woPdf;
    float m_bsdfPdf;
    float m_dtreePdf;
    std::shared_ptr<BSDF> m_bsdf;
    DTreeWrapper* m_dtree;
};

class Vertex {
public:    
    void Commit();

    DTreeWrapper* m_dtree;
    Float3 m_d;
    Spectrum m_radiance;
    Spectrum m_throughput;
    Spectrum m_bsdfVal; // f * |cos|
    float m_woPdf;
};

class PathGuiderIntegrator : public Integrator {
public:
    PathGuiderIntegrator(
        const std::shared_ptr<Scene>& scene,
        const std::shared_ptr<Camera>& camera,
        const std::shared_ptr<Framebuffer>& buffer,
        const uint32_t maxBounce,
        const uint32_t initSpp,
        const uint32_t maxIteration)
        : Integrator(scene, camera, buffer), 
        m_maxBounce(maxBounce), m_initSpp(initSpp), m_maxIteration(maxIteration) {}

    Spectrum Li(Ray ray, Sampler& sampler);
    void Start();
    void Stop();
    void Wait();
    bool IsRendering();
    std::string ToString() const;
    // Debug
    void Debug(DebugRecord& debugRec);
private:
    Spectrum GuideSampleBSDF(GuideMaterialRecord& guideMatRec, Sampler& sampler) const;
    float GuidePdfBSDF(GuideMaterialRecord& guideMatRec) const;
    Spectrum GuideEvalPdfBSDF(GuideMaterialRecord& guideMatRec) const;

    void Setup();
    void Render();
    void RenderIteration(const uint32_t& spp, const uint32_t& iteration);
    void RenderTile(const Framebuffer::Tile& tile, const uint32_t& spp, const uint32_t& iteration);
    void SynchronizeThreads();
    // Misc
    float PowerHeuristic(float a, float b) const;
    // Debug
    void DebugRay(Ray ray, Sampler& sampler);
private:
    // Muti-thread setting
    bool m_rendering;
    std::atomic<int> m_renderingNum;
    std::vector<std::thread*> m_renderThreads;
    std::vector<Framebuffer::Tile> m_tiles;
    std::atomic<uint32_t> m_currentTileIndex;
    std::unique_ptr<std::thread> m_controlThread;

    // Options
    uint32_t m_maxBounce;
    uint32_t m_maxIteration;
    uint32_t m_initSpp;

    // State
    uint32_t m_currentSpp;
    uint32_t m_currentIteration;

    SDTree m_sdtree;
};