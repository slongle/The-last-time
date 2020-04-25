#include "core/renderer.h"
#include "integrator/pathguider.h"

int main(int argc, char* argv[]) {
	std::cout << sizeof(SDTreeNode) << std::endl;

	std::string prefix = "E:/Document/Graphics/code/The-Last-time/scenes/Scenes/";
	std::vector<std::string> scenes(100);
	scenes[0] = "Box/scene.json";
	scenes[1] = "1.Texture/Globe/scene.json";
    scenes[2] = "1.Texture/Pbrt-book/scene.json";
	scenes[3] = "1.Texture/Leaf/scene.json";
	scenes[4] = "2.Shader/Shader-ball/scene.json";
	scenes[5] = "3.Volume/Homogeneous/scene.json";
	scenes[6] = "5.Guide/Torus/scene.json";
	std::string filename = prefix + scenes[6];

	Renderer renderer(filename);
	renderer.Render();
	return 0;
}
