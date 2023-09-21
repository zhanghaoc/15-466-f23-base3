#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint question_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > question_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("question.pnct"));
	question_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > question_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("question.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = question_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = question_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > cat_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("cat.wav"));
});
Load< Sound::Sample > insect_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("insect.wav"));
});
Load< Sound::Sample > crowd_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("crowd.wav"));
});
Load< Sound::Sample > construction_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("construction.wav"));
});

// this function is from my Graphics homework
bool PlayMode::hit(Ray ray, Boundingbox box) {
	glm::vec3 invdir;
	//taking some care so that we don't end up with NaN's , just a degenerate matrix, if scale is zero:
	invdir.x = (ray.dir.x == 0.0f ? 0.0f : 1.0f / ray.dir.x);
	invdir.y = (ray.dir.y == 0.0f ? 0.0f : 1.0f / ray.dir.y);
	invdir.z = (ray.dir.z == 0.0f ? 0.0f : 1.0f / ray.dir.z);

	float tmin, tmax, tymin, tymax, tzmin, tzmax;
	bool sign[3] = {invdir.x < 0, invdir.y < 0, invdir.z < 0};
	glm::vec3 bounds[2] = {box.min, box.max};

	tmin = (bounds[sign[0]].x - ray.point.x) * invdir.x;
	tmax = (bounds[1 - sign[0]].x - ray.point.x) * invdir.x;
	tymin = (bounds[sign[1]].y - ray.point.y) * invdir.y;
	tymax = (bounds[1 - sign[1]].y - ray.point.y) * invdir.y;

	if ((tmin > tymax) || (tymin > tmax)) return false;

	if (tymin > tmin)
		tmin = tymin;
	if (tymax < tmax)
		tmax = tymax;

	tzmin = (bounds[sign[2]].z - ray.point.z) * invdir.z;
	tzmax = (bounds[1 - sign[2]].z - ray.point.z) * invdir.z;

	if ((tmin > tzmax) || (tzmin > tmax)) return false;

	if (tzmin > tmin)
		tmin = tzmin;
	if (tzmax < tmax)
		tmax = tzmax;

	return true;
}
void PlayMode::createChoice(
		unsigned int id,
		unsigned int sound_index,
		bool is_selected,
		bool is_right,
		std::string choice_name,
		glm::vec3 position) {
	
	choices.emplace_back();
	auto &choice = choices.back();
	choice.id = id;
	choice.sound_index = sound_index;
	choice.is_selected = is_selected;
	choice.is_right = is_right;
	choice.choice_name = choice_name;

	// add random position
	scene.transforms.emplace_back();
	auto choice_transforms = &scene.transforms.back();
	choice.transform = choice_transforms;

	// choice
	Scene::Drawable new_choice_instance = choice_instance;
	new_choice_instance.transform = choice_transforms;
	scene.drawables.emplace_back(new_choice_instance);

	choice_transforms->name = "choice" + std::to_string(choice.id);
	choice_transforms->rotation = choice_instance.transform->rotation;
	choice_transforms->scale = choice_instance.transform->scale;
	choice_transforms->position = choice_instance.transform->position + position;
	
}


PlayMode::PlayMode() : scene(*question_scene) {
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	// get model from scene:
	
	for (auto drawable : scene.drawables) {
		if (drawable.transform->name == "choice") {
			choice_instance = drawable;
		}
		if (drawable.transform->name == "redChoice") {
			red_choice_instance = drawable;
		}
		if (drawable.transform->name == "greenChoice") {
			green_choice_instance = drawable;
		}
	}
	scene.drawables.clear();

	choice_mesh = question_meshes->lookup("choice");
	red_choice_mesh = question_meshes->lookup("redChoice");
	green_choice_mesh = question_meshes->lookup("greenChoice");
	

	// create multiple object, set their position and bounding box
	// for each choice, create red/green instance.
	createChoice(0, 0, false, true, "cat", glm::vec3(0.0f, 0.0f, 0.0f));
	createChoice(1, 1, false, true, "insect", glm::vec3(10.0f, 0.0f, 0.0f));
	createChoice(2, 2, false, true, "construction", glm::vec3(20.0f, 0.0f, 0.0f));
	createChoice(3, 3, false, true, "crowd", glm::vec3(30.0f, 10.0f, 0.0f));
	createChoice(4, 0, false, false, "bird", glm::vec3(0.0f, 10.0f, 0.0f));
	createChoice(5, 0, false, false, "wind", glm::vec3(10.0f, 10.0f, 0.0f));
	createChoice(6, 0, false, false, "train", glm::vec3(20.0f, 10.0f, 0.0f));
	createChoice(7, 0, false, false, "game", glm::vec3(30.0f, 0.0f, 0.0f));

	// create sounds and initialize right choices
	sounds.push_back(Sound::loop_3D(*cat_sample, 1.0f, glm::vec3(0.0f, 0.0f, 0.0f), 10.0f));
	sounds.push_back(Sound::loop_3D(*insect_sample, 1.0f, glm::vec3(0.0f, 0.0f, 0.0f), 10.0f));
	sounds.push_back(Sound::loop_3D(*construction_sample, 1.0f, glm::vec3(0.0f, 0.0f, 0.0f), 10.0f));
	sounds.push_back(Sound::loop_3D(*crowd_sample, 1.0f, glm::vec3(0.0f, 0.0f, 0.0f), 10.0f));


}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			click.downs += 1;
			click.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE) {
			click.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_MOUSEBUTTONDOWN) {
		if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	} else if (evt.type == SDL_MOUSEMOTION) {
		if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation
				* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
				* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			);
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//move camera:
	{

		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x =-1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y =-1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	{ // update click position for objects
		Ray ray;
		ray.point = camera->transform->position;
		ray.dir = camera->transform->rotation * glm::vec3(0.0f, 0.0f, -1.0f); // camera direction -z
	  	
		for (auto &choice : choices) {
			choice.boundingbox.min = choice.transform->position + choice.transform->scale * choice_mesh.min;
			choice.boundingbox.max = choice.transform->position + choice.transform->scale * choice_mesh.max;
			if (click.pressed && (!choice.is_selected) && hit(ray, choice.boundingbox)) {
				choice.is_selected = true;
				
				// append transform
				scene.transforms.emplace_back();
				// append a bigger drawable according to the choice for display
				Scene::Drawable new_choice_instance = red_choice_instance;
				if (choice.is_right) {
					new_choice_instance = green_choice_instance;
					count--;
				}
				new_choice_instance.transform = &scene.transforms.back();
				if (choice.is_right) {
					new_choice_instance.transform->rotation = green_choice_instance.transform->rotation;
					new_choice_instance.transform->position = choice.transform->position;
					new_choice_instance.transform->scale = 1.5f * green_choice_instance.transform->scale;
				} else {
					new_choice_instance.transform->rotation = red_choice_instance.transform->rotation;
					new_choice_instance.transform->position = choice.transform->position;
					new_choice_instance.transform->scale = 1.5f * red_choice_instance.transform->scale;
				}
				scene.drawables.emplace_back(new_choice_instance);

				if (choice.is_right)
					sounds[choice.sound_index]->stop();
			}		
		}
	}

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
	click.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse; Space to select",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse; Space to select",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		
		// draw center aim
		float aim_offset_x = 1.7f;
		float aim_offset_y = 1.0f;
		lines.draw_text("------",
			glm::vec3(-aspect + 0.1f * H + aim_offset_x, -1.0 + 0.1f * H + aim_offset_y, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0x00, 0x00, 0x00));
		lines.draw_text("|",
			glm::vec3(-aspect + 0.1f * H + aim_offset_x + 0.1f, -1.0 + 0.1f * H + aim_offset_y, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0x00, 0x00, 0x00));

		if (count == 0) {
			lines.draw_text("You Win!",
			glm::vec3(-aspect + 0.1f * H + aim_offset_x, -1.0 + 0.5f + 0.1f * H + aim_offset_y, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0x00, 0xff));
		}


		// draw text for choices 

		DrawLines draw_lines(camera->make_projection() * glm::mat4(camera->transform->make_world_to_local()));
		for (auto &choice : choices) {
			// draw text
			draw_lines.draw_text(choice.choice_name,
				choice.transform->position + glm::vec3(0.0f, 0.0f, 0.0f),
				glm::vec3(1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f),
				glm::u8vec4(0xff, 0x00, 0xff, 0xff)
			);
		}

	}
	GL_ERRORS();
}
