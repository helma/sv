#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

static void error_callback(int error, const char* description) { fputs(description, stderr); }

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_Q && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
}

const char * readShader(char * path) {
  fputs(path,stderr);
  fputs("\n",stderr);
  FILE *file = fopen(path, "r");
  if (!file) {
    fputs("Cannot read ",stderr);
    fputs(path,stderr);
    exit(EXIT_FAILURE);
  }
  char source[2000];
  int res = fread(source,1,2000-1,file);
  source[res] = 0;
  fclose(file);
  const char *c_str = source;
  return c_str;
}

GLuint compileShader(const char * source, GLenum type) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  int success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    fputs("Shader compilation failed:\n",stderr);
    fputs(infoLog,stderr);
    exit(EXIT_FAILURE);
  }
  else { return shader; }
}

GLuint linkShader(GLuint vertex, GLuint fragment) {
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertex);
  glAttachShader(shaderProgram, fragment);
  glLinkProgram(shaderProgram);
  int success;
  char infoLog[512];
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
      glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
      fputs("Shader linking failed:\n",stderr);
      fputs(infoLog,stderr);
  }
  glDetachShader(shaderProgram, vertex); 
  glDetachShader(shaderProgram, fragment); 
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  glUseProgram(shaderProgram);
  return shaderProgram;
}

GLFWwindow* createWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  GLFWwindow* window = glfwCreateWindow(1920,1080, "", NULL, NULL);
  glfwMakeContextCurrent(window);
  //glfwSetKeyCallback(window, key_callback);
  glfwSetErrorCallback(error_callback);
  glewInit();
  return window;
}

GLuint createShader() {
  GLuint vertexShader = compileShader(readShader("shader.vert"), GL_VERTEX_SHADER);
  GLuint fragmentShader = compileShader(readShader("shader.frag"),GL_FRAGMENT_SHADER);
  GLuint shaderProgram = linkShader(vertexShader,fragmentShader);
  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  return shaderProgram;
};

void uniform1f(GLuint * shader, char * var, float f) {
  int v = glGetUniformLocation(*shader, var);
  glUniform1f(v, f);
}

void uniform2f(GLuint shader, char * var, float f0, float f1) {
  int v = glGetUniformLocation(shader, var);
  glUniform2f(v,f0,f1);
}

float color = 0.9;

void *readStdin(void * s) {
  GLuint *shader = s;
  while (1) {
    char * str;
    size_t l = 80;
    getline(&str,&l,stdin);
    char * variable = strtok (str," ");
    float value = atof(strtok (NULL,"\n"));
    printf("%.1f\n",value);
    color = value;
  }
}

int main(void) {

  GLFWwindow* window = createWindow();
  GLuint shader = createShader();
  uniform2f(shader, "resolution", 1920,1080);
  pthread_t tid;
  pthread_create(&tid, NULL, readStdin, &shader);

  while (!glfwWindowShouldClose(window)) {
    uniform1f(&shader, "time",glfwGetTime());
    uniform1f(&shader, "color",color);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);
    //glfwPollEvents();
  }

  //pthread_join(tid, NULL);
  //printf("After Thread\n");
  glfwDestroyWindow(window);
  glfwTerminate();
 
  exit(EXIT_SUCCESS);
}
