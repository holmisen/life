/* Compile like so:
    cc -o life -lSDL -lSDL_gfx life.c
*/

#include <stdlib.h>
#include <stdio.h>   /* sscanf */
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_thread.h>


#define SIZE    5
#define MAX_WIDTH  500
#define MAX_HEIGHT 500

static int WIDTH  = 100 ;
static int HEIGHT =  90 ;

#define Off   0
#define Dying 1
#define On    2

static SDL_Surface *screen;


static int space[2][MAX_WIDTH][MAX_HEIGHT];
static int src = 0 ;
static int dst = 1 ;

void flip () { src = dst ; dst = (dst+1)%2 ; }


inline int iN (int y) { return (y < 1 ? HEIGHT - 1 : y - 1); }
inline int iW (int x) { return (x < 1 ? WIDTH  - 1 : x - 1); }
inline int iS (int y) { return ((y+1) % HEIGHT); }
inline int iE (int x) { return ((x+1) % WIDTH ); }

void draw_cell (int x , int y)
{
   Uint32   col;
   SDL_Rect r = { x*SIZE, y*SIZE, SIZE, SIZE };
   
   switch (space[dst][x][y]) {
      case On    : col = 0xFFFFFFFF ; break;
      case Dying : col = 0xAAAAAAFF ; break;
      default    : col = 0x000050FF ;
   }

/*   boxColor( screen, x*SIZE, y*SIZE, x*SIZE+SIZE-1, y*SIZE+SIZE-1, col ); */
   SDL_FillRect (screen, &r, col);
}

void draw_space ()
{
   int i, j ;
   for (i = 0 ; i < WIDTH ; ++i)
      for (j = 0 ; j < HEIGHT ; ++j)
         draw_cell (i,j) ;
}

int alive (int x , int y) { return (space[src][x][y] == On ? 1 : 0); }

int neighs (int x , int y)
{
   int iiN = iN (y);
   int iiS = iS (y);
   int iiE = iE (x);
   int iiW = iW (x);
   
   return (alive(iiE, y ) + alive(iiE,iiS) +
           alive( x ,iiS) + alive(iiW,iiS) +
           alive(iiW, y ) + alive(iiW,iiN) +
           alive( x ,iiN) + alive(iiE,iiN));
}

int status (int v, int n)
{
   switch (v) {
      case Off  : return (n == 2 ? On : Off); break;
      case On   : return Dying; break;
      default   : return Off;
   }
}

SDL_mutex  *global_mutex ;

void step_part (int fromI, int toX)
{
   int x,y;

   for (x = fromI ; x < toX ; ++x) {
      for (y = 0 ; y < HEIGHT ; ++y) {
         space[dst][x][y] = status (space[src][x][y], neighs (x,y));
         if ( space[dst][x][y] != space[src][x][y] ) {
            if ( SDL_mutexP(global_mutex) == -1) {
               fprintf(stderr, "No mutex lock\n");
               exit(-1);
            }
            draw_cell (x,y) ;
            if ( SDL_mutexV (global_mutex) == -1) {
               fprintf(stderr, "No mutex release\n");
               exit(-1);
            }
         }
      }
   }
}

int step_p1 (void *x) { step_part (0, WIDTH/2) ; }
int step_p2 (void *x) { step_part (WIDTH/2, WIDTH) ; }
int step_p3 (void *x) { step_part (2*WIDTH/3, WIDTH) ; }

void step ()
{
   int x,y;
   SDL_Thread *t1,*t2,*t3 ;
   
   /* Calculate */
   t1 = SDL_CreateThread (step_p1, NULL);
   t2 = SDL_CreateThread (step_p2, NULL);
   SDL_WaitThread (t1, NULL);
   SDL_WaitThread (t2, NULL);
   
   /* Draw */
   SDL_LockSurface (screen);
   for (x = 0 ; x < WIDTH ; ++x) {
      for (y = 0 ; y < HEIGHT ; ++y) {
         if ( space[dst][x][y] != space[src][x][y] )
            draw_cell (x,y);
      }
   }
   SDL_UnlockSurface (screen);
}

void action_mouse_left (Uint16 sX , Uint16 sY, int v)
{
   int x = sX / SIZE ;
   int y = sY / SIZE ;

   space[dst][x][y] = v ;
   draw_cell (x,y) ;
   SDL_Flip (screen);
}

void action_clear_space ()
{
   int i,j;
   for (i = 0 ; i < WIDTH ; ++i)
      for (j = 0 ; j < HEIGHT ; ++j) {
         space[0][i][j] = Off;
         space[1][i][j] = Off;
         draw_cell (i,j) ;
      }
   
   SDL_Flip (screen);
}

void pause ()
{
   SDL_Event ev;
   int       mBtn = -1 ;
   
   while (1) {
      SDL_WaitEvent(&ev);
      switch (ev.type) {
      case SDL_KEYDOWN:
         if (ev.key.keysym.sym == SDLK_SPACE) {
            return;
         }
         else if (ev.key.keysym.sym == SDLK_RETURN) {
            flip ();
            step ();
            SDL_Flip (screen);
         }
         else if (ev.key.keysym.sym == SDLK_BACKSPACE)
            action_clear_space ();
         break;
      case SDL_MOUSEBUTTONDOWN:
         if (ev.button.state == SDL_PRESSED)
            mBtn = (int)(ev.button.button);
         else mBtn = -1;
         if (mBtn == SDL_BUTTON_LEFT) 
            action_mouse_left (ev.button.x , ev.button.y, On);
         else if (mBtn == SDL_BUTTON_RIGHT)
            action_mouse_left (ev.button.x , ev.button.y, Off);
         else if (mBtn == SDL_BUTTON_MIDDLE)
            action_mouse_left (ev.button.x , ev.button.y, Dying);
         break;
      case SDL_MOUSEBUTTONUP:
         mBtn = -1;
         break;
      case SDL_MOUSEMOTION:
         if (mBtn == SDL_BUTTON_LEFT)
            action_mouse_left (ev.button.x , ev.button.y, On);
         else if (mBtn == SDL_BUTTON_RIGHT)
            action_mouse_left (ev.button.x , ev.button.y, Off);
         else if (mBtn == SDL_BUTTON_MIDDLE)
            action_mouse_left (ev.button.x , ev.button.y, Dying);
         break;
      case SDL_QUIT : exit(0);
      }
   }
}


void main (int argc, char *argv[])
{
   SDL_Event    ev;
   int i,j;
   Uint32       ticks ;

   if ( SDL_Init (SDL_INIT_VIDEO) < 0 ) {
      fprintf (stderr, "Unable to init SDL: %s\n", SDL_GetError() );
      exit(1);
   }
   atexit(SDL_Quit);
   
   global_mutex = SDL_CreateMutex () ;

   if ( argc > 1 ) {
      sscanf(argv[1], "%d", &WIDTH);
      if ( argc > 2 )
         sscanf(argv[2], "%d", &HEIGHT);
      else
         sscanf(argv[1], "%d", &HEIGHT);
   }
   if ( WIDTH > MAX_WIDTH || HEIGHT > MAX_HEIGHT ) {
      fprintf( stderr, "Cannot be bigger than 500x500" );
      exit(1);
   }
   
   screen = SDL_SetVideoMode( WIDTH*SIZE, HEIGHT*SIZE, 0, SDL_HWSURFACE | SDL_DOUBLEBUF );
   SDL_WM_SetCaption("Life", 0);
   
   /* Random start space */
   for (i = 0 ; i < WIDTH ; ++i)
      for (j = 0 ; j < HEIGHT ; ++j)
         space[1][i][j] = rand() % 3;

   draw_space ();

   ticks = SDL_GetTicks ();
   while (1) {
      flip ();
      step ();
      SDL_Flip (screen);
      while (SDL_PollEvent(&ev)) {
         switch ( ev.type ) {
            case SDL_QUIT:
               exit(0);
            case SDL_KEYDOWN:
               if (ev.key.keysym.sym == SDLK_SPACE) {
                  pause () ;
                  ticks = SDL_GetTicks ();
               }
         }
      }
/*      SDL_Delay(50);  */

/*
      SDL_Delay( (50 + ticks) - SDL_GetTicks () );
      ticks = SDL_GetTicks () ;
*/      
   }
}
