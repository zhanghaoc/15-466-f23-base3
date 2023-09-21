#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"
#include "Mesh.hpp"

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, click;

	struct Boundingbox {
		glm::vec3 min = glm::vec3(0.0f);
		glm::vec3 max = glm::vec3(0.0f);
	};

	struct Ray {
		glm::vec3 point = glm::vec3(0.0f);
		glm::vec3 dir = glm::vec3(0.0f);
	};

	struct choice {
		unsigned int id = 0;
		unsigned int sound_index = 0;
		bool is_selected = false;
		bool is_right = false;
		std::string choice_name;
		Boundingbox boundingbox;
		Scene::Transform * transform = nullptr;
	};
	
	bool hit(Ray, Boundingbox);
	void createChoice(
		unsigned int id = 0,
		unsigned int sound_index = 0,
		bool is_selected = false,
		bool is_right = false,
		std::string choice_name = "",
		glm::vec3 position = glm::vec3(0.0f));

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// choices instance
	std::vector<choice> choices;

	Scene::Drawable choice_instance;
	Scene::Drawable red_choice_instance;
	Scene::Drawable green_choice_instance;

	int count = 4;


	Mesh choice_mesh;
	Mesh red_choice_mesh;
	Mesh green_choice_mesh;

	//music coming from the tip of the leg (as a demonstration):
	std::vector<std::shared_ptr< Sound::PlayingSample >> sounds;

	//camera:
	Scene::Camera *camera = nullptr;

};
