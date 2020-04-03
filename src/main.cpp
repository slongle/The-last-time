#include "core/renderer.h"

int main(int argc, char* argv[]) {
	std::string filename = "E:/Document/Graphics/code/The-Last-time/scenes/Scenes/Box/scene.json";

	Renderer renderer(filename);
	renderer.Render();
	return 0;
}
