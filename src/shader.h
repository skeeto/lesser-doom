// GLEW must come before OpenGL
#include <GL/glew.h>
#include <SDL.h>

typedef struct Shader* Shader;

bool shaderInit(Shader* shader, const char* vertex_source, const char* fragment_source);
void shaderDestroy(Shader shader);
void shaderUse(Shader shader);