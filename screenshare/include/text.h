#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <string>

struct Character
{
    unsigned int id;
    glm::ivec2   size;
    glm::ivec2   bearing;
    unsigned int advance;
};

typedef std::unordered_map<wchar_t, Character> Font;

struct Text
{
    Font* font;
    std::wstring content;
    glm::vec2 size;
    float scale;
    float y_min;
};

void init_font_renderer();
void destroy_font_renderer();
void resize_font_projection(glm::vec2 size);
Font load_font(const char* path, unsigned int height = 48);
void render_text(Font* font, std::string text, glm::vec2 position, float scale, glm::vec3 color, glm::mat4 view);

Text preview_text(Font* font, const std::wstring& content, float scale = 1.0f, float max_length = 0.0f, const std::wstring& truncation = L"");
void render_text(const Text* text, glm::vec2 position, glm::vec3 color, glm::mat4 view);
