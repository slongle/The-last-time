#pragma once

#include "framebuffer.h"
#include "scene.h"
#include "camera.h"
#include "light/arealight.h"
#include "utility/timer.h"
#include "sampler/independent.h"

#include <thread>
#include <atomic>
#include <mutex>

class Integrator {
public:
	Integrator(
		const std::shared_ptr<Scene>& scene,
		const std::shared_ptr<Camera>& camera,
		const std::shared_ptr<Framebuffer>& buffer)
		: m_scene(scene), m_camera(camera), m_buffer(buffer) {}

	virtual void Setup() {
		m_camera->Setup(m_buffer->m_width, m_buffer->m_height);
		m_scene->Setup();
	}
	virtual Spectrum Li(Ray ray, Sampler& sampler) = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual void Wait() = 0;
	virtual bool IsRendering() = 0;
	virtual void Save();
	virtual std::string ToString() const = 0;
	// Debug
	virtual void Debug(DebugRecord& debugRec) {}
protected:
	float PowerHeuristic(float a, float b) const;
	// Debug
	void DrawPoint(const Float3& p, const Spectrum& c);
	void DrawLine(const Float3& p, const Float3& q, const Spectrum& c);
	void DrawBounds(const Bounds& bounds, const Spectrum& c);
public:
	Timer m_timer;
protected:
	std::shared_ptr<Framebuffer> m_buffer;
	std::shared_ptr<Scene> m_scene;
	std::shared_ptr<Camera> m_camera;
};

class SampleIntegrator : public Integrator {
public:
	SampleIntegrator(
		const std::shared_ptr<Scene>& scene,
		const std::shared_ptr<Camera>& camera,
		const std::shared_ptr<Framebuffer>& buffer,
		const uint32_t& spp)
		: Integrator(scene, camera, buffer), m_spp(spp) {}
	~SampleIntegrator();

	// Schedule
	virtual void Start();
	virtual void Stop();
	virtual void Wait();
	virtual bool IsRendering();
	virtual void RenderTile(const Framebuffer::Tile& tile, int spp, IndependentSampler& sampler);
	// Debug
	virtual Spectrum NormalCheck(Ray ray, Sampler& sampler);
protected:
	// Muti-thread setting
	std::atomic<bool> m_rendering;
	std::vector<Framebuffer::Tile> m_tiles;
	std::vector<IndependentSampler> m_samplers;
	std::thread* m_renderThread;

	// Options
	uint32_t m_spp;
	uint32_t m_accSpp = 0;
};
