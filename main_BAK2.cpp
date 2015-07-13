/*	Resources:
	
	PIXEL ART:
	http://www.gotoandplay.it/_articles/2004/10/tcgtipa_p02.php
	
	
		*/

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <time.h>
#include <thread>
#include <fstream>

#include "Astar.h"

using namespace std;

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 640;

const int LEVEL_WIDTH = 16;
const int LEVEL_HEIGHT = 16;

const int TILE_WIDTH = 64;
const int TILE_HEIGHT = 32;

const int FPS = 60;



int init( SDL_Window **window, SDL_Renderer **renderer, SDL_Surface **screenSurface );
int gameloop( SDL_Renderer *renderer );
int draw( SDL_Renderer *renderer );
void handle_input( bool *quit, bool *key_pressed, bool *lvl_cng );
void matrix_to_iso( int *x, int *y );
void iso_to_matrix( int *x, int *y );


/*	
	==============================
	CLASS MAP, HOLDS THE GAMEMAP:
	==============================
		*/
class Map {
	int width;
	int height;
	int **matrix;
	
	public:
		void load_map( std::string path );
		int tileType( int i, int j ) { return matrix[i][j]; };
		bool isPassable( int i, int j ) { if(matrix[j][i] == 0){ return true;} else {return false;} };
		int **get_matrix() { return matrix; };
		~Map();

};

/*	
	=====================================
	CLASS ROUTE - A UNIT MIGHT HAVE ONE,
	IT WILL MOVE ALONG PATH IF THAT IS
	THE CASE:
	=====================================
		*/
class Route {
	int start_x;
	int start_y;
	int finish_x;
	int finish_y;
	
	node *path;
	node *closed_list;
	int **path_matrix;
	bool active;
	
	public:
		Route() {
			path_matrix = new int*[LEVEL_HEIGHT];
			for( int i = 0; i < LEVEL_HEIGHT; i++ )
				path_matrix[i] = new int[LEVEL_WIDTH];
				
			set_path_matrix();
			
			path = NULL;
			closed_list = NULL;
			
			active = false;
		};
		void set_start( int x, int y ) { start_x = x; start_y = y; };
		void set_finish( int x, int y ) { finish_x = x; finish_y = y; };
		int get_start_x() { return start_x; };
		int get_start_y() { return start_y; };
		int get_finish_x() { return finish_x; };
		int get_finish_y() { return finish_y; };
		node **get_ptr_to_path() { return &path; };
		node **get_ptr_to_closed() { return &closed_list; };
		void set_path_matrix() {
			for ( int i = 0; i < LEVEL_HEIGHT; i += 1) {
				for ( int j = 0; j < LEVEL_WIDTH; j += 1) {
					path_matrix[j][i] = 0;
				}
			}
			node *temp = closed_list;
			while( temp != NULL ) {
				path_matrix[temp->y][temp->x] = 1;
				temp = temp->next;
			}
			
			temp = path;
			while( temp != NULL ) {
				path_matrix[temp->y][temp->x] = 2;
				temp = temp->next;
			}
			
		};
		bool tile_closed(int i, int j) {  if (path_matrix[j][i] == 1) return true; else return false; };
		bool tile_on_path(int i, int j) { if (path_matrix[j][i] == 2) return true; else return false; };
		bool isActive() { return active; };
		void setActive( bool a ) { active = a; };
};




/*	
	=====================================
	BASE CLASS FOR BASICALLY EVERYTHING:
	=====================================
		*/
class Entity {
	protected:
		Uint32 w;
		Uint32 h;
		SDL_Texture *texture;
	
	public:
		void loadSurface( SDL_Renderer *renderer, std::string path );
		void render( SDL_Renderer *renderer, int x, int y );
		void set_dim( int width, int height ) { w = width; h = height; };
		//~Entity(){ SDL_DestroyTexture(texture); };
};


/*	
	===============================================
	UNIT CLASS FOR EVERYTHING REPRESENTING A UNIT,
	INHERITS ENTITY AND ROUTE:
	===============================================
		*/
class Unit : public Entity, public Route {
	int xval;	/*	COORDS IN MATRIX!	*/
	int yval;
	int px;		/*	COORDS IN PIXELS!	*/
	int py;
	/*	frame indices:	*/
	Uint32 max_frames;
	Uint32 cur_frame;
	/*	timers:	*/
	Uint32 last_frame_chg;
	Uint32 last_move;
	
	bool idle;
	
	public:
		Unit() {
			idle = true;
			setActive(false);
		};
		void render( SDL_Renderer *renderer );
		void set_pos( int a, int b ) {
			xval = a;
			yval = b;
			px = ( xval - yval ) * (TILE_WIDTH / 2);
			py = ( xval + yval ) * (TILE_HEIGHT / 2);
			
			/*	Target reached ?:	*/
			if( xval == get_finish_x() && yval == get_finish_y() ) {
				setActive( false );
			}
			
		};
		int get_pos_x() { return xval; };
		int get_pos_y() { return yval; };
		void set_max_frames( Uint32 a ) { max_frames = a; };
		void animate();
		void move();
		bool isIdle() { return idle; };
		void setIdle( bool state ) { idle = state; };
};

/*	
	=====================
	THE CAMERA STRUCT:
	=====================
		*/
typedef struct camera {
	int offset_x;
	int offset_y;
	int offset_dx;
	int offset_dy;
} camera;



/*	
	================
	GLOBAL STUFF:
	================
		*/
Map gamemap;
camera cam;		
		
Unit player;
Unit ghost;
Entity grid;
Entity obstacle;

Entity highlighted_grid;
Entity closed_grid;
Entity target_marker;
Entity path_marker;


/*	
	============
	MAIN:
	============
		*/
int main( void )
{
	/*	The window we'll be rendering to:	*/
	SDL_Window *window = NULL;
	/*	The surface contained by the window:	*/
	SDL_Surface *screenSurface = NULL;
	/*	The main renderer:	*/
	SDL_Renderer *renderer = NULL;
	

	if( init(&window, &renderer, &screenSurface ) != 0 ){
		cout << "INIT FAILED !\n";
		exit(EXIT_FAILURE);
	}
	
	gameloop( renderer );
	
	/*	Destroy the renderer:	*/
	SDL_DestroyRenderer(renderer);
	/*	Destroy window:	*/
	SDL_DestroyWindow( window );
	/*	Quit SDL subsystems:	*/
	SDL_Quit();

	return 0;
}


/*	
	============
	INIT:
	============
		*/
int init( SDL_Window **window, SDL_Renderer **renderer, SDL_Surface **screenSurface ) {

	/*	Initialize SDL:	*/
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << "\n";
		exit(EXIT_FAILURE);
	}
	
	/*	Initialize SDL_Image:	*/
	int imgFlags = IMG_INIT_PNG;
	if( !( IMG_Init( imgFlags ) & imgFlags ) ) {
		cerr << "\n\n\tERROR INITIALIZING SDL_image :\n\t" << IMG_GetError() << "\n\n";
		exit(EXIT_FAILURE);
	}
	
	/*	Create window:	*/
	(*window) = SDL_CreateWindow( "ISOMETRIC", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL );
	if( (*window) == NULL ) {
		cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << "\n";
		exit(EXIT_FAILURE);
	}
	
	/*	Create the renderer:	*/
	*renderer = SDL_CreateRenderer( *window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE );
	if (*renderer == NULL) {
		cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << "\n";
		exit(EXIT_FAILURE);
	}	
	SDL_SetRenderDrawColor( *renderer, 0, 0, 0, 0xFF );
	
	/*	Init the cam:	*/
	cam.offset_x = 0;
	cam.offset_y = 0;
	cam.offset_dx = 0;
	cam.offset_dy = 0;
	
	/*	Get window surface:	*/
	*screenSurface = SDL_GetWindowSurface( *window );
	
	/*	Load map:	*/
	gamemap.load_map( "gamefile.txt" );
	
	/*	Load player:	*/
	player.loadSurface( *renderer, "player.png" );
	player.set_dim( 64, 64 );
	player.set_max_frames( 8 );
	/*	Load ghost:	*/
	ghost.loadSurface( *renderer, "ghost_2.png" );
	ghost.set_dim( 64, 64 );
	ghost.set_max_frames( 6 );
	/*	Load grid:	*/
	grid.loadSurface( *renderer, "iso_grid_3.png" );
	/*	Load obstacle:	*/
	obstacle.loadSurface( *renderer, "iso_obstacle_2.png" );
	/*	Load highlighted_grid:	*/
	highlighted_grid.loadSurface( *renderer, "iso_highlighted_grid.png" );
	/*	Load target_marker:	*/
	target_marker.loadSurface( *renderer, "iso_target_marker.png" );
	/*	Load path_marker:	*/
	path_marker.loadSurface( *renderer, "iso_path_marker_2.png" );
	/*	Load closed_grid:	*/
	closed_grid.loadSurface( *renderer, "iso_closed_grid_2.png" );

	return 0;
}


/*	
	============
	GAMELOOP:
	============
		*/
int gameloop( SDL_Renderer *renderer ) {
	
	bool quit = false, key_pressed = false;
	int frames_drawn = 0;
	Uint32 start = 0;
	
	bool lvl_cng = true;
	
	//thread t1( test, 100000 );
	
	
	while( !quit ) {
		start = SDL_GetTicks();
	
		/*	CHECK THE INPUT QUEUE:	*/
		handle_input( &quit, &key_pressed, &lvl_cng );
		
		/*	ADJUST THE CAM:	*/
		cam.offset_x += cam.offset_dx;
		cam.offset_y += cam.offset_dy;
		if (cam.offset_x < -6*LEVEL_WIDTH) {
			cam.offset_x = -6*LEVEL_WIDTH;
		} else if (cam.offset_x > 3*LEVEL_WIDTH) {
			cam.offset_x = 3*LEVEL_WIDTH;
		}
		if (cam.offset_y < -6*LEVEL_HEIGHT) {
			cam.offset_y = -6*LEVEL_HEIGHT;
		} else if (cam.offset_y > 3*LEVEL_HEIGHT) {
			cam.offset_y = 3*LEVEL_HEIGHT;
		}
		
		
		
		/*	
			HANDLE LOGIC HERE:
		*/
		if ( !(player.isIdle()) ) {
			/*	CLEAN UP BEHIND THE LAST A-STAR CALL:	*/
			if ( *(player.get_ptr_to_closed()) != NULL) {
				delete_list( *(player.get_ptr_to_closed()) );
				*(player.get_ptr_to_closed()) = NULL;
			}
			
			/*	Astar call:	*/
			Astar( gamemap.get_matrix(), LEVEL_HEIGHT, LEVEL_WIDTH,
					player.get_start_x(), player.get_start_y(),
					player.get_finish_x(), player.get_finish_y(),
					player.get_ptr_to_path(), player.get_ptr_to_closed() );
			player.set_path_matrix();
			player.setActive( true );
			
			player.setIdle(true);
		}
		if ( !(ghost.isIdle()) ) {
			/*	CLEAN UP BEHIND THE LAST A-STAR CALL:	*/
			if ( *(ghost.get_ptr_to_closed()) != NULL) {
				delete_list( *(ghost.get_ptr_to_closed()) );
				*(ghost.get_ptr_to_closed()) = NULL;
			}
			
			/*	Astar for ghost:	*/
			Astar( gamemap.get_matrix(), LEVEL_HEIGHT, LEVEL_WIDTH,
					ghost.get_start_x(), ghost.get_start_y(),
					ghost.get_finish_x(), ghost.get_finish_y(),
					ghost.get_ptr_to_path(), ghost.get_ptr_to_closed() );
			ghost.set_path_matrix();
			ghost.setActive( true );
			
			ghost.setIdle(true);
			
		}
		
		/*	MOVE THE PLAYER:	*/
		player.move();
		/*	MOVE THE GHOST:	*/
		ghost.move();
		/*	LOGIC END.	*/



		/*	ANIMATIONS:	*/
		player.animate();
		ghost.animate();
		
		/*	REFRESH THE WINDOW:	*/
		draw( renderer );
		
		/*	HANDLE THE FPS:	*/
		frames_drawn++;
		/*	Sleep to keep the FPS as defined:	*/
		if( (1000 / FPS) > (SDL_GetTicks() - start)  ) {
			SDL_Delay( (1000 / FPS) - (SDL_GetTicks() - start) );
		}
	}
	
	//t1.join();
	
	return 0;
}


/*	
	============
	DRAW:
	============
		*/
int draw( SDL_Renderer *renderer ) {
	
	int tmpx, tmpy;
	int cursor_x, cursor_y;
	SDL_GetMouseState(&cursor_x, &cursor_y);
	cursor_x += cam.offset_x * TILE_WIDTH;
	cursor_y += cam.offset_y * TILE_WIDTH;
	iso_to_matrix( &cursor_x, &cursor_y );
	
	SDL_RenderClear( renderer );
	
	/*	Fill the surface white	*/
	//SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 255, 0, 255 ) );

	
	/*	Draw the map:	*/
	for ( int i = 0; i < LEVEL_HEIGHT; i += 1) {
		for ( int j = 0; j < LEVEL_WIDTH; j += 1) {
			if ( gamemap.tileType( i, j ) == 0 ) {
				if (player.tile_closed(j, i)) {
					tmpx = j;
					tmpy = i;
					matrix_to_iso( &tmpx, &tmpy );
					closed_grid.render( renderer, tmpx, tmpy );
				} else {
				/*	Draw the grid:	*/
				grid.render( renderer, ( j - i ) * (TILE_WIDTH / 2) - (TILE_WIDTH/2),
										( j + i ) * (TILE_HEIGHT / 2));
				}
			}
			/*	Draw the highlighted_grid:	*/
			if( cursor_x == j && cursor_y == i ) {
				matrix_to_iso( &cursor_x, &cursor_y );
				highlighted_grid.render( renderer, cursor_x, cursor_y);
			}
			/*	Draw the path:	*/
			if( player.tile_on_path(j, i) ) {
				tmpx = j;
				tmpy = i;
				matrix_to_iso( &tmpx, &tmpy );
				path_marker.render( renderer, tmpx, tmpy );
			}
			/*	Draw the Obstacles:	*/
			if( gamemap.tileType( i, j ) == 1 ) {
				obstacle.render( renderer, ( j - i ) * (TILE_WIDTH / 2) - (TILE_WIDTH/2),
										( j + i ) * (TILE_HEIGHT / 2) - 32);
			}
			/*	Draw the finish point:	*/
			if ( player.get_finish_x() == j && player.get_finish_y() == i ) {
				tmpx = j;
				tmpy = i;
				matrix_to_iso( &tmpx, &tmpy );
				target_marker.render( renderer, tmpx, tmpy );
			}
			/*	Draw the ghost:	*/
			if( ghost.get_pos_x() == j && ghost.get_pos_y() == i ) {
				ghost.render( renderer );
			}
			/*	Draw the player:	*/
			if( player.get_pos_x() == j && player.get_pos_y() == i ) {
				player.render( renderer );
			}
		}
	}
	
	SDL_RenderPresent( renderer );
	
	return 0;
}


/*	
	=========================
	HANDLE THE INPUT QUEUE:
	=========================
		*/
void handle_input( bool *quit, bool *key_pressed, bool *lvl_cng ) {
	SDL_Event event;
	int x = 0, y = 0;
	float i = 0,j = 0;
	
	while( SDL_PollEvent(&event) != 0 ) {
		switch (event.type) {
			case SDL_QUIT:
				*quit = true;
				break;
			case SDL_KEYDOWN:	/*	KEY PRESSED:	*/
				*key_pressed = true;
				switch (event.key.keysym.sym) {
					case SDLK_ESCAPE:
						*quit = true;
						break;
					case SDLK_a:
						cam.offset_dx = -1;
						break;
					case SDLK_d:
						cam.offset_dx = 1;
						break;
					case SDLK_w:
						cam.offset_dy = -1;
						break;
					case SDLK_s:
						cam.offset_dy = 1;
						break;
					default:
						break;
				}
				break;
			case SDL_KEYUP:	/*	KEY RELEASED:	*/
				*key_pressed = false;
				switch( event.key.keysym.sym ){
					case SDLK_a:
						if(cam.offset_dx < 0 && !(*key_pressed))
							cam.offset_dx = 0;
						break;
					case SDLK_d:
						if(cam.offset_dx > 0 && !(*key_pressed))
							cam.offset_dx = 0;
						break;
					case SDLK_w:
						if(cam.offset_dy < 0 && !(*key_pressed))
							cam.offset_dy = 0;
						break;
					case SDLK_s:
						if(cam.offset_dy > 0 && !(*key_pressed))
							cam.offset_dy = 0;
						break;
					default:
						break;
				}
				break;
			case SDL_MOUSEBUTTONDOWN:	/*	MOUSE BUTTON PRESSED	*/
				x = (event.button.x) + cam.offset_x * TILE_WIDTH;
				y = (event.button.y) + cam.offset_y * TILE_WIDTH;
				if( x <= LEVEL_WIDTH * TILE_WIDTH && y <= LEVEL_HEIGHT * TILE_HEIGHT ) {
					if( event.button.button == 1 ) {	/*	LMB	*/
						i = x;
						j = y;
						x = (i / (TILE_WIDTH/2) + j / (TILE_HEIGHT/2)) / 2;
						y = (j / (TILE_HEIGHT/2) - i / (TILE_WIDTH/2)) / 2;
						/*	Is there any unit at cursor position ?:	*/
						if( gamemap.isPassable(x,y) ){
							if ( player.get_pos_y() != y || player.get_pos_x() != x ) {
								player.set_pos( x, y );
								player.set_start( x, y );
								player.setIdle(false);
							}
						}
					} else if( event.button.button == 3 ) {	/*	RMB:	*/
						i = x;
						j = y;
						x = (i / (TILE_WIDTH/2) + j / (TILE_HEIGHT/2)) / 2;
						y = (j / (TILE_HEIGHT/2) - i / (TILE_WIDTH/2)) / 2;
						if( gamemap.isPassable(x,y) ){
							if ( player.get_finish_y() != y || player.get_finish_x() != x ) {
								player.set_finish( x, y );
								player.setIdle(false);
							}
						}
					}
				}
				break;
			default:
				break;
					
		}
	}
}


/*	
	=========================
	LOAD THE MAP FROM FILE:
	=========================
		*/
void Map::load_map( std::string path ) {
	ifstream file( path.c_str() );
	
	if( !(file.is_open()) ) {
		cerr << "\n\n\tERROR OPENING " << path << ":\n\n";
		exit(EXIT_FAILURE);
	}
	/*	Read the dimensions of saved level:	*/
	file >> gamemap.width;
	file >> gamemap.height;
	
	/*	Allocate memory for the matrix:	*/
	gamemap.matrix = new int *[height];
	for ( int i = 0; i < height; i += 1 ) {
		gamemap.matrix[i] = new int [width];
	}
	
	/*	Read the matrix which represents the tile types of loaded level:	*/
	for ( int i = 0; i < height; i += 1 ) {
		for ( int j = 0; j < width; j += 1 ) {
			if ( file.eof() ) {
				cerr << "\n\n\tERROR READING FROM FILE " << path << ":\n\t" << "MATRIX TOO SMALL !" << "\n\n";
			}
			file >> gamemap.matrix[i][j];
		}
	}
	
	file.close();

}


/*	
	==============================
	DESTRUCTOR FOR THE CLASS MAP:
	==============================
		*/
Map::~Map(){
	for ( int i = 0; i < height; i += 1) {
		delete [] matrix[i];
	}
	delete [] matrix;

}


/*	
	==============================
	LOAD AN IMAGE FOR THE ENTITY:
	==============================
		*/
void Entity::loadSurface( SDL_Renderer *renderer, std::string path ) {
	
	SDL_Texture *opt_texture = NULL;
	SDL_Surface *temp = IMG_Load( path.c_str() );
	
	if (temp == NULL ) {
		cerr << "\n\n\tERROR LOADING " << path << ":\n\t" << IMG_GetError() << "\n\n";
		exit(EXIT_FAILURE);
	}
	
	/*	Set the width / height of the Entity:	*/
	w = temp->w;
	h = temp->h;
	
	opt_texture = SDL_CreateTextureFromSurface( renderer, temp );

    if ( opt_texture == NULL) {
		cerr << "\n\n\tERROR CREATING TEXTURE FROM SURFACE " << path << ":\n\t" << SDL_GetError() << "\n\n";
		exit(EXIT_FAILURE);
	}
	/*	Free the temp surface:	*/
	SDL_FreeSurface(temp);
	
	texture = opt_texture;
}


/*	
	======================================
	RENDER THE ENTITY ONTO THE BACKGROUND:
	======================================
		*/
void Entity::render( SDL_Renderer *renderer, int x, int y ) {
	SDL_Rect src, dest;
	
	/*	Set the coordinates:	*/
	src.x = 0;
	src.y = 0;
	src.w = w;
	src.h = h;
	dest.x = x - cam.offset_x * TILE_WIDTH ;
	dest.y = y - cam.offset_y * TILE_WIDTH ;
	dest.w = w;
	dest.h = h;
	
	/*	Draw the entity:	*/
	SDL_RenderCopy( renderer, texture, &src, &dest );
}

/*	
	======================================
	RENDER THE UNIT ONTO THE BACKGROUND:
	======================================
		*/
void Unit::render( SDL_Renderer *renderer ){
	SDL_Rect src, dest;
	
	/*	Set the coordinates:	*/
	src.x = 0 + w * cur_frame;
	src.y = 0;
	src.w = w;
	src.h = h;
	dest.x = px - cam.offset_x * TILE_WIDTH - TILE_WIDTH/2;
	dest.y = py - cam.offset_y * TILE_WIDTH - 36;
	dest.w = w;
	dest.h = h;
	
	/*	Draw the entity:	*/
	SDL_RenderCopy( renderer, texture, &src, &dest );
}

/*	
	======================================
	ANIMATE A UNIT:
	======================================
		*/
void Unit::animate() {
	
	Uint32 cur_time = SDL_GetTicks();
	
	if ( cur_time - last_frame_chg > 100 ) {
		cur_frame++;
		if( cur_frame > max_frames-1 ) {
			cur_frame = 0;
		}
		last_frame_chg = cur_time;
	}
	
	
}

/*	
	======================================
	MOVE A UNIT:
	======================================
		*/
void Unit::move() {

	if ( isActive() ) {
		Uint32 cur_time = SDL_GetTicks();
		if ( cur_time - last_move > 500 ) {
			node **temp = get_ptr_to_path();
			*temp = (*temp)->next;
			if (*temp != NULL) 
				set_pos( (*temp)->x, (*temp)->y );
			last_move = cur_time;
		}
	}
}


/*	
	======================================
	CALCULATE POSITION ON SCREEN (IN PX):
	======================================
		*/
void matrix_to_iso( int *x, int *y ) {
	int a, b;
	
	a = ( *x - *y ) * (TILE_WIDTH / 2)  - (TILE_WIDTH/2);
	b = ( *x + *y ) * (TILE_HEIGHT / 2);
	
	*x = a;
	*y = b;
}


/*	
	======================================
	CALCULATE POSITION IN MATRIX (INDICES):
	======================================
		*/
void iso_to_matrix( int *x, int *y ) {
	float i, j;
	
	i=*x; j=*y;
	
	*x = (i / (TILE_WIDTH/2) + j / (TILE_HEIGHT/2)) / 2;
	*y = (j / (TILE_HEIGHT/2) - i / (TILE_WIDTH/2)) / 2;
	
}
