#include <pch.h>
#include <text.h>

#include <glad/glad.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <shader.h>
#include <shader/text.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static Shader SHADER;
static unsigned int VAO, VBO;

void init_font_renderer()
{
    ShaderSource shaders[2] = {
        compile_shader(ShaderType::VERTEX_SHADER, TEXT_VERTEX_SHADER),
        compile_shader(ShaderType::FRAGMENT_SHADER, TEXT_FRAGMENT_SHADER)
    };
    SHADER = link_shaders(2, shaders);
    glUseProgram(SHADER);

    resize_font_projection({ 1280, 720 });

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void destroy_font_renderer()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(SHADER);
}

void resize_font_projection(glm::vec2 size)
{
    glUseProgram(SHADER);
    glm::mat4 projection = glm::ortho(0.0f, size.x, 0.0f, size.y); // Has to be floats
    glUniformMatrix4fv(glGetUniformLocation(SHADER, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

Font load_font(const char* path, unsigned int height)
{
    Font font;

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        fprintf(stderr, "Could not initialize FreeType Library\n");
        exit(-1);
    }

    FT_Face face;
    if (FT_New_Face(ft, path, 0, &face)) {
        fprintf(stderr, "Failed to load font: %s\n", path);
        exit(-1);
    }
    else {
        FT_Set_Pixel_Sizes(face, 0, height);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (unsigned char c = 0; c < 128; c++)
        {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                fprintf(stderr, "Failed to load glyph\n");
                continue;
            }
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            font.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return font;
}

void render_text(Font* font, std::string text, glm::vec2 position, float scale, glm::vec3 color, glm::mat4 view)
{
    glUseProgram(SHADER);
    glUniformMatrix4fv(glGetUniformLocation(SHADER, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniform3f(glGetUniformLocation(SHADER, "text_color"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = (*font)[*c];

        float xpos = position.x + ch.bearing.x * scale;
        float ypos = position.y - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        glBindTexture(GL_TEXTURE_2D, ch.id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        position.x += (ch.advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Text preview_text(Font* font, const std::wstring& content, float scale, float max_length, const std::wstring& truncation)
{
    std::wstring string;
    if (!truncation.empty()) max_length -= preview_text(font, truncation, scale).size.x;
    float width = 0;
    float y_min = 0;
    glm::vec2 size{};
    for (auto c = content.begin(); c != content.end(); c++)
    {
        Character ch = (*font)[*c];
        if (max_length != 0 && size.x + (ch.advance >> 6) * scale > max_length) return preview_text(font, string + truncation, scale);
        size.x += (ch.advance >> 6) * scale;
        size.y = std::max(size.y, ch.size.y * scale);
        y_min = std::max(y_min, (ch.size.y - ch.bearing.y) * scale);
        string += *c;
    }
    return { font, string, size, scale, y_min };
}

void render_text(const Text* text, glm::vec2 position, glm::vec3 color, glm::mat4 view)
{
    glUseProgram(SHADER);
    glUniformMatrix4fv(glGetUniformLocation(SHADER, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniform3f(glGetUniformLocation(SHADER, "text_color"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    std::wstring::const_iterator c;
    for (c = text->content.begin(); c != text->content.end(); c++)
    {
        Character ch = (*text->font)[*c];

        float xpos = position.x + ch.bearing.x * text->scale;
        float ypos = position.y - (ch.size.y - ch.bearing.y) * text->scale;

        float w = ch.size.x * text->scale;
        float h = ch.size.y * text->scale;
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        glBindTexture(GL_TEXTURE_2D, ch.id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        position.x += (ch.advance >> 6) * text->scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
