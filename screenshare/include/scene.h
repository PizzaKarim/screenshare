#pragma once

void render_scene(float delta_time);

void scene_hover(GLFWwindow* window, double xpos, double ypos);
void scene_click(GLFWwindow* window, int button, int action, int mods);
void scene_key(GLFWwindow* window, int key, int scancode, int action, int mods);
void scene_resize(glm::vec2 size);
