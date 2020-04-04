#include "core/renderer.h"

int main(int argc, char* argv[]) {
	std::string prefix = "E:/Document/Graphics/code/The-Last-time/scenes/Scenes/";
	std::vector<std::string> scenes(100);
	scenes[0] = "Box/scene.json";
	scenes[1] = "1.Texture/Globe/scene.json";
    scenes[2] = "1.Texture/Pbrt-book/scene.json";
	std::string filename = prefix + scenes[2];

	Renderer renderer(filename);
	renderer.Render();
	return 0;
}
