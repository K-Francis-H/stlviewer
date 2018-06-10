/*
 * Copyright (c) 2012, Vishal Patil
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <unistd.h>


#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef _Linux_
#include <GL/glut.h>
#endif

#ifdef _Darwin_
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif


#include "stl.h"
#include "bitmap.h"

#define MAX( x, y) (x) > (y) ? (x) : (y)
#define MIN( x, y) (x) < (y) ? (x) : (y)

#define DEFAULT_SCALE 1.0
#define DEFAULT_ROTATION_X 0
#define DEFAULT_ROTATION_Y 0
#define DEFAULT_ROTATION_Z 0
#define DEFAULT_ZOOM 2.5 //2.0

#define MAX_Z_ORTHO_FACTOR 20
#define ROTATION_FACTOR 15

#define SCREEN_SIZE 256

static int wiremesh = 0;
static GLfloat scale = DEFAULT_SCALE;
static stl_t *stl;
static float ortho_factor = 1.5;
static float zoom = DEFAULT_ZOOM;

static int screen_width = 0;
static int screen_height = 0;

static float global_ambient_light[4] = {0.0, 0.0, 0.0, 0};
static float light_ambient[4] = {0.3, 0.3, 0.3, 0.0};
static float light_diffuse[4] = {0.5, 0.5, 0.5, 1.0};
static float light_specular[4] = {0.5, 0.5, 0.5, 1.0};
static float mat_shininess[] = {10.0};
static float mat_specular[] = { 0.5, 0.5, 0.5, 1.0 };

static float x_ang = -55.0;
static float y_ang = 0.0;
static float z_ang = 0.0;
static float x_change = 0.0;
static float y_change = 0.0;
static float z_change = 5.0;
static GLfloat rot_matrix[16];
void rotate(float m[16], float x_deg, float y_deg, float z_deg);

GLuint fbo_id, rbo_id;

char* gif_file_out = "stl.gif";

void screenshot(int frame_num);

static GLuint model;

typedef struct {
	GLfloat x;
	GLfloat y;
	GLfloat z;
} vector_t;

static void
ortho_dimensions(GLfloat *min_x, GLfloat *max_x,
                GLfloat *min_y, GLfloat *max_y,
                GLfloat *min_z, GLfloat *max_z)
{
	GLfloat diff_x = stl_max_x(stl) - stl_min_x(stl);
	GLfloat diff_y = stl_max_y(stl) - stl_min_y(stl);
	GLfloat diff_z = stl_max_z(stl) - stl_min_z(stl);

        GLfloat max_diff = MAX(MAX(diff_x, diff_y), diff_z);

        *min_x = stl_min_x(stl) - ortho_factor*max_diff;
	*max_x = stl_max_x(stl) + ortho_factor*max_diff;
	*min_y = stl_min_y(stl) - ortho_factor*max_diff;
	*max_y = stl_max_y(stl) + ortho_factor*max_diff;
	*min_z = stl_min_z(stl) - MAX_Z_ORTHO_FACTOR * ortho_factor*max_diff;
	*max_z = stl_max_z(stl) + MAX_Z_ORTHO_FACTOR * ortho_factor*max_diff;
}

static void
reshape(int width, int height)
{
	int size = MIN(width, height);
        GLfloat min_x, min_y, min_z, max_x, max_y, max_z;
        
        screen_width = width;
        screen_height = height;

	int width_half = width / 2;
	int height_half = height / 2;

	glViewport(width_half - (size/2), height_half - (size/2) ,
			size, size);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

        ortho_dimensions(&min_x, &max_x, &min_y, &max_y, &min_z, &max_z);
	glOrtho(min_x, max_x, min_y, max_y, min_z, max_z);

	glMatrixMode(GL_MODELVIEW);

        glLoadIdentity();
	// Set global ambient light
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient_light);

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
        float Lt0pos[] = {0, 0, 0, 1};
	glLightfv(GL_LIGHT0, GL_POSITION, Lt0pos);

	glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);
        float Lt1pos[] = {max_x, max_y, max_z, 0};
	glLightfv(GL_LIGHT1, GL_POSITION, Lt1pos);

        glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
        glEnable(GL_LIGHT1);
}


static void
drawTriangle(	GLfloat x1, GLfloat y1, GLfloat z1,
		GLfloat x2, GLfloat y2, GLfloat z2,
		GLfloat x3, GLfloat y3, GLfloat z3)
{
	vector_t normal;
	vector_t U;
	vector_t V;
	GLfloat length;

	U.x = x2 - x1;
	U.y = y2 - y1;
	U.z = z2 - z1;

	V.x = x3 - x1;
	V.y = y3 - y1;
	V.z = z3 - z1;

	normal.x = U.y * V.z - U.z * V.y;
	normal.y = U.z * V.x - U.x * V.z;
	normal.z = U.x * V.y - U.y * V.x;

	length = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
	length = sqrt(length);

	glNormal3f(normal.x / length, normal.y / length, normal.z / length);
	glVertex3f(x1, y1, z1);
	glVertex3f(x2, y2, z2);
	glVertex3f(x3, y3, z3);
}

void
drawBox(void)
{
	GLfloat *vertices = NULL;
	GLuint triangle_cnt = 0;
	int i = 0, base = 0;
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glPushMatrix();

	glTranslatef(
		((stl_max_x(stl) + stl_min_x(stl))/2),
		((stl_max_y(stl) + stl_min_y(stl))/2),
		((stl_max_z(stl) + stl_min_z(stl))/2));


	glScalef(zoom, zoom, zoom);

        //GLfloat rot_matrix[4][4];
        //build_rotmatrix(rot_matrix, rot_cur_quat);
        glMultMatrixf(&rot_matrix[0]);

	glTranslatef(
		-((stl_max_x(stl) + stl_min_x(stl))/2),
		-((stl_max_y(stl) + stl_min_y(stl))/2),
		-((stl_max_z(stl) + stl_min_z(stl))/2));

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular );
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	//get stl stuff
	stl_error_t err =  stl_vertices(stl, &vertices);
	triangle_cnt = stl_facet_cnt(stl);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);//fbo_id);
        //glCallList(model);
	glBegin(GL_TRIANGLES);
	for (i = 0; i < triangle_cnt; i++) {
		base = i*18;
		drawTriangle(vertices[base], vertices[base + 1], vertices[base + 2],
			     vertices[base + 6], vertices[base + 7], vertices[base + 8],
			     vertices[base + 12], vertices[base + 13], vertices[base + 14]);
	}
	glEnd();

        glPopMatrix();

	glFlush();
	
	glutSwapBuffers();
}

int frame_count = 0;
void
idle_func(void)
{
	//TODO turn these into cmd line vars
	z_ang += z_change;
	x_ang += x_change;
	y_ang += y_change;
	rotate(rot_matrix, x_ang, y_ang, z_ang);
	screenshot(frame_count);
	if(z_ang >= 360.0){
		exit(0);
	}else{
		frame_count++;
	}
	glutPostRedisplay();
}


void
display(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawBox();
}

void
init(char *filename)
{
	stl_error_t err;
	GLfloat *vertices = NULL;
	GLuint triangle_cnt = 0;
	int i = 0, base = 0;

 	stl = stl_alloc();
	if (stl == NULL) {
		fprintf(stderr, "Unable to allocate memoryfor the stl object");
		exit(1);
	}

	err = stl_load(stl, filename);

	if (err != STL_ERR_NONE) {
		fprintf(stderr, "Problem loading the stl file, check lineno %d\n",
			stl_error_lineno(stl));
		exit(1);
	}

	err =  stl_vertices(stl, &vertices);
	if (err) {
		fprintf(stderr, "Problem getting the vertex array");
		exit(1);
	}

	triangle_cnt = stl_facet_cnt(stl);

        model = glGenLists(1);
        glNewList(model, GL_COMPILE);
        glBegin(GL_TRIANGLES);
	for (i = 0; i < triangle_cnt; i++) {
		base = i*18;
		drawTriangle(vertices[base], vertices[base + 1], vertices[base + 2],
			     vertices[base + 6], vertices[base + 7], vertices[base + 8],
			     vertices[base + 12], vertices[base + 13], vertices[base + 14]);
	}
	glEnd();
        glEndList();

	if(wiremesh){
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}else{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

        glClearColor(135.0 / 255, 206.0 / 255.0, 250.0 / 255.0, 0.0);	//TODO make this an input, but this the default
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(120.0 / 255.0 , 120.0 / 255.0, 120.0 / 255.0);

        glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_TEXTURE_2D);

	glFlush();
}



void rotate(float m[16], float x_deg, float y_deg, float z_deg){
	float csx = cos(M_PI * x_deg/180.0);
	float snx = sin(M_PI * x_deg/180.0);
	float csy = cos(M_PI * y_deg/180.0);
	float sny = sin(M_PI * y_deg/180.0);
	float csz = cos(M_PI * z_deg/180.0);
	float snz = sin(M_PI * z_deg/180.0);

	m[0] = (csy*csz);              m[4] = (-csy*snz);             m[8] = sny;        m[12] = 0;//position.x();
	m[1] = (snx*sny*csz+csx*snz);  m[5] = (-snx*sny*snz+csx*csz); m[9] = (-snx*csy); m[13] = 0;//position.y();
	m[2] = (-csx*sny*csz+snx*snz); m[6] = (csx*sny*snz+snx*csz);  m[10] = (csx*csy); m[14] = 0;//position.z();
	m[3] = 0;                      m[7] = 0;                      m[11] = 0;         m[15] = 1;
}

void screenshot(int frame_num){

	GLuint pixels[SCREEN_SIZE*SCREEN_SIZE];
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_id);
	//glReadBuffer(GL_BACK);//read from renderbuffer
	glReadPixels(0,0, SCREEN_SIZE, SCREEN_SIZE, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pixels);

	int i;
	for(i=0; i < SCREEN_SIZE*SCREEN_SIZE; i++){
		pixels[i] >>= 8; //prevents weird interpeting of alpha as color leading to shitty blue shift
	}
	char file_name[24];
	//char *file_name = NULL;

	if(frame_num < 10){
		//file_name = (char*)malloc(sizeof(char)*22);//+1 for \0
		sprintf(file_name, "/tmp/stl2gif/000%i.bmp", frame_num);
		//file_name[21] = '\0';
	}else if(frame_num < 100){
		//file_name = (char*)malloc(sizeof(char)*23);//+1 for \0
		sprintf(file_name, "/tmp/stl2gif/00%i.bmp", frame_num);
		//file_name[22] = '\0';
	}else{
		//file_name = (char*)malloc(sizeof(char)*24);//+1 for \0
		sprintf(file_name, "/tmp/stl2gif/0%i.bmp", frame_num);
		//file_name[23] = '\0';
	}
	//printf("%s\n", file_name);
    	generateBitmapImage((unsigned char *)pixels, SCREEN_SIZE, SCREEN_SIZE, file_name);


}




int main(int argc, char **argv)
{

  char* stl_file_in = NULL;

  //setup xyz angle changes
  x_change = 0.0;
  y_change = 0.0;
  z_change = 5.0; //default af
  
  opterr = 0; //from <unistd.h>
  int flag;
  int is_infile = 0;
  //options help, version, infile, outfile, x rotation angle, y rotation angle, z rotation angle, zoom(see) factor
  while( (flag = getopt(argc, argv, "hvwi:o:x:y:z:s:")) != -1 ){
	switch(flag){
		case 'h':
			//TODO print help message
			printf("usage: ./stl2gif -i [FILE] [OPTIONS] \n"); 
			exit(0);
			break;
		case 'v':
			//TODO print version string
			printf("STL 2 GIF Version X\n");
			exit(0);
			break;
		case 'w':
			wiremesh = 1;
			break;
		case 'i':
			stl_file_in = optarg;
			is_infile = 1;
			break;
		case 'o':
			//TODO rename default out file, doesn't work cause thats done by the shell
			gif_file_out = optarg;
			break;
		case 'x':
			x_change = strtof(optarg, optarg+strlen(optarg));
			break;
		case 'y':
			y_change = strtof(optarg, optarg+strlen(optarg));
			break;
		case 'z':
			z_change = strtof(optarg, optarg+strlen(optarg));
			break;
		case 's':
			zoom = strtof(optarg, optarg+strlen(optarg));
			break;
		case '?':
		default:
			//if(strcmp(optopt, "--") == 0){
			//	continue;
			//}	
			switch(optopt){
				case '-': //long arg --version or --help
					break;
				case 'i':
				case 'o':;
				case 'x':
				case 'y':
				case 'z':
				case 's':
					fprintf(stderr, "Option -%c requires an argument\n", optopt);
					break;
				default:
					fprintf(stderr, "Unknown option -%c\n", optopt);
					break;
			}
			//exit(0);
			break;
	}
  }
  //now check for --help, --version based off of optind
  int i;
  for(i=optind; i < argc; i++){
	if(strcmp(argv[i], "--help") == 0){
		//TODO print help message
		printf("usage: ./stl2gif -i [FILE] [OPTIONS] \n");
		exit(0);
	}
	if(strcmp(argv[i], "--version") == 0){
		//TODO print version string
		printf("STL 2 GIF Version X\n");
		exit(0);
	}
	//else ignore
  }

  if(!is_infile){
	printf("usage: ./stl2gif -i [FILE] [OPTIONS] \n");
	exit(0);
  }


  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

  glutInitWindowSize(SCREEN_SIZE, SCREEN_SIZE);
  glutCreateWindow(stl_file_in);
  glutHideWindow();
  
  
  init(stl_file_in);
  reshape(SCREEN_SIZE, SCREEN_SIZE);
  while(1){
	display();
	idle_func();
  }
  return 0;             /* ANSI C requires main to return int. */
}
