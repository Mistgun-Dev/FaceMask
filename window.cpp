/*!\file window.c
 *
 * \brief Utilisation de GL4Dummies et d'OpenGL 3+
 *
 * \author Farès BELHADJ, amsi@ai.univ-paris8.fr
 * \date October 27 2016
 */
#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4duw_SDL2.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/core/core.hpp>
#include <objdetect.hpp>
#include <SDL2/SDL_image.h>

/* Prototypes des fonctions statiques contenues dans ce fichier C */
static void init(void);
static void resize(int w, int h);
static void draw(void);
static void quit(void);
static void faceDetection(IplImage *img, IplImage *imgG);

using namespace std;
using namespace cv;

static CascadeClassifier * face_cc;
static CascadeClassifier * eye_cc;
/*!\brief dimensions de la fenêtre */
static int _w = 800, _h = 600;
/*!\brief identifiant du quadrilatère */
static GLuint _quad = 0;
static GLuint _quadEffect = 0;
/*!\brief identifiants des (futurs) GLSL programs */
static GLuint _pId = 0;
/*!\brief identifiant de la texture chargée */
static GLuint _tId = 0;
/*!\brief device de capture OpenCV */
CvCapture * _capture = NULL;
SDL_Surface *texSurface;
static GLuint _effectTexture = 0;
static CvSize s;
static GLfloat posVisageX, posVisageY, scaleRefX, scaleRefY, rotateEffect;


/*!\brief Création de la fenêtre et paramétrage des fonctions callback.*/
int main(int argc, char ** argv)
{
  face_cc = new CascadeClassifier("data/haarcascade_frontalface_default.xml");
  eye_cc = new CascadeClassifier("data/haarcascade_eye.xml");
  if(face_cc == NULL || eye_cc == NULL)
    exit(0);
  
  if(!(_capture = cvCaptureFromCAM(1)))
    _capture = cvCaptureFromCAM(CV_CAP_ANY);
  s.width = _w = (int)cvGetCaptureProperty(_capture, CV_CAP_PROP_FRAME_WIDTH);
  s.height = _h = (int)cvGetCaptureProperty(_capture, CV_CAP_PROP_FRAME_HEIGHT);
  
  if(!gl4duwCreateWindow(argc, argv, "GL4Dummies", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
			 _w, _h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN))
    return 1;
  
  init();
  atexit(quit);
  gl4duwResizeFunc(resize);
  gl4duwDisplayFunc(draw);
  gl4duwMainLoop();
  
  return 0;
}

/*!\brief Initialise les paramètres OpenGL.*/
static void init(void)
{
  glEnable(GL_BLEND); 
  glEnable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glClearColor(1.0f, 0.7f, 0.7f, 0.0f);
  glEnable(GL_TEXTURE_2D);
  _pId = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  _quad = gl4dgGenQuadf();
  _quadEffect = gl4dgGenQuadf();
  
  glGenTextures(1, &_tId);
  glBindTexture(GL_TEXTURE_2D, _tId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  
  glGenTextures(1, &_effectTexture);
  glBindTexture(GL_TEXTURE_2D, _effectTexture);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  texSurface = IMG_Load("pictures/effect.png");
  assert(texSurface);
#if __APPLE__
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSurface->w, texSurface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, texSurface->pixels);
#else
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSurface->w, texSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texSurface->pixels);
#endif
  
  gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
  resize(_w, _h);
}

/*!\brief Paramétrage de la vue (viewPort) OpenGL en fonction
 * des dimensions de la fenêtre.
 * \param w largeur de la fenêtre transmise par GL4Dummies.
 * \param h hauteur de la fenêtre transmise par GL4Dummies.
 */
static void resize(int w, int h)
{
  _w  = w; _h = h;
  glViewport(0, 0, _w, _h);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duOrthof(0, 5, -5, 0, 0.0, 1000);
  gl4duBindMatrix("modelViewMatrix");
}

/*!\brief Dessin de la géométrie texturée. */
static void draw(void)
{
  
  IplImage *imgG = NULL, *img = cvQueryFrame(_capture);
  float _scaleX, _scaleY;
  float posEffX, posEffY;
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
  glUseProgram(_pId);
  
  faceDetection(img, imgG);
  
  glBindTexture(GL_TEXTURE_2D, _tId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_BGR, GL_UNSIGNED_BYTE, img->imageData);
  glDisable(GL_DEPTH_TEST);
  
  glBindTexture(GL_TEXTURE_2D, _tId);
  glUseProgram(_pId);
  glUniform1i(glGetUniformLocation(_pId, "redim"), 0);
  glUniform1i(glGetUniformLocation(_pId, "inv"), 1);
  gl4duSendMatrices();
  gl4dgDraw(_quad);
  
  gl4duPushMatrix();
  gl4duLoadIdentityf();
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindTexture(GL_TEXTURE_2D, _effectTexture);
  glUniform1i(glGetUniformLocation(_pId, "redim"), 1);

  //Scale de l'effet en fonction de la profondeur du visage
  _scaleX = (scaleRefX / _w * 3.1);
  _scaleY = (scaleRefY / _h * 4.0);
  
  //Position de l'effet en fonction de l'unité du repere
  posEffX = (posVisageX / _w * 5) / _scaleX ;
  posEffY = (posVisageY / _h * 5) / _scaleY ;

  gl4duScalef(_scaleX, _scaleY, 0);
  gl4duTranslatef(posEffX, -posEffY, 0);
  //gl4duRotatef(-rotateEffect,0,0,1);
  gl4duSendMatrices();
  gl4dgDraw(_quadEffect);
  gl4duPopMatrix();
}

/*!\brief Appelée au moment de sortir du programme (atexit). Elle
 *  libère les données utilisées par OpenGL et GL4Dummies.*/
static void quit(void)
{
  if(_capture)
    cvReleaseCapture(&_capture);
  if(_tId)
    glDeleteTextures(1, &_tId);
  gl4duClean(GL4DU_ALL);
}

static void faceDetection(IplImage *img, IplImage *imgG)
{
  Mat ci = (Mat)img;
  imgG = cvCreateImage(s, IPL_DEPTH_8U, 1);
  cvCvtColor(img, imgG, CV_RGB2GRAY);
  Mat gsi = (Mat)imgG;
  vector<Rect> faces;

  face_cc->detectMultiScale(gsi, faces, 1.3, 5);

      for (vector<Rect>::iterator fc = faces.begin(); fc != faces.end(); ++fc)
	{
	  //Nouvelle taille de l'effet, en fonction de la taille du visage
	  scaleRefX   = (float)(*fc).br().x - (float)(*fc).tl().x;
	  scaleRefY   = (float)(*fc).br().y - (float)(*fc).tl().y;

	  //Position du centre du visage
	  posVisageX = (float)(*fc).tl().x + (((float)(*fc).br().x - (float)(*fc).tl().x) / 2);
	  posVisageY = (float)(*fc).tl().y + (((float)(*fc).br().y - (float)(*fc).tl().y) / 2);

	}
}


/*
static void faceDetection(IplImage *img, IplImage *imgG)
{
  imgG = cvCreateImage(s, IPL_DEPTH_8U, 1);
  cvCvtColor(img, imgG, CV_RGB2GRAY);
  Mat ci = (Mat)imgG;
  Mat gsi;
  vector<Rect> faces;
  Point2f temp(ci.cols/2.0, ci.rows/2.0);
  Mat rot_matrix, r;
  int success = 0;
  
  for(int i=-90 ; i<90; i+=20)
    {
      r = getRotationMatrix2D(temp, i, 1.0);
      warpAffine(ci, rot_matrix, r , ci.size());
     
      gsi = rot_matrix;
      face_cc->detectMultiScale(gsi, faces, 1.3, 3);
      
      for (vector<Rect>::iterator fc = faces.begin(); fc != faces.end(); ++fc)
	{
	  fprintf(stderr,"aa\n");
	  rotateEffect = i;
	  success = 1;
	  
	  scaleRefX   = (float)(*fc).br().x - (float)(*fc).tl().x;
	  scaleRefY   = (float)(*fc).br().y - (float)(*fc).tl().y;
	  //Position du centre du visage
	  posVisageX = (float)(*fc).tl().x + (((float)(*fc).br().x - (float)(*fc).tl().x) / 2);
	  posVisageY = (float)(*fc).tl().y + (((float)(*fc).br().y - (float)(*fc).tl().y) / 2);

	  ci = (Mat)img;
	  rectangle(ci, (*fc).tl(), (*fc).br(), Scalar(0, 255, 0), 2, CV_AA);  	 
	}
      if(success == 1)
	return;
    }
  
}
*/
