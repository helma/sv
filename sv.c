#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define STB_IMAGE_IMPLEMENTATION
#include "std/stb_image.h"
#include "uthash/src/utstring.h"
#include "uthash/src/uthash.h"

int width = 1920;
int height = 1080;
GLFWwindow* window;
GLuint vertex;
GLuint fragment;
GLuint shader;
GLuint textures[4];

struct param {
  char name[10];
  float value;
  UT_hash_handle hh;         /* makes this structure hashable */
};
struct param *parameters  = NULL;

struct image {
  int id;
  char path[40];
  UT_hash_handle hh;         /* makes this structure hashable */
};
struct image *images  = NULL;

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
  glDeleteProgram(shader);
  shader = glCreateProgram();
  glAttachShader(shader, vertex);
  glAttachShader(shader, fragment);
  glLinkProgram(shader);
  int success;
  char infoLog[512];
  glGetProgramiv(shader, GL_LINK_STATUS, &success);
  if (!success) {
      glGetProgramInfoLog(shader, 512, NULL, infoLog);
      fputs("Shader linking failed:\n",stderr);
      fputs(infoLog,stderr);
  }
  glDetachShader(shader, vertex); 
  glDetachShader(shader, fragment); 
  glDeleteShader(vertex);
  glDeleteShader(fragment);
  glUseProgram(shader);
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
  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);
};

void uniform1f(char * var, float f) {
  int v = glGetUniformLocation(shader, var);
  glUniform1f(v, f);
}

void uniform2f(char * var, float f0, float f1) {
  int v = glGetUniformLocation(shader, var);
  glUniform2f(v,f0,f1);
}

void readImage(char *file, int i) {

  glUseProgram(shader);
  char name[4];
  sprintf(name,"img%i",i);
  glUniform1i(glGetUniformLocation(shader, name), i);
  //glBindTexture(GL_TEXTURE_2D, 0);
  //glDeleteTextures(1,&textures[i]);
  //GLuint tex;
  //glGenTextures(1, &tex);
  //textures[i] = tex;
  glActiveTexture(GL_TEXTURE0+i);
  glBindTexture(GL_TEXTURE_2D,textures[i]);
  //glBindTexture(GL_TEXTURE_2D,tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  //glEnable(GL_TEXTURE_2D);//Only use this if not using shaders.

  int w,h,comp;
  stbi_set_flip_vertically_on_load(1);
  unsigned char* pixels = stbi_load(file, &w, &h, &comp, STBI_rgb_alpha);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  char ratio[9];
  sprintf(ratio,"%sratio",name);
  uniform1f(ratio,(float) w/h);
  stbi_image_free(pixels);
}

void *readStdin() {
  while (1) {

    char * str;
    size_t l = 80;
    getline(&str,&l,stdin);
    char *n = strtok(str," ");
    UT_string *name;
    utstring_new(name);
    utstring_printf(name, n);

    if (utstring_find(name,0,"img",3) == 0) {
      struct image *i;
      i = (struct image*)malloc(sizeof(struct image));
      i->id = str[3]-'0';
      strncpy(i->path, strtok(NULL,"\n"),40);
      HASH_ADD_INT( images, id, i );
    }
    else if (utstring_find(name,0,"frag",4) == 0) {
      createShader("shader.vert",strtok(NULL,"\n"));
    }
    else if (utstring_find(name,0,"q",1) == 0) {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else {
      struct param *p;
      p = (struct param*)malloc(sizeof(struct param));
      strncpy(p->name, n,10);
      p->value = atof(strtok (NULL,"\n"));
      HASH_ADD_STR( parameters, name, p );
    }
  }
}

int main(int argc, char **argv) {

  createWindow();
  //createShader("shader.vert","cross.frag");
  createShader("shader.vert","shader.frag");
  glGenTextures(4, textures);
  for (int i = 0; i<4; i++) { readImage(argv[i+1],i); }
  uniform2f("resolution", width,height);
  
  pthread_t tid;
  pthread_create(&tid, NULL, readStdin, NULL);

  while (!glfwWindowShouldClose(window)) {
    uniform1f("time",glfwGetTime());
    // parameters mutex??
    // see uthash/doc/userguide.txt
    struct param *p, *tmp0;
    HASH_ITER(hh, parameters, p, tmp0) {
      printf("%s %.f\n",p->name,p->value);
      uniform1f(p->name,p->value);
      HASH_DEL(parameters, p);
      free(p);
    }
    struct image *i, *tmp1;
    HASH_ITER(hh, images, i, tmp1) {
      printf("%i %s\n",i->id,i->path);
      HASH_DEL(images, i);
      readImage(i->path,i->id);
      free(i);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  pthread_cancel(tid);
  glfwDestroyWindow(window);
  glfwTerminate();
 
  exit(EXIT_SUCCESS);
}
