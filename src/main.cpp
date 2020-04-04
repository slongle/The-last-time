#include "core/renderer.h"

int main(int argc, char* argv[]) {
	std::string filename = "E:/Document/Graphics/code/The-Last-time/scenes/Scenes/Box/scene.json";
	filename = "E:/Document/Graphics/code/The-Last-time/tools/scene.json";

	Renderer renderer(filename);
	renderer.Render();
	return 0;
}
