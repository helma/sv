// https://www.raspberrypi.org/forums/viewtopic.php?t=119859
#include <stdio.h>
#include <sys/stat.h>
#include "bcm_host.h"
#include "egl.h"
#include "gl2.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

EGLDisplay egldisplay;
EGLConfig eglconfig;
EGLSurface eglsurface;
EGLContext eglcontext;

GLuint shaderProgram;
GLuint backbuffer;

time_t shaderMtime;
int shaderNew;
uint32_t width, height;
int bars;

typedef struct Img {
  GLuint id;
  char path[40];
  float ratio;
  int new;
} Image;

Image images[4];

typedef struct Param {
  float value;
  int new;
} Parameter;

Parameter parameters[32];

void screenshot() {
  unsigned char pixels[width*height*4];
  time_t current_time = time(NULL);
  char output_file[25];
  strftime(output_file, 25, "%Y-%m-%d_%H%M%S.png", localtime(&current_time));
  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Flip the image on Y
  int depth = 4;
  int row,col,z;
  stbi_uc temp;
  for (row = 0; row < (height>>1); row++) {
   for (col = 0; col < width; col++) {
      for (z = 0; z < depth; z++) {
         temp = pixels[(row * width + col) * depth + z];
         pixels[(row * width + col) * depth + z] = pixels[((height - row - 1) * width + col) * depth + z];
         pixels[((height - row - 1) * width + col) * depth + z] = temp;
      }
   }
  }
  if (0 == stbi_write_png(output_file, width, height, 4, pixels, width * 4)) { printf("can't create file %s",output_file); }
}

int initShader() {
	const GLchar *vShaderStr[] = {
    "attribute vec4 vPosition;   \n"
    "void main(){                \n"
    "   gl_Position = vPosition; \n"
    "}                           \n"};
  FILE *file = fopen("shader.frag", "r");
  if (!file) { fprintf(stderr,"Cannot read %s\n","shader.frag"); }
  char source[10000];
  int res = fread(source,1,10000-1,file);
  source[res] = 0;
  fclose(file);
	const GLchar *fShaderStr = source;
	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(vShader, 1, vShaderStr, NULL);
	glCompileShader(vShader);
	glShaderSource(fShader, 1, &fShaderStr, NULL);
	glCompileShader(fShader);
  int success;
  glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(fShader, 512, NULL, infoLog);
    fprintf(stderr,"Shader compilation failed: %s\n",infoLog);
  }
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vShader);
	glAttachShader(shaderProgram, fShader);
	glLinkProgram(shaderProgram);
  //glDetachShader(shaderProgram, vShader); 
  //glDetachShader(shaderProgram, fShader); 
  //glDeleteShader(vShader);
  //glDeleteShader(fShader);

	GLfloat vVertices[] = {
   -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
   -1.0f,  1.0f, 0.0f,
    1.0f,  1.0f, 0.0f};

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	glEnableVertexAttribArray(0);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glUseProgram(shaderProgram);
}

void initWindow() {
	static const EGLint s_configAttribs[] = {
		EGL_RED_SIZE,		8,
		EGL_GREEN_SIZE, 	8,
		EGL_BLUE_SIZE,		8,
		EGL_ALPHA_SIZE, 	8,
		EGL_LUMINANCE_SIZE, EGL_DONT_CARE,
		EGL_SURFACE_TYPE,	EGL_WINDOW_BIT,
		EGL_SAMPLES,		1,
		EGL_NONE
	};

	static const EGLint context_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	bcm_host_init();
	int s;

	static EGL_DISPMANX_WINDOW_T window;
	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;

	s = graphics_get_display_size(0 /* LCD */, &width, &height);

	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = width;
	dst_rect.height = height;

	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = width << 16;
	src_rect.height = height << 16;

	dispman_display = vc_dispmanx_display_open( 0 /* LCD */);
	dispman_update = vc_dispmanx_update_start( 0 );

	dispman_element = vc_dispmanx_element_add ( dispman_update, dispman_display,
		1/*layer*/, &dst_rect, 0/*src*/,
		&src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);

	window.element = dispman_element;
	window.width = width;
	window.height = height;
	vc_dispmanx_update_submit_sync( dispman_update );

	EGLint numconfigs;

	egldisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(egldisplay, NULL, NULL);
	eglBindAPI(EGL_OPENGL_ES_API);

	eglChooseConfig(egldisplay, s_configAttribs, &eglconfig, 1, &numconfigs);

	eglsurface = eglCreateWindowSurface(egldisplay, eglconfig, &window, NULL);
	eglcontext = eglCreateContext(egldisplay, eglconfig, EGL_NO_CONTEXT, context_attributes);
	eglMakeCurrent(egldisplay, eglsurface, eglsurface, eglcontext);
}

void deinit(void) {
	eglMakeCurrent(egldisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglTerminate(egldisplay);
	eglReleaseThread();
}

void imageUniforms(int i) {
  glUniform1i(glGetUniformLocation(shaderProgram, "images")+i,i);
  glUniform1f(glGetUniformLocation(shaderProgram, "ratios")+i,images[i].ratio);
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
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  stbi_image_free(pixels);
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

void updateParams(int force) {
  glUniform1i(glGetUniformLocation(shaderProgram, "bars"), bars);
  for (int i = 0; i<32; i++) {
    if (parameters[i].new || force) {
      glUniform1f(glGetUniformLocation(shaderProgram, "params")+i, parameters[i].value);
      parameters[i].new = 0;
    }
  }
}

void *readStdin() {
  while (1) {
    char * str;
    size_t l = 80;
    getline(&str,&l,stdin);
    char *n = strtok(str," ");
    if (n[0] == 'i') {
      int i = atoi(strtok(NULL," "));
      strncpy(images[i].path, strtok(NULL,"\n"),40);
      images[i].new = 1;
    }
    /*
    else if (n[0] == 'f') {
      strncpy(shader.fragment, strtok(NULL,"\n"),40);
      shader.new = 1;
    }
    */
    else if (n[0] == 'b') {
      bars = atof(strtok (NULL,"\n"));
      //glfwSetTime(0);	
      printf("%i\n",bars);
    }
    else if (n[0] == 'p') {
      int i = atoi(strtok(NULL," "));
      parameters[i].value = atof(strtok (NULL,"\n"));
      parameters[i].new = 1;
    }
    else if (n[0] == 'q') {
      //glfwSetWindowShouldClose(window, GL_TRUE);
    }
  }
}

void *watchShader() {
  while (1) {
    sleep(0.2);
    struct stat file_stat;
    stat("shader.frag", &file_stat);
    if (file_stat.st_mtime > shaderMtime) shaderNew = 1; 
  }
}

int main(int argc, char **argv) {

	initWindow();
  for (int i = 1; i<argc; i++) {
    strncpy(images[i-1].path, argv[i],40);
    images[i-1].new = 1;
  }
  /*
  glCreateTextures(GL_TEXTURE_2D,1,&backbuffer);
  glTextureStorage2D(backbuffer,1,GL_RGBA32F,width,height);
  glBindImageTexture(0,backbuffer, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
  */
  
  pthread_t stdin_t;
  pthread_create(&stdin_t, NULL, readStdin, NULL);
  pthread_t frag_t;
  pthread_create(&frag_t, NULL, watchShader, NULL);

   while (1) {
    if (shaderNew) {
      initShader();
      glUniform2f(glGetUniformLocation(shaderProgram, "resolution"),width,height); // important!!
      for (int i = 0; i<4; i++) { imageUniforms(i); }
      updateParams(1);
      shaderNew = 0;
    }
    updateImages(0);
    updateParams(0);
		//clear the buffer
		//glClear(GL_COLOR_BUFFER_BIT);

		//Draw the triangles
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		
		//actually draw the stuff to the screen
		eglSwapBuffers(egldisplay, eglsurface);

   }
   deinit();

   return 0;
}
