#include "shader.h"

struct Shader {
    GLuint program;
};

bool shaderInit(Shader* shader, const char* vertex_source, const char* fragment_source) {

    *shader = malloc(sizeof(struct Shader));

    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_source, NULL);
    glCompileShader(vertex_shader);

    int success;
    char info_log[512];
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
        SDL_Log("ERROR compiling shader: %s\n", info_log);
    }

    unsigned int fragment_shader;
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
        SDL_Log("ERROR compiling shader: %s\n", info_log);
    }

    (*shader)->program = glCreateProgram();

    glAttachShader((*shader)->program, vertex_shader);
    glAttachShader((*shader)->program, fragment_shader);
    glLinkProgram((*shader)->program);

    glGetProgramiv((*shader)->program, GL_LINK_STATUS, &success);

    if (!success) {

        glGetProgramInfoLog((*shader)->program, 512, NULL, info_log);
        SDL_Log("ERROR compiling shader: %s\n", info_log);

    } else {
        SDL_Log("Shader compiled\n");
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

void shaderDestroy(Shader shader) {
    glDeleteProgram(shader->program);
    // free(shader);
}

void shaderUse(Shader shader) {
    glUseProgram(shader->program);
}