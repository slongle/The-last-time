#include "pathguider.h"
#include "light/environment.h"

void AddToAtomicFloat(std::atomic<float>& v1, float v2) {
    auto current = v1.load();
    while (!v1.compare_exchange_weak(current, current + v2));
}

Float3 DTree::Sample(Sampler& sampler) const
{    
    uint32_t idx = 0;
    while (!m_nodes[idx].IsLeaf()) {
        std::array<float, 4> cdf = GetChildrenCDF(idx);
        for (int i = 3; i >= 0; i--) {            
            if (sampler.Next1D() < cdf[i]) {
                float l = i == 0 ? 0 : cdf[i - 1], r = cdf[i];                
                idx = m_nodes[idx].m_children[i];
                break;
            }
        }
    }
    return CanonicalToDirection(sampler.Next2D());
}

float DTree::Pdf(const Float3& d) const
{
    Float2 s = DirectionToCanonical(d);
    uint32_t idx = 0;
    float pdf = 1.f;
    while (!m_nodes[idx].IsLeaf()) {
        std::array<float, 4> pdfs = GetChildrenPDF(idx);
        int tag0 = s.x < 0.5f;
        int tag1 = s.y < 0.5f;
        s.x = tag0 ? s.x * 2 : s.x * 2 - 1;
        s.y = tag1 ? s.y * 2 : s.y * 2 - 1;
        uint32_t childrenIdx = tag0 + tag1 * 2;
        idx = m_nodes[idx].m_children[childrenIdx];
        pdf *= pdfs[childrenIdx] * 4;
    }
    pdf *= INV_FOURPI;
    return pdf;
}

void DTree::AddSample(const Float3& d, const float& weight)
{
    Float2 s = DirectionToCanonical(d);
    uint32_t idx = 0;
    while (!m_nodes[idx].IsLeaf()) {
        AddToAtomicFloat(m_nodes[idx].m_weight, weight);
        int tag0 = s.x < 0.5f;
        int tag1 = s.y < 0.5f;
        s.x = tag0 ? s.x * 2 : s.x * 2 - 1;
        s.y = tag1 ? s.y * 2 : s.y * 2 - 1;
        uint32_t childrenIdx = tag0 + tag1 * 2;
        idx = m_nodes[idx].m_children[childrenIdx];
    }
    AddToAtomicFloat(m_nodes[idx].m_weight, weight);
}

std::array<float, 4> DTree::GetChildrenPDF(const uint32_t& idx) const
{
    std::array<float, 4> ret;
    float sum = 0;
    for (uint32_t i = 0; i < 4; i++) {
        ret[i] = m_nodes[m_nodes[idx].m_children[i]].m_weight;
        sum += ret[i];
    }
    for (uint32_t i = 0; i < 4; i++) {
        ret[i] /= sum;
    }
    return ret;
}

std::array<float, 4> DTree::GetChildrenCDF(const uint32_t& idx) const
{
    std::array<float, 4> ret;
    for (uint32_t i = 0; i < 4; i++) {
        ret[i] = m_nodes[m_nodes[idx].m_children[i]].m_weight;
        ret[i] += i == 0 ? 0 : ret[i - 1];
    }
    for (uint32_t i = 0; i < 4; i++) {
        ret[i] /= ret[3];
    }
    return ret;
}

void DTree::Subdivide(const float& threshold, const float& totalWeight)
{
    std::vector<DTreeNode> newNodes;
    newNodes.push_back(m_nodes[0]);
    newNodes[0].m_children = { -1,-1,-1,-1 };

    struct StackNode {
        uint32_t m_idx;
        uint32_t m_newIdx;
        uint32_t m_depth;
    };

    std::stack<StackNode> stack;
    stack.push(StackNode{ 0, 0, 1 });
    while (!stack.empty()) {
        StackNode stackNode = stack.top(); stack.pop();
        uint32_t idx = stackNode.m_idx;
        uint32_t newIdx = stackNode.m_newIdx;
        uint32_t depth = stackNode.m_depth;
        m_depth = std::max(m_depth, depth);
        DTreeNode node = m_nodes[idx];
        float weight = node.m_weight;
        if (weight < threshold * totalWeight) {
            // Stop
        }
        else {
            if (node.IsLeaf()) {
                for (uint32_t i = 0; i < 4; i++) {
                    newNodes[newIdx].m_children[i] = newNodes.size();
                    stack.push(StackNode{ uint32_t(m_nodes.size()), uint32_t(newNodes.size()), depth + 1 });
                    m_nodes.push_back(DTreeNode(weight * 0.25f));
                    newNodes.push_back(DTreeNode(weight * 0.25f));
                }
            }
            else {
                for (uint32_t i = 0; i < 4; i++) {
                    newNodes[newIdx].m_children[i] = newNodes.size();
                    stack.push(StackNode{ uint32_t(m_nodes.size()), uint32_t(newNodes.size()), depth + 1 });
                    m_nodes.push_back(DTreeNode(weight));
                    newNodes.push_back(DTreeNode(weight));
                }
            }
        }
    }
    m_nodes = newNodes;
}

void DTree::Clear()
{
    m_nodes.clear();
}

Float2 DTree::DirectionToCanonical(const Float3& d)
{
    float cosTheta = Frame::CosTheta(d);
    float phi = Frame::SphericalPhi(d);
    return Float2((cosTheta + 1) * 0.5f, phi / TWO_PI);
}

Float3 DTree::CanonicalToDirection(const Float2& p)
{
    float cosTheta = p.x * 2 - 1;
    float phi = p.y * TWO_PI;
    return Frame::SphericalToDirect(cosTheta, phi);
}

void DTreeWrapper::AddSample(const Float3& d, const float& w)
{
    m_sampleNum++;
    AddToAtomicFloat(m_weight, w);
    m_train.AddSample(d, w);
}

void DTreeWrapper::Refine(const float& threshold)
{
    m_train.Subdivide(threshold, m_weight);
    m_render = m_train;

    m_depth = m_train.GetDepth();
}

void DTreeWrapper::Clear()
{
    m_train.Clear();
    m_render.Clear();
}


DTreeWrapper* SDTree::GetDTreeWrapper(const Float3& p)
{
    uint32_t idx = 0;
    while (!m_nodes[idx].IsLeaf()) {        
        Float3 centroid = m_nodes[idx].m_bounds.Centroid();
        if (p[m_nodes[idx].m_axis] < centroid[m_nodes[idx].m_axis]) {
            idx = m_nodes[idx].m_children[0];
        }
        else {
            idx = m_nodes[idx].m_children[1];
        }
    }
    return &(m_nodes[idx].m_dtree);
}

void SDTree::RefineSTree(const uint32_t& limit)
{
    for (uint64_t i = 0; i < m_nodes.size(); i++) {
        if (m_nodes[i].IsLeaf()) {
            Subdivide(i, limit);            
        }
    }    
}

void SDTree::RefineDTree(const float& threshold)
{
    m_leafNum = 0;
    uint32_t sumDepth = 0;
    m_maxDTreeDepth = 0;
    for (uint64_t i = 0; i < m_nodes.size(); i++) {
        if (m_nodes[i].IsLeaf()) {
            m_nodes[i].m_dtree.Refine(threshold);
            m_leafNum++;
            uint32_t depth = m_nodes[i].m_dtree.GetDepth();
            sumDepth += depth;
            m_maxDTreeDepth = std::max(m_maxDTreeDepth, depth);
        }
    }
    m_averageDTreeDepth = sumDepth / m_leafNum;
}

void SDTree::Subdivide(const uint32_t& idx, const uint32_t& limit)
{
    if (m_nodes[idx].GetSampleNum() > limit) {
        // Divide bounds
        int axis = m_nodes[idx].m_axis = m_nodes[idx].m_bounds.MaxAxis();
        Bounds lBounds(m_nodes[idx].m_bounds), rBounds(m_nodes[idx].m_bounds);
        lBounds.m_pMax[axis] = rBounds.m_pMin[axis] = m_nodes[idx].m_bounds.Centroid()[axis];
        // Divide sample number
        uint32_t lNum = m_nodes[idx].GetSampleNum() * 0.5f, rNum = m_nodes[idx].GetSampleNum() - lNum;
        // Construct new children
        m_nodes[idx].m_children[0] = m_nodes.size();
        m_nodes.push_back(SDTreeNode(lBounds, m_nodes[idx].m_dtree, lNum));
        m_nodes[idx].m_children[1] = m_nodes.size();
        m_nodes.push_back(SDTreeNode(rBounds, m_nodes[idx].m_dtree, rNum));
        // Clear DTree
        m_nodes[idx].m_dtree.Clear();
    }
}

void Vertex::Commit()
{    
    Spectrum weight = m_radiance / (m_throughput * m_woPdf) * 0.5f;
    m_dtree->AddSample(m_d, weight.y());
}

Spectrum PathGuiderIntegrator::Li(Ray ray, Sampler& sampler)
{
    Spectrum radiance(0.f);
    Spectrum throughput(1.f);
    float eta = 1.f;
    HitRecord hitRec;
    std::vector<Vertex> vertices;
    bool hit = m_scene->Intersect(ray, hitRec);
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        if (!hit) {
            // Environment Light
            if (bounce == 0) {
                for (const auto& envLight : m_scene->m_environmentLights) {
                    radiance += throughput * envLight->Eval(ray);
                }
            }
            break;
        }

        if (bounce == 0 && hitRec.m_primitive->IsAreaLight()) {
            LightRecord lightRec(ray.o, hitRec.m_geoRec);
            radiance += throughput * hitRec.m_primitive->m_areaLight->Eval(lightRec);
        }

        
        DTreeWrapper* dtree = m_sdtree.GetDTreeWrapper(hitRec.m_geoRec.m_p);        
        auto& bsdf = hitRec.m_primitive->m_bsdf;
        bool isDelta = bsdf->IsDelta(hitRec.m_geoRec.m_st);

        if (!isDelta) {
            LightRecord lightRec(hitRec.m_geoRec.m_p);
            Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D(), sampler);
            if (!emission.IsBlack()) {
                /* Allocate a record for querying the BSDF */
                MaterialRecord matRec(-ray.d, lightRec.m_wi, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
                GuideMaterialRecord guideMatRec(matRec, bsdf, dtree);

                /* Evaluate BSDF * cos(theta) and pdf */
                //Spectrum bsdfVal = bsdf->EvalPdf(matRec);
                Spectrum bsdfVal = GuideEvalPdfBSDF(guideMatRec);

                if (!bsdfVal.IsBlack()) {
                    /* Weight using the power heuristic */
                    float weight = PowerHeuristic(lightRec.m_pdf, guideMatRec.m_woPdf);
                    Spectrum L = throughput * bsdfVal * emission * weight;

                    radiance += L;
                    for (auto& vertex : vertices) {
                        vertex.m_radiance += L;
                    }

                    Vertex{ dtree, lightRec.m_wi, L, throughput * bsdfVal / lightRec.m_pdf, 
                        bsdfVal, lightRec.m_pdf }.Commit();
                }
            }
        }

        // Sample BSDF
        {
            // Sample BSDF * |cos| / pdf
            MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
            GuideMaterialRecord guideMatRec(matRec, bsdf, dtree);
            //Spectrum bsdfVal = bsdf->Sample(matRec, sampler.Next2D());
            Spectrum bsdfVal = GuideSampleBSDF(guideMatRec, sampler);
            ray = Ray(hitRec.m_geoRec.m_p, guideMatRec.ToWorld(guideMatRec.m_wo));
            if (bsdfVal.IsBlack()) {
                break;
            }

            // Update throughput
            throughput *= bsdfVal;
            eta *= guideMatRec.m_eta;

            // Evaluate light
            LightRecord lightRec;
            Spectrum emission;
            hit = m_scene->Intersect(ray, hitRec);
            if (hit) {
                if (hitRec.m_primitive->IsAreaLight()) {
                    auto areaLight = hitRec.m_primitive->m_areaLight;
                    lightRec = LightRecord(ray.o, hitRec.m_geoRec);
                    emission = areaLight->EvalPdf(lightRec);
                    lightRec.m_pdf /= m_scene->m_lights.size();
                }
            }
            else {
                // Environment Light
                for (const auto& envLight : m_scene->m_environmentLights) {
                    lightRec = LightRecord(ray);
                    emission = envLight->EvalPdf(lightRec);
                    lightRec.m_pdf /= m_scene->m_lights.size();
                }
            }

            if (!emission.IsBlack()) {
                // Heuristic
                float weight = isDelta ? 1.f : PowerHeuristic(guideMatRec.m_woPdf, lightRec.m_pdf);
                Spectrum L = throughput * emission * weight;
                               
                radiance += L;
                for (auto& vertex : vertices) {
                    vertex.m_radiance += L;
                }

                if (!isDelta) {                    
                    vertices.push_back(Vertex{ dtree, ray.d, Spectrum(0.f), 
                        throughput, bsdfVal * guideMatRec.m_woPdf, guideMatRec.m_woPdf });
                }
            }
        }

        if (bounce > 5) {
            float q = std::min(0.99f, MaxComponent(throughput * eta * eta));
            if (sampler.Next1D() > q) {
                break;
            }
            throughput /= q;
        }
    }
    for (auto& vertex : vertices) {
        vertex.Commit();
    }

    return radiance;
}

Spectrum PathGuiderIntegrator::GuideSampleBSDF(GuideMaterialRecord& guideMatRec, Sampler& sampler) const
{
    Spectrum bsdfVal;
    Float2 s = sampler.Next2D();
    if (m_currentIteration == 0 || guideMatRec.m_bsdf->IsDelta(guideMatRec.m_st)) {
        bsdfVal = guideMatRec.m_bsdf->Sample(guideMatRec, s);
        guideMatRec.m_dtreePdf = 0.f;
        guideMatRec.m_woPdf = guideMatRec.m_bsdfPdf = guideMatRec.m_pdf;
    }
    else {
        if (s.x < 0.5f) {
            // Sample BSDF
            bsdfVal = guideMatRec.m_bsdf->Sample(guideMatRec, sampler.Next2D());
            if (bsdfVal.IsBlack()) {
                guideMatRec.m_woPdf = guideMatRec.m_bsdfPdf = guideMatRec.m_dtreePdf = 0.f;
                return Spectrum(0.f);
            }

            if (guideMatRec.m_bsdf->IsDelta(guideMatRec.m_st)) {
                guideMatRec.m_dtreePdf = 0.f;
                guideMatRec.m_woPdf = guideMatRec.m_bsdfPdf * 0.5f;
                return bsdfVal * 2.f;
            }

            bsdfVal *= guideMatRec.m_pdf;
        }
        else {
            // Sample DTree
            guideMatRec.m_wo = guideMatRec.ToLocal(guideMatRec.m_dtree->Sample(sampler));
            bsdfVal = guideMatRec.m_bsdf->Eval(guideMatRec);
        }

        if (bsdfVal.IsBlack()) {
            return Spectrum(0.f);
        }

        float woPdf = GuidePdfBSDF(guideMatRec);
        if (woPdf == 0.f) {
            return Spectrum(0.f);
        }

        bsdfVal /= woPdf;
    }
    return bsdfVal;
}

float PathGuiderIntegrator::GuidePdfBSDF(GuideMaterialRecord& guideMatRec) const
{
    guideMatRec.m_bsdf->Pdf(guideMatRec);
    guideMatRec.m_bsdfPdf = guideMatRec.m_pdf;
    if (m_currentIteration == 0 || guideMatRec.m_bsdf->IsDelta(guideMatRec.m_st)) {
        guideMatRec.m_dtreePdf = 0.f;
        guideMatRec.m_woPdf = guideMatRec.m_bsdfPdf;
    }
    else {
        guideMatRec.m_dtreePdf = guideMatRec.m_dtree->Pdf(guideMatRec.ToWorld(guideMatRec.m_wo));
        guideMatRec.m_woPdf = guideMatRec.m_bsdfPdf * 0.5f + guideMatRec.m_dtreePdf * 0.5f;
    }
    return guideMatRec.m_woPdf;
}

Spectrum PathGuiderIntegrator::GuideEvalPdfBSDF(GuideMaterialRecord& guideMatRec) const
{
    Spectrum bsdfVal = guideMatRec.m_bsdf->Eval(guideMatRec);
    GuidePdfBSDF(guideMatRec);
    return bsdfVal;
}

void PathGuiderIntegrator::Setup()
{
    Integrator::Setup();
}

void PathGuiderIntegrator::Start()
{
    Setup();

    m_tiles.clear();
    for (int j = 0; j < m_buffers[0]->m_height; j += tile_size) {
        for (int i = 0; i < m_buffers[0]->m_width; i += tile_size) {
            m_tiles.push_back({
                {i, j},
                {std::min(m_buffers[0]->m_width - i, tile_size), std::min(m_buffers[0]->m_height - j, tile_size)}
                });
        }
    }
    std::reverse(m_tiles.begin(), m_tiles.end());

    m_renderingNum = 0;
    m_rendering = true;

    m_currentSpp = 0;
    m_currentIteration = 0;

    m_renderThreads.resize(std::thread::hardware_concurrency());
    //m_renderThreads.resize(1);
    m_timer.Start();
    m_controlThread = std::make_unique<std::thread>(&PathGuiderIntegrator::Render, this);
}

void PathGuiderIntegrator::Stop()
{
    m_rendering = false;
}

void PathGuiderIntegrator::Wait()
{
    if (m_controlThread->joinable()) {
        m_controlThread->join();
    }
    assert(m_renderingNum == 0);
}

bool PathGuiderIntegrator::IsRendering()
{
    return m_rendering;
}

std::string PathGuiderIntegrator::ToString() const
{
    return fmt::format("Path Guider\niteration : {0}\nspp : {1}\n{2}", 
        m_currentIteration, m_currentSpp, m_sdtree.ToString());
}

void PathGuiderIntegrator::Render()
{
    m_sdtree = SDTree(m_scene->m_bounds);
    uint32_t spp = m_initSpp;
    for (m_currentIteration = 0; m_currentIteration < m_maxIteration && m_rendering; m_currentIteration++) {
        m_currentTileIndex = 0;
        for (std::thread*& thread : m_renderThreads) {
            thread = new std::thread(&PathGuiderIntegrator::RenderIteration, this, spp, m_currentIteration);
        }
        SynchronizeThreads();

        m_sdtree.RefineSTree(12000 * std::pow(2, 0.5 * (m_currentIteration + 1)));
        m_sdtree.RefineDTree(0.01f);
        if (m_currentIteration != m_maxIteration - 1) {
            m_buffers[0]->Save(std::to_string(m_currentSpp + spp));
            m_buffers[0]->Initialize();
        }

        m_currentSpp += spp;
        spp *= 2;
    }
    m_rendering = false;
    m_timer.Stop();
    m_buffers[0]->Save(std::to_string(m_currentSpp));
}

void PathGuiderIntegrator::RenderIteration(const uint32_t& spp, const uint32_t& iteration)
{
    ++m_renderingNum;

    while (m_rendering) {
        Framebuffer::Tile tile;
        if (m_currentTileIndex < m_tiles.size()) {
            tile = m_tiles[m_currentTileIndex++];
        }
        else {
            break;
        }

        if (m_rendering) {
            RenderTile(tile, spp, iteration);
        }
    }

    m_renderingNum--;
    if (m_renderingNum == 0) {
        
    }
}

void PathGuiderIntegrator::RenderTile(
    const Framebuffer::Tile& tile, 
    const uint32_t& spp, 
    const uint32_t& iteration)
{
    Sampler sampler;
    uint64_t s = (tile.pos[1] * m_buffers[0]->m_width + tile.pos[0]) + 
        iteration * (m_buffers[0]->m_width * m_buffers[0]->m_height);
    sampler.Setup(s);

    for (int j = 0; j < tile.res[1]; j++) {
        for (int i = 0; i < tile.res[0]; i++) {
            for (uint32_t k = 0; k < spp; k++) {
                int x = i + tile.pos[0], y = j + tile.pos[1];
                Ray ray;
                m_camera->GenerateRay(Float2(x, y), sampler, ray);
                Spectrum radiance = Li(ray, sampler);
                m_buffers[0]->AddSample(x, y, radiance);                
            }
        }
    }
}

void PathGuiderIntegrator::SynchronizeThreads()
{
    for (std::thread*& thread : m_renderThreads) {
        thread->join();
        delete thread;
    }
    assert(m_renderingNum == 0);
}

void PathGuiderIntegrator::Debug(DebugRecord& debugRec)
{
    if (debugRec.m_debugRay) {
        Float2 pos = debugRec.m_rasterPosition;
        if (pos.x >= 0 && pos.x < m_buffers[0]->m_width &&
            pos.y >= 0 && pos.y < m_buffers[0]->m_height)
        {
            int x = pos.x, y = m_buffers[0]->m_height - pos.y;
            Sampler sampler;
            unsigned int s = y * m_buffers[0]->m_width + x;
            sampler.Setup(s);
            Ray ray;
            m_camera->GenerateRay(Float2(x, y), sampler, ray);
            DebugRay(ray, sampler);
        }
    }
    if (debugRec.m_debugKDTree) {
        //DrawBounds(m_guider.m_train.m_nodes[0].m_bounds, Spectrum(1, 0, 0));
        //return;

        struct StackNode {
            uint32_t idx;
            uint32_t depth;
        };

        std::stack<StackNode> stack;
        const std::vector<SDTreeNode>& nodes = m_sdtree.m_nodes;
        stack.push(StackNode{ 0,0 });
        while (!stack.empty()) {
            StackNode snode = stack.top(); stack.pop();
            uint32_t idx = snode.idx, depth = snode.depth;
            const SDTreeNode& node = nodes[idx];
            if (node.IsLeaf()) {
                DrawBounds(node.m_bounds, Spectrum(1, 0, 0));
            }
            else {
                stack.push(StackNode{uint32_t(node.m_children[0]), depth + 1});
                stack.push(StackNode{uint32_t(node.m_children[1]), depth + 1});
            }
        }
    }
}

void PathGuiderIntegrator::DebugRay(Ray ray, Sampler& sampler)
{
    Spectrum throughput(1.f);
    HitRecord hitRec;
    bool hit = m_scene->Intersect(ray, hitRec);
    for (uint32_t bounce = 0; bounce < m_maxBounce; bounce++) {
        if (!hit) {
            break;
        }

        auto& bsdf = hitRec.m_primitive->m_bsdf;

        if (!bsdf->IsDelta(hitRec.m_geoRec.m_st)) {
            LightRecord lightRec(hitRec.m_geoRec.m_p);
            Spectrum emission = m_scene->SampleLight(lightRec, sampler.Next2D(), sampler);
            if (!emission.IsBlack()) {
                /* Allocate a record for querying the BSDF */
                MaterialRecord matRec(-ray.d, lightRec.m_wi, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);

                /* Evaluate BSDF * cos(theta) and pdf */
                Spectrum bsdfVal = bsdf->EvalPdf(matRec);

                if (!bsdfVal.IsBlack()) {
                    /* Weight using the power heuristic */
                    float weight = PowerHeuristic(lightRec.m_pdf, matRec.m_pdf);
                    Spectrum value = throughput * bsdfVal * emission * weight;

                    Float3 p = hitRec.m_geoRec.m_p;
                    Float3 q = lightRec.m_geoRec.m_p;
                    DrawLine(p, q, Spectrum(1, 1, 0));
                }
            }
        }

        // Sample BSDF
        {
            // Sample BSDF * |cos| / pdf
            MaterialRecord matRec(-ray.d, hitRec.m_geoRec.m_ns, hitRec.m_geoRec.m_st);
            Spectrum bsdfVal = bsdf->Sample(matRec, sampler.Next2D());
            ray = Ray(hitRec.m_geoRec.m_p, matRec.ToWorld(matRec.m_wo));
            if (bsdfVal.IsBlack()) {
                break;
            }

            // Update throughput
            throughput *= bsdfVal;

            LightRecord lightRec;
            Spectrum emission;
            hit = m_scene->Intersect(ray, hitRec);
            if (hit) {
                if (hitRec.m_primitive->IsAreaLight()) {
                    auto areaLight = hitRec.m_primitive->m_areaLight;
                    lightRec = LightRecord(ray.o, hitRec.m_geoRec);
                    emission = areaLight->EvalPdf(lightRec);
                    lightRec.m_pdf /= m_scene->m_lights.size();
                }
                Float3 p = ray.o;
                Float3 q = hitRec.m_geoRec.m_p;
                DrawLine(p, q, Spectrum(1, 1, 1));
            }
            else {
                // Environment Light
                if (m_scene->m_environmentLights.empty()) {
                    Float3 p = ray.o;
                    Float3 q = ray.o + ray.d * 100.f;
                    DrawLine(p, q, Spectrum(1, 0, 0));
                }
                else {
                    break;
                }
            }

            if (!emission.IsBlack()) {
                // Heuristic
                float weight = bsdf->IsDelta(hitRec.m_geoRec.m_st) ?
                    1.f : PowerHeuristic(matRec.m_pdf, lightRec.m_pdf);
                Spectrum contribution = throughput * emission * weight;
            }
        }

        if (bounce > 5) {
            float q = std::min(0.99f, MaxComponent(throughput));
            if (sampler.Next1D() > q) {
                break;
            }
            throughput /= q;
        }
    }
}

float PathGuiderIntegrator::PowerHeuristic(float a, float b) const
{
    a *= a;
    b *= b;
    return a / (a + b);
}

