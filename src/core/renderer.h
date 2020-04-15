#pragma once
    
#include "framebuffer.h"
#include "scene.h"
#include "camera.h"
#include "light/arealight.h"
#include "utility/timer.h"
#include "integrator.h"

#include <vector>
#include <thread>
#include <algorithm>
#include <iostream>
#include <atomic>
#include <mutex>

struct GLFWwindow;

class Renderer {
public:
    Renderer(const std::string& filename);
    void Render();
private:
    void InitializeGUI();    
    void Draw();

public:
    std::shared_ptr<Framebuffer> m_buffer;
    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<Integrator> m_integrator;
private:
    // GUI
    GLFWwindow* m_window;
};