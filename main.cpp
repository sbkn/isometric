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
		int tileType( int i, int j ) { return matrix[j][i]; };
		void setTileType( int xval, int yval, int new_type ) { matrix[xval][yval] = new_type; };
		bool isPassable( int i, int j ) { if(matrix[j][i] != 1){ return true;} else {return false;} };
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
		void clearClosedList() {
			node *tmp = closed_list;
			node *tmp2 = NULL;
	
			while(tmp != NULL){
				tmp2 = tmp;
				tmp = tmp->next;
				delete tmp2;
			}
			closed_list = NULL;
		};
		bool tile_closed(int i, int j) {  if (path_matrix[j][i] == 1) return true; else return false; };
		bool tile_on_path(int i, int j) { if (path_matrix[j][i] == 2) return true; else return false; };
		bool isActive() { return active; };
		void setActive( bool a ) { active = a; };
};




/*	
	==============================================================
	BASE CLASS FOR BASICALLY EVERYTHING THE USER SEES ON SCREEN:
	==============================================================
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
		Uint32 get_w() { return w; };
		Uint32 get_h() { return h; };
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
	int facing_x;	/*	+1 OR -1 	*/
	int facing_y;
	
	/*	frame indices:	*/
	Uint32 sprite_sheet_row;
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
			facing_x = -1;
			facing_y = 1;	/*	-> look bottom left	*/
		};
		Unit( int a, int b, int width, int height, int frames ) {
			idle = true;
			setActive(false);
			facing_x = -1;
			facing_y = 1;	/*	-> look bottom left	*/
			set_pos( a, b );
			set_dim( width, height );
			set_max_frames(frames);
			set_start( a, b );
			
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
			
			/*	Update the Route:	*/
			set_start( xval, yval );
			
		};
		int get_pos_x() { return xval; };
		int get_pos_y() { return yval; };
		void set_max_frames( Uint32 a ) { max_frames = a; };
		void animate();
		void move();
		bool isIdle() { return idle; };
		void setIdle( bool state ) { idle = state; };
		void turn( int a, int b ) { facing_x = a; facing_y = b; };
		void get_facing_dir( int *a, int *b ) { *a = facing_x; *b = facing_y; };
};


/*	
	===================
	THE BUTTON CLASS:
	===================
		*/
class Button : public Entity {
	/*	Position of the button (from the top left corner):	*/
	int px;
	int py;
	/*	This is the offset from the top left corner:	*/
	int offset_x;
	int offset_y;
	
	bool active;

	public:
		Button() { px = 0; py = 0; offset_x = 10; offset_y = 10; active = false; };
		void render( SDL_Renderer *renderer, int x, int y );
		void set_pos( int a, int b ) { px = a; py = b; offset_x = a; offset_y = b; };
		int get_pos_x() { return px; };
		int get_pos_y() { return py; };
		bool isActive() { return active; };
		void setActive( bool state ) { active = state; };
		bool cursor_on_button( int a, int b );
};


/*	
	===================
	THE TASK CLASS:
	===================
		*/
class Task {
	/*	Where does the unit has to go in order to
		work at this task?:	*/
	int xval;
	int yval;
	
	/*	Total number of tasks allocated atm:	*/
	static int num_tasks;
	
	/*	The index will be referring to the type
		of the task:	*/
	int index;
	
	/*	How to get to this place ?:	*/
	node *path_to_task;
	
	/*	Ptr to the next task in queue (if there is one):	*/
	Task *next_task;
	
	public:
		Task() { xval = 0; yval = 0; index = 0; path_to_task = NULL; next_task = NULL; num_tasks++; };
		~Task() { num_tasks--; };
		void set_pos( int a, int b ) { xval = a; yval = b; };
		int get_pos_x() { return xval; };
		int get_pos_y() { return yval; };
		/*	These 2 can be used to alter the path_to_task and next_task variables !:	*/
		node **get_path_to_task() { return &path_to_task; };
		void set_next_task( Task *a ) { next_task = a; };
		Task *get_next_task() { if (next_task != NULL) { return next_task; } else return NULL; };
		int get_num_tasks() { return num_tasks; };

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
/*	Init number of tasks (static member in Task):	*/
int Task::num_tasks = 0;		

Map gamemap;
camera cam;		
		
Unit player( 2, 2, 64, 64, 8 );
Unit ghost;
Unit *selected_unit;
Entity grid;
Entity obstacle;
Entity box_blueprint;

Entity highlighted_grid;
Entity closed_grid;
Entity target_marker;
Entity path_marker;

/*	This is the button:	*/
Button UI_build_box;
/*	This is the todo list (task queue):	*/
Task *task_queue;


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
	
	/*	Load ghost:	*/
	ghost.loadSurface( *renderer, "ghost_2.png" );
	ghost.set_dim( 64, 64 );
	ghost.set_max_frames( 6 );
	/*	Load grid:	*/
	grid.loadSurface( *renderer, "iso_grid_spooky.png" );
	/*	Load obstacle:	*/
	obstacle.loadSurface( *renderer, "iso_obstacle_spooky_box_closed.png" );
	/*	Load the box blueprint:	*/
	box_blueprint.loadSurface( *renderer, "iso_obstacle_spooky_box_closed_blueprint.png" );
	/*	Load highlighted_grid:	*/
	highlighted_grid.loadSurface( *renderer, "iso_highlighted_grid.png" );
	/*	Load target_marker:	*/
	target_marker.loadSurface( *renderer, "iso_target_marker.png" );
	/*	Load path_marker:	*/
	path_marker.loadSurface( *renderer, "iso_path_marker_spooky.png" );
	/*	Load closed_grid:	*/
	closed_grid.loadSurface( *renderer, "iso_closed_grid_spooky.png" );
	/*	Load UI_build_box button:	*/
	UI_build_box.loadSurface( *renderer, "button1.png" );
	UI_build_box.set_dim( 64, 64 );
	UI_build_box.set_pos( 10, SCREEN_HEIGHT - UI_build_box.get_h() - 10 );
	
	/*	Init selected_unit:	*/
	selected_unit = &player;
	/*	Init the todo list:	*/
	task_queue = NULL;
	
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
	int state = 0;
	
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
			state = Astar( gamemap.get_matrix(), LEVEL_HEIGHT, LEVEL_WIDTH,
					player.get_start_x(), player.get_start_y(),
					player.get_finish_x(), player.get_finish_y(),
					player.get_ptr_to_path(), player.get_ptr_to_closed() );
			if (state != -1) {
				player.set_path_matrix();
				player.setActive( true );
			} else {
				/*	If no path found:	*/
				player.clearClosedList();
				player.set_path_matrix();
			}
			player.setIdle(true);
		}
		if ( !(ghost.isIdle()) ) {
			/*	CLEAN UP BEHIND THE LAST A-STAR CALL:	*/
			if ( *(ghost.get_ptr_to_closed()) != NULL) {
				delete_list( *(ghost.get_ptr_to_closed()) );
				*(ghost.get_ptr_to_closed()) = NULL;
			}
			
			/*	Astar for ghost:	*/
			state = Astar( gamemap.get_matrix(), LEVEL_HEIGHT, LEVEL_WIDTH,
					ghost.get_start_x(), ghost.get_start_y(),
					ghost.get_finish_x(), ghost.get_finish_y(),
					ghost.get_ptr_to_path(), ghost.get_ptr_to_closed() );
			if (state != -1) {
				ghost.set_path_matrix();
				ghost.setActive( true );
			} else {
				/*	If no path found:	*/
				ghost.clearClosedList();
				ghost.set_path_matrix();
			}
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
			if ( gamemap.tileType( j, i ) == 0 ) {
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
			/*	Draw the players path:	*/
			if( player.tile_on_path(j, i) ) {
				tmpx = j;
				tmpy = i;
				matrix_to_iso( &tmpx, &tmpy );
				path_marker.render( renderer, tmpx, tmpy );
			}
			/*	Draw the ghosts path:	*/
			if( ghost.tile_on_path(j, i) ) {
				tmpx = j;
				tmpy = i;
				matrix_to_iso( &tmpx, &tmpy );
				path_marker.render( renderer, tmpx, tmpy );
			}
			/*	Draw the Obstacles:	*/
			if( gamemap.tileType( j, i ) == 1 ) {
				obstacle.render( renderer, ( j - i ) * (TILE_WIDTH / 2) - (TILE_WIDTH/2),
										( j + i ) * (TILE_HEIGHT / 2) - 32);
			}
			/*	Draw the box blueprints:	*/
			if( gamemap.tileType( j, i ) == 7 ) {
				box_blueprint.render( renderer, ( j - i ) * (TILE_WIDTH / 2) - (TILE_WIDTH/2),
										( j + i ) * (TILE_HEIGHT / 2) - 32);
			}
			/*	Draw the players finish point:	*/
			if ( player.get_finish_x() == j && player.get_finish_y() == i ) {
				tmpx = j;
				tmpy = i;
				matrix_to_iso( &tmpx, &tmpy );
				target_marker.render( renderer, tmpx, tmpy );
			}
			/*	Draw the ghosts finish point:	*/
			if ( ghost.get_finish_x() == j && ghost.get_finish_y() == i ) {
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
	
	/*	Draw the button:	*/
	UI_build_box.render( renderer, UI_build_box.get_pos_x(), UI_build_box.get_pos_y() );
	
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
	Task *tmp_task = NULL;
	Task *tmp_task_pred = NULL;
	
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
				/*	Check if UI_button clicked:	*/
				if( UI_build_box.cursor_on_button( event.button.x, event.button.y ) ) {
					if (UI_build_box.isActive()) {
						UI_build_box.setActive(false);
					} else
						UI_build_box.setActive(true);
				}
				/*	If not:	*/
				else {
					x = (event.button.x) + cam.offset_x * TILE_WIDTH;
					y = (event.button.y) + cam.offset_y * TILE_WIDTH;
					i = x;
					j = y;
					x = (i / (TILE_WIDTH/2) + j / (TILE_HEIGHT/2)) / 2;
					y = (j / (TILE_HEIGHT/2) - i / (TILE_WIDTH/2)) / 2;
					if( x >= 0 && y >= 0 && x < LEVEL_WIDTH && y < LEVEL_HEIGHT ) {
						if( event.button.button == 1 ) {	/*	LMB	*/
							/*	Is there any unit at cursor position ?:	*/
							if( player.get_pos_x() == x && player.get_pos_y() == y ) {
								selected_unit = &player;
							} else if( ghost.get_pos_x() == x && ghost.get_pos_y() == y ) {
								selected_unit = &ghost;
							}
							/*	.. or does the user want to build something ?:	*/
							else if( gamemap.isPassable(x,y) && UI_build_box.isActive() ){
								if ( gamemap.tileType(x,y) != 7 ) {
									/*	Enqueue the a new task (building a box):	*/
									tmp_task = new Task;
									tmp_task->set_pos( x, y );
									tmp_task->set_next_task( task_queue );
									task_queue = tmp_task;
									gamemap.setTileType( y, x, 7 );
								} else {
									/*	Dequeue the task (DELETE !),
										tmp_task_pred is a ptr trailing
										right behind tmp_task:	*/
									tmp_task = task_queue;
									tmp_task_pred = task_queue;
									/*	If it's right the first task in queue:	*/
									if ( tmp_task->get_pos_x() == x && tmp_task->get_pos_y() == y ) {
										task_queue = tmp_task->get_next_task();
									} else {
										/*	Otherwise:	*/
										while ( !(tmp_task->get_pos_x() == x && tmp_task->get_pos_y() == y) ) {
											tmp_task_pred = tmp_task;
											tmp_task = tmp_task->get_next_task();
										}
										tmp_task_pred->set_next_task( tmp_task->get_next_task() );
									}
									delete tmp_task;
								
									gamemap.setTileType( y, x, 0 );
								}
							}
						} else if( event.button.button == 3 ) {	/*	RMB:	*/
							if( gamemap.isPassable(x,y) ){
								if ( (*selected_unit).get_finish_y() != y || (*selected_unit).get_finish_x() != x ) {
									(*selected_unit).set_finish( x, y );
									(*selected_unit).setIdle(false);
								}
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
	RENDER THE BUTTON ONTO THE BACKGROUND:
	======================================
		*/
void Button::render( SDL_Renderer *renderer, int x, int y ) {
	SDL_Rect src, dest;
	
	/*	Set the coordinates:	*/
	src.x = 0 + active * 64;
	src.y = 0;
	src.w = w;
	src.h = h;
	dest.x = x;
	dest.y = y;
	dest.w = w;
	dest.h = h;
	
	/*	Draw the entity:	*/
	SDL_RenderCopy( renderer, texture, &src, &dest );
}


/*	
	==================================================
	CHECK WHETHER (x,y) IS ON TOP OF THE BUTTON AREA:
	==================================================
		*/
bool Button::cursor_on_button( int a, int b ) {
	
	if ( a >= offset_x && a <= offset_x + (int)get_w() && b >= offset_y && b <= offset_y + (int)get_h() ) {
		return true;
	} else
		return false;
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
	src.y = 0 + h * sprite_sheet_row;
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
	
	int x = 0, y = 0;
	
	if ( cur_time - last_frame_chg > 100 ) {
		/*	Pick the right row, depending
			on the facing:	*/
		get_facing_dir( &x, &y );
		if ( x == 1 && y == 0 ) {
			sprite_sheet_row = 1;
		} else if( x == 0 && y == -1 ) {
			sprite_sheet_row = 2;
		} else if( x == -1 && y == 0 ) {
			sprite_sheet_row = 3;
		} else {
			sprite_sheet_row = 0;
		}
		/*	Go to the next animation frame:	*/
		cur_frame++;
		if( cur_frame > max_frames-1 ) {
			cur_frame = 0;
		}
		last_frame_chg = cur_time;
	}
	
	
}

/*	
	======================================
	MOVE A UNIT ALONG ITS PATH:
	======================================
		*/
void Unit::move() {
	int dx = get_pos_x(), dy = get_pos_y();

	if ( isActive() ) {
		Uint32 cur_time = SDL_GetTicks();
		if ( cur_time - last_move > 400 ) {
			node **temp = get_ptr_to_path();
			if (*temp != NULL) {
				/*	CARE !
					NEXT LINE REMOVES THE FIRST
					NODE IN PATH AND SETS PATH
					TO ITS SUCCESSOR !	*/
				*temp = (*temp)->next;
				if ( *temp != NULL ) {
					set_pos( (*temp)->x, (*temp)->y );
					/*	Update the path:	*/
					*(get_ptr_to_path()) = (*temp);
					set_path_matrix();						/*	-> SO UGLY ..	EMB-FUCKEN-ARASSING	*/
					/*	Turn the unit in the right
						direction for the next movement
						(if there is a step left to go):
						*/
					if ( (*temp)->next != NULL ) {
						node *help = (*temp)->next;
						dx = help->x - get_pos_x();
						dy = help->y - get_pos_y();
						turn( dx, dy );
					} else {		/*	IF THERE IS NO STEP LEFT AFTER THIS, SET FACING DIRECTION FOR THE CURRENT STEP:	*/
						dx = (*temp)->x - dx;
						dy = (*temp)->y - dy;
						turn( dx, dy );
					}
					last_move = cur_time;
				}
			}
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
