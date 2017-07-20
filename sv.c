#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define STB_IMAGE_IMPLEMENTATION
#include "std/stb_image.h"

int width = 1920;
int height = 1080;
GLFWwindow* window;
GLuint vertex;
GLuint fragment;
GLuint shaderProgram;

typedef struct Shad {
  GLuint id;
  char path[40];
  int new;
} Shader;

Shader shader;

typedef struct Img {
  GLuint id;
  char path[40];
  float ratio;
  int new;
} Image;

Image images[4];

typedef struct Param {
  char name[3];
  float value;
  int new;
} Parameter;

Parameter parameters[16];

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
  GLuint sh = glCreateShader(type);
  glShaderSource(sh, 1, &source, NULL);
  glCompileShader(sh);
  int success;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(sh, 512, NULL, infoLog);
    fputs("Shader compilation failed:\n",stderr);
    fputs(infoLog,stderr);
    exit(EXIT_FAILURE);
  }
  else { return sh; }
}

GLuint linkShader(GLuint vertex, GLuint fragment) {
  glUseProgram(0);
  glDeleteProgram(shader.id);
  shader.id = glCreateProgram();
  glAttachShader(shader.id, vertex);
  glAttachShader(shader.id, fragment);
  glLinkProgram(shader.id);
  int success;
  char infoLog[512];
  glGetProgramiv(shader.id, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader.id, 512, NULL, infoLog);
    fputs("Shader linking failed:\n",stderr);
    fputs(infoLog,stderr);
  }
  glDetachShader(shader.id, vertex); 
  glDetachShader(shader.id, fragment); 
  glDeleteShader(vertex);
  glDeleteShader(fragment);
}

void createWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  window = glfwCreateWindow(width,height, "", NULL, NULL);
  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_callback);
  glfwSetErrorCallback(error_callback);
  glewInit();
}

void createShader(char *vert, char *frag) {
  vertex = compileShader(readShader(vert), GL_VERTEX_SHADER);
  fragment = compileShader(readShader(frag),GL_FRAGMENT_SHADER);
  linkShader(vertex,fragment);
  printf("%i\n",shader.id);
  glUseProgram(shader.id);
};

void uniform1f(char * var, float f) {
  int v = glGetUniformLocation(shader.id, var);
  glUniform1f(v, f);
}

void uniform2f(char * var, float f0, float f1) {
  int v = glGetUniformLocation(shader.id, var);
  glUniform2f(v,f0,f1);
}

void imageUniforms(int i) {
  Image image = images[i];
  char name[2];
  sprintf(name,"i%i",i);
  glUniform1i(glGetUniformLocation(shader.id, name), i);
  char ratioName[7];
  sprintf(ratioName,"%sratio",name);
  uniform1f(ratioName,images[i].ratio);
}

void readImage(int i) {

  images[i].new = 1;

  int w,h,comp;
  stbi_set_flip_vertically_on_load(1);
  unsigned char* pixels = stbi_load(images[i].path, &w, &h, &comp, STBI_rgb_alpha);

  images[i].ratio = (float) w/h;
  imageUniforms(i);

  glActiveTexture(GL_TEXTURE0+i);
  glGenTextures(1, &images[i].id);
  glBindTexture(GL_TEXTURE_2D,images[i].id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  stbi_image_free(pixels);
}

void *readStdin() {

  while (1) {
    char * str;
    size_t l = 80;
    getline(&str,&l,stdin);
    char *n = strtok(str," ");

    if (n[0] == 'i') {
      int i = n[1]-'0';
      strncpy(images[i].path, strtok(NULL,"\n"),40);
      images[i].new = 1;
    }
    else if (n[0] == 'f') {
      strncpy(shader.path, strtok(NULL,"\n"),40);
      shader.new = 1;
    }
    else if (n[0] == 'p') {
      int i = n[1]-'0';
      strncpy(parameters[i].name,n,3);
      parameters[i].value = atof(strtok (NULL,"\n"));
      parameters[i].new = 1;
    }
    else if (n[0] == 'q') {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
  }
}

void updateImages(int force) {
  for (int i = 0; i<4; i++) {
    if (images[i].new || force) {
      readImage(i);
      printf("%s\n",images[i].path);
      images[i].new = 0;
    }
  }
}

void updateUniforms(int force) {
  for (int i = 0; i<16; i++) {
    if (parameters[i].new || force) {
      uniform1f(parameters[i].name,parameters[i].value);
      parameters[i].new = 0;
    }
  }
}

int main(int argc, char **argv) {

  createWindow();
  createShader("shader.vert","shader.frag");
  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
  for (int i = 0; i<4; i++) {
    strncpy(images[i].path, argv[i+1],40);
    images[i].new = 1;
  }
  uniform2f("resolution", width,height);
  
  pthread_t tid;
  pthread_create(&tid, NULL, readStdin, NULL);

  while (!glfwWindowShouldClose(window)) {
    uniform1f("time",glfwGetTime());
    if (shader.new) {
      createShader("shader.vert",shader.path);
      uniform2f("resolution", width,height); // important!!
      for (int i = 0; i<4; i++) { imageUniforms(i); }
      updateUniforms(1);
      shader.new = 0;
    }
    updateImages(0);
    updateUniforms(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  pthread_cancel(tid);
  glfwDestroyWindow(window);
  glfwTerminate();
 
  exit(EXIT_SUCCESS);
}
