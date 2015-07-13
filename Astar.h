#ifndef _ASTAR_H_
#define _ASTAR_H_

typedef struct node {
	int x;
	int y;
	
	int cost;
	
	struct node *parent;
	struct node *next;
} node;



int Astar( int **map, const int map_height, const int map_width, int start_pos_x, int start_pos_y, int finish_pos_x, int finish_pos_y, node **path, node **main_closed );
node* nodeInList( node *list, int x, int y );
void delete_list( node *list );
void printmatrix( int **map, int map_height, int map_width );

#endif
