#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <cstdlib>
#include <time.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

void init();
void display();
void idle();
void keyboard(unsigned char key, int x, int y );
void update(bool * world);
void set_pixbuf(bool * world, uint8_t * pixeldata);

int winw = 1024;
int winh = 1024;
int worldw = 1024;
int worldh = 1024;
int generations = 1000;
int n_particles = 1000;
float update_speed = 100; // updates per second (roughly);

int main(int argc, char ** argv){
    
    // Program-specific setup
    // initialize world
    srand(time(NULL));
    bool * world = new bool [worldw*worldh];
    memset(world,0,worldw*worldh*sizeof(bool));    
    for (int i=0; i < (worldw*worldh); i++){
        if ((float)rand()/(float)RAND_MAX<0.2)
            world[i] = true;
    }
    
    // Create our pixel buffer
    uint8_t * pixeldata = new uint8_t [worldw*worldh*4];
    memset(pixeldata,(bool)0,worldw*worldh*4);
    for (int i=0; i<worldh; i++){
        for (int j=0; j<worldw; j++){
            int idx = i*worldw + j;
            if (world[idx]==true){
                pixeldata[4*idx + 0] = 255;
                pixeldata[4*idx + 1] = 255;
                pixeldata[4*idx + 2] = 255;
                pixeldata[4*idx + 3] = 0;
            }
        }
    }
    
    // Create the window
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);  // RGBA Pixel format, Double buffered screen
    glutInitWindowSize(winw, winh);
    glutCreateWindow("OpenGL and CUDA Tests");

    glewInit();

    // Create the OpenGL context
    glutInitContextVersion(3,2);

    // Set up the viewport
    glViewport(0,0,winw,winh);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1.0f, 0, 1.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Enable depth sorting (wat?)
    glEnable(GL_DEPTH_TEST);

    // Set the clear color and clear the screen
    glClearColor( 0.5f, 0.5f, 1.0f, 1.0f );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    display();

    // Allocated the GL Buffer the same size as our image
    //GLuint bufferID;
    //glGenBuffers(1,&bufferID);
    //glBindBuffer(GL_PIXEL_UNPACK_BUFFER,bufferID);
    //glBufferData(GL_PIXEL_UNPACK_BUFFER,winw*winh*4,NULL,GL_DYNAMIC_COPY);

    // Create a GL texture
    GLuint textureID;
    glEnable(GL_TEXTURE_2D); // Enable texturing
    glGenTextures(1,&textureID); // Generate the texture ID
    glBindTexture(GL_TEXTURE_2D, textureID); // Make this texture the current texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, worldw, worldh, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata); // Allocate the texture memory w/o initialization
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);

    glBegin(GL_QUADS);
      glTexCoord2f( 0, 1.0f);
      glVertex3f(0,0,0);      
      glTexCoord2f(0,0);
      glVertex3f(0,1.0f,0);      
      glTexCoord2f(1.0f,0);
      glVertex3f(1.0f,1.0f,0);      
      glTexCoord2f(1.0f,1.0f);
      glVertex3f(1.0f,0,0);
    glEnd();
    display();    
    
    sleep(1);

    for (int i=0; i<generations; i++){
        usleep((int)((float)(1.0f/update_speed)*1E6));
        std::cout << "Generation: " << i << std::endl;
        update(world);
        set_pixbuf(world,pixeldata);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);        
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, worldw, worldh, GL_RGBA, GL_UNSIGNED_BYTE, pixeldata); // Allocate the texture memory w/o initialization        
        glBegin(GL_QUADS);
        glTexCoord2f( 0, 1.0f);
        glVertex3f(0,0,0);      
        glTexCoord2f(0,0);
        glVertex3f(0,1.0f,0);      
        glTexCoord2f(1.0f,0);
        glVertex3f(1.0f,1.0f,0);      
        glTexCoord2f(1.0f,1.0f);
        glVertex3f(1.0f,0,0);        
        glEnd();
        display();        
    }

    delete[] pixeldata;    
    // Set the window/glut callbacks
    glutDisplayFunc( display );
    glutKeyboardFunc( keyboard );
    glutIdleFunc( idle );
    glutMainLoop();
    
    return 0;
}

void init(void){
}

void display(void){
    glutSwapBuffers();
}

void idle(void){
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y){
    switch (key){
    case 'q': case 'Q':
        exit(0);
        break;
    case 'r': case 'R':
        break;
    }    
}

void update(bool * world){
    bool * tmp = new bool[worldw*worldh];
    memset(tmp,0,worldw*worldh*sizeof(bool));

    // Random updates (for testing)
    //for (int i=0; i < (worldw*worldh); i++){
    //    if ((float)rand()/(float)RAND_MAX<0.05)
    //        tmp[i] = true;
    //}

    // GOL updates
    for (int i=1; i<worldh-1; i++){
        for (int j=1; j<worldw-1; j++){
    
            int idx = i*worldw+j;
            
            int sum =
                (int)world[(i-1)*worldw + (j-1)] + (int)world[(i-1)*worldw +j] + (int)world[(i-1)*worldw +(j+1)] + 
                (int)world[(i)*worldw +(j-1)]    +                               (int)world[(i)*worldw + (j+1)]  + 
                (int)world[(i+1)*worldw + (j-1)] + (int)world[(i+1)*worldw + j]+ (int)world[(i+1)*worldw + (j+1)];
    
            if (world[idx] == true){
                if (sum < 2)
                    tmp[idx] = false;
                else if (sum ==2 || sum==3)
                    tmp[idx] = true;
                else
                    tmp[idx] = false;
            }
            else{
                if (sum == 3)
                    tmp[idx] = true;
            }            
        }        
    }

    for (int i=0; i<worldw*worldh; i++)
        world[i] = tmp[i];

    delete[] tmp;
}


void set_pixbuf(bool * world, uint8_t * pixeldata){
    for (int i=0; i<worldh; i++){
        for (int j=0; j<worldw; j++){
            int idx = i*worldw + j;
            if (world[idx]==true){
                pixeldata[4*idx + 0] = 255;
                pixeldata[4*idx + 1] = 255;
                pixeldata[4*idx + 2] = 255;
                pixeldata[4*idx + 3] = 0;
            }
            else{
                pixeldata[4*idx + 0] = 0;
                pixeldata[4*idx + 1] = 0;
                pixeldata[4*idx + 2] = 0;
                pixeldata[4*idx + 3] = 0;
            }            
        }
    }
}
