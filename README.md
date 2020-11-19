# The-last-time
![C/C++ CI](https://github.com/slongle/The-last-time/workflows/C/C++%20CI/badge.svg?branch=master)  
![](images/cover.png)

## Building

### External libraries
I recommend you use some package manager like `vcpkg`.  
```bash
 > set VCPKG_DEFAULT_TRIPLET=x64-windows  
 > vcpkg install embree3 openvdb tinyobjloader imgui glad glfw3 glew glm fmt glog nlohmann-json stb openimageio   
 > vcpkg integrate install 
```
### Build
Now I just test building it on Windows.   
Please build as x64 platform like
```bash
 > mkdir build
 > cd build
 > cmake -G "Visual Studio 16 2019" ..
```
Or use `./setup_builds.bat`. 

## Project
[Project Website](https://slongle.github.io/projects/The-Last-Time)

### Overview
[This Offline Renderer](https://github.com/slongle/The-Last-time) is a physically-based photorealistic 3D renderer I am writing from scratch in C++. Rainbow is built from the ground up as a global illumination renderer supporting global illumination through light transport algorithms, including volumetric unidirectional pathtracing with multiple importance sampling. The renderer is still a work in progress.  

### Features
- Advanced Global illumination 
  - Volumetric unidirectional path tracing   
  - Volumetric probabilistic progressive photon mapping(Point estimation and Beam estimation)
  - Primary sample space metroplis light transport (Kelemen-style MLT)(WIP)
  - Path Guider (WIP)
- Geometry 
  - Triangle and quad meshes
- Lights 
  - Point light
  - Spot light
  - Directional light
  - Area light
- Materials and BSDFs
  - Thin-film iridescence
  - Rough conductor and dielectric using microfacet models (GGX)
  - Smooth conductor and dielectric
  - Transparent BSDF
  - Blend BSDF
  - Support alpha texture
- Media
  - Homogeneous medium
  - Heterogeneous medium (OpenVDB file)  
- Acceleration Structure System 
  - SAH-BVH acceleration
  - Embree3
- Render Mode 
  - Adaptive mode
  - Progressive mode
  - Final mode  

### Select Images
![](images/ir_eta2_1.33_eta3_1_k3_0_d_300-450.png)  
![](images/ir_eta2_1.33_eta3_1_k3_0_d_500-650.png)  
![](images/ir_eta2_1.33_eta3_1_k3_0_d_700-850.png)  
Iridescence effect, using the method from Belcour and Barla 2017 (eta_1 = 1.0, eta_2 = 1.33, eta_3 = 1.0, k_3 = 0, d = 300-850nm)   
![](images/ir4.gif)  
Goniochromism when changing view position, eta_1 = 1.0, eta_2 = 1.33, eta_3 = 1.0, k_3 = 0, d = 550nm, alpha = 0.1    
![](images/glory_4096spp.png)  
Crepuscular beam  
![](images/envbunny_spp=512_density=10_albedo=1.png)  
Homogeneous medium with HG phase function (density = 10, albedo = 1, g = 0), multi-scatter  
![](images/cloud_depth=100_1024spp_9h1m17s.png)  
Disney cloud  
![](images/smoke_1024spp_2h31m29s.png)  
Smoke(VDB file from [OpenVDB](https://www.openvdb.org/download/))   
![](images/fire_128spp.gif)  
Smoke(VDB file from [JangaFX](https://jangafx.com/software/embergen/download/free-vdb-animations/))   
![](images/vppm_0.5Kiteration_10.0M_0.005_0.9.png)  
Initial radius = 0.005, alpha = 0.9, # of delta photon = 1M, iteration = 0.5K     
![](images/scene_SPPM_5000.png)  
Initial radius = 0.2, alpha = 0.5, iteration = 5000  
![](images/torus_PT_1024000spp_11h59m46s720ms.png)  
![](images/torus_PM_0.08_0.5_1mX1k_8m48s.png)  
The first image is from path tracing, 1024K spp, 11h59m46s   
The second image is from SPPM, initial radius = 0.08, alpha = 0.5, # of delta photon = 1M, iteration = 1K, 8m48s   
![](images/scene_1024_1kiiteration_100kdelta.png)
Initial radius = 0.05, alpha = 0.3, iteration = 1000, # of delta photon = 100k  
![](images/globe_64spp.png)  
Blend BSDF of smooth dielectric and matte, Au conductor BSDF (convert SPD to RGB)  
![](images/scene_1024spp.png)  
Alpha texture for leaf  
![](images/pbrt-book_64spp.png) 
![](images/scene_RoughConductor_0.1_0.1_512spp.png)   
Rough conductor, alpha_u = 0.1, alpha_v = 0.1  
![](images/scene_RoughConductor_0.005_0.1_512spp.png)   
Rough conductor, alpha_u = 0.005, alpha_v = 0.1  
![](images/scene_RoughDielectric_0.1_0.1_512spp.png)   
Rough dielectric, alpha_u = 0.1, alpha_v = 0.1  
![](images/scene_RoughDielectric_0.005_0.1_512spp.png)   
Rough dielectric, alpha_u = 0.005, alpha_v = 0.1  

The images below are from my old renderer [Rainbow](https://github.com/slongle/Rainbow).  
![](images/cornell-box-heat.jpg)  
![](images/cornell-box-sphere-heat.png)  
![](images/veach-mis-heat.jpg)  
Veach thesis multiple importance sampling test scene. Rendered using pathtracing with multiple importance sampling.  
![](images/vol-caustic-final.png)  
Homogeneous medium(volumetric caustic WIP)  
![](images/hetvol.jpg)  
Heterogeneous medium  
Rendered using volumetric unidirectional pathtracing.  
