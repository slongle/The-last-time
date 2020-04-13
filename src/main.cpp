#include "core/renderer.h"

#include "core/sampler.h"
#include "core/sampling.h"

void foo() {
	/*
	std::vector<float> x{ 1,2,3,4};
	Distribution1D distribution(&x[0], 4);
	float pdf, idx, s;
	while (std::cin >> s) {		
		idx = distribution.Sample(s, pdf);
		std::cout << idx << ' ' << pdf << std::endl;
	}
	*/
}

int main(int argc, char* argv[]) {
	std::string prefix = "E:/Document/Graphics/code/The-Last-time/scenes/Scenes/";
	std::vector<std::string> scenes(100);
	scenes[0] = "Box/scene.json";
	scenes[1] = "1.Texture/Globe/scene.json";
    scenes[2] = "1.Texture/Pbrt-book/scene.json";
	scenes[3] = "2.Shader/Shader-ball/scene.json";
	std::string filename = prefix + scenes[3];

	Renderer renderer(filename);
	renderer.Render();
	return 0;
}
