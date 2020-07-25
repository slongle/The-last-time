#include "core/renderer.h"
#include "bsdf/microfacet.h"

int main(int argc, char* argv[]) {
	google::InitGoogleLogging("Render");
	google::InstallFailureSignalHandler();

	std::string prefix = "E:/Document/Graphics/code/The-Last-time/scenes/Scenes/";
	std::vector<std::string> scenes(100);
	scenes[0] = "Box/Diffuse/scene.json";
	scenes[1] = "Box/Specular/scene.json";
	scenes[2] = "1.Texture/Globe/scene.json";
    scenes[3] = "1.Texture/Pbrt-book/scene.json";
	scenes[4] = "1.Texture/Leaf/scene.json";
	scenes[5] = "2.Shader/Shader-ball/scene.json";
	scenes[6] = "3.Volume/Homogeneous/scene.json";
	scenes[7] = "3.Volume/Equiangular/scene.json";
	scenes[8] = "3.Volume/Glory/scene.json";
	scenes[9] = "5.Guide/Torus/scene.json";
	scenes[10] = "6.Caustics/DiscoBall/scene.json";
	scenes[11] = "6.Caustics/Water/scene.json";
	std::string filename = prefix + scenes[5];

	Renderer renderer(filename);
	renderer.Render();
	return 0;
}
