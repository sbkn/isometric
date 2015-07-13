#include<iostream>
#include<stdlib.h>
#include<math.h>

#include "Astar.h"

using namespace std;

/*	
	BY TURNING THIS ON (=TRUE) YOU TELL ASTAR
	TO SKIP DIAGONALLY ADJACENT NODES WHEN EXPANDING:
		*/
#define NO_DIAG_MOV false


/* A-STAR: */
int Astar( int **map, const int map_height, const int map_width, int start_pos_x, int start_pos_y, int finish_pos_x, int finish_pos_y, node **path, node **main_closed ) {
	int i, j;
	int ite_cnt = 1;
	int best_F = -1, cur_F = -1;
	int done = 0;
	
	node *open_list = NULL;
	node *closed_list = NULL;
	node *tmp = NULL, *tmp2 = NULL;
	node *current_node = NULL;
	node *cur = NULL, *best = NULL;
	node *way = NULL; /* UGLY BULLSHIT .. FUCK IT .. */
	
	
	/* FOR TESTING PURPOSES ONLY: */
	int **temp_map = NULL;
	temp_map = new int*[map_height];
	for( i = 0; i < map_height; i++ )
		temp_map[i] = new int[map_width];
	

	
	/* ADD STARTING POINT TO THE OPEN LIST: */
	tmp = new node;
	tmp->x = start_pos_x;
	tmp->y = start_pos_y;
	tmp->cost = 0;
	tmp->parent = NULL;
	tmp->next = open_list;
	open_list = tmp;
	
	current_node = tmp;
	
	
	/* MAIN LOOP: */
	while(  !done  ) {
		
		if( current_node->x == finish_pos_x && current_node->y == finish_pos_y ) {
			cout << "\n\nDESTINATION REACHED AFTER "  << ite_cnt << " ITERATIONS!\n\n";
			cout << "\nCUMULATED COST OF OPTIMAL PATH:\t" << current_node->cost << "\n\n";
			
			done = 1;
			
			tmp = current_node;
			while(tmp != NULL) {
						
				tmp2 = new node;
				tmp2->x = tmp->x;
				tmp2->y = tmp->y;
				tmp2->next = way;
				way = tmp2;
				
				
				tmp = tmp->parent;
			}
			cout << "\n";
			
			(*path) = way;
			
		}
		
	
		/* EXPAND CURRENT_NODE: */
		for( i = -1; i < 2; i++ ) {
			for( j = -1; j < 2; j++ ) {
				if( i == 0 && j == 0 )
					continue;
				if( current_node->x + i < 0 || current_node->x + i >= map_width || current_node->y + j < 0 || current_node->y + j >= map_height)
					continue;
				if( map[current_node->y + j][current_node->x + i] == 1 )
					continue;
				if( nodeInList(closed_list, current_node->x + i, current_node->y + j) != NULL )
					continue;
				/*	Skip diagonally adjacent nodes IF NO_DIAG_MOV == true:	*/
				if ( i != 0 && j != 0 && NO_DIAG_MOV)
					continue;
					/*	THIS IS FOR PSEUDO-NO_DIAG_MOV
						YOU SHALL NOT MOVE DIAGONALLY IF
						AN OBSTACLE IS ADJACENT TO current_node
						AND THIS NODE: 	*/
			
				/* check whether neighbor is already on open list
					and update it's costs accordingly: */
				tmp = nodeInList(open_list, current_node->x + i, current_node->y + j);
				if( tmp != NULL ) {
					if( i != 0 && j != 0 ) {		/* checking for diagonal vs horizontal / vertical step */
						if( tmp->cost > current_node->cost + 14 ) {
							tmp->cost = current_node->cost + 14;
							tmp->parent = current_node;
						}
					} else {
						if( tmp->cost > current_node->cost + 10 ) {
							tmp->cost = current_node->cost + 10;
							tmp->parent = current_node;
						}
					}
					
				} else { /* neighbor is neither in open nor in closed list: */
					tmp = new node;
					tmp->x = current_node->x + i;
					tmp->y = current_node->y +j;
					if( i != 0 && j != 0 ) {			/* if the adjacency is diagonal, the cost for stepping over is about 1.44 times higher than otherwise */
						tmp->cost = current_node->cost + 14;
					} else {							/* otherwise the step cost is 10 (for keeping it integer) */
						tmp->cost = current_node->cost + 10;
					}
					tmp->parent = current_node;
					tmp->next = open_list;
					open_list = tmp;
				}
			}
		}
	
	
	
		/* REMOVE CURRENT NODE FROM OPEN LIST: */
		tmp = open_list;
		tmp2 = open_list;
		while( tmp != current_node ) {
			tmp2 = tmp;
			tmp = tmp->next;
		}
		if( tmp2 == tmp ) { /* last element in list */
			open_list = tmp->next;
			
		} else if( tmp->next == NULL ) { /* first element in list */
			tmp2->next = NULL;
		} else { /* right in the middle */
			tmp2->next = tmp->next;
		}
		
		
	
		/* ADD CURRENT_NODE TO CLOSED LIST: */
		current_node->next = closed_list;
		closed_list = current_node;
		
		(*main_closed) = closed_list;
	
	
	
	
		/* PATH SCORING: */
	
		/* 
		F = G + H,
	
		where F is the score, G the cost to move from starting point to a given point on the grid
		and H the approximative cost to reach the destination ( f.e. Manhattan distance )	
		 */
	
		
		/*	if open_list is empty, there are no open nodes left, even though destination is not reached yet:	*/
		if( open_list == NULL ) {
			cout << "\n\n\tNO PATH FOUND !\n\n";
			(*path) = NULL;
			return 1;
			/*	CLEAN UP NEEDED !	*/
		}
		
		/* Find the node from the open list which has the lowest F: */
		cur = open_list;
		best = open_list;
		
		/* THE APPROXIMATION FOR H CAN BE DONE WITH DIRECT DISTANCE OR WITH MANHATTAN DISTANCE, IDENTICAL RESULTS, BUT DIFFERENT PERFORMANCES ! (BLACK SWANS EXIST !!!) */
		
		//best_F = cur->cost + sqrt((finish_pos_x - cur->x) * (finish_pos_x - cur->x) + (finish_pos_y - cur->y) * (finish_pos_y - cur->y));
		best_F = cur->cost + abs(finish_pos_x - cur->x) + abs(finish_pos_y - cur->y);
	
		while( cur != NULL ) {
		
			//cur_F = cur->cost + sqrt((finish_pos_x - cur->x) * (finish_pos_x - cur->x) + (finish_pos_y - cur->y) * (finish_pos_y - cur->y));
			cur_F = cur->cost + abs(finish_pos_x - cur->x) + abs(finish_pos_y - cur->y);
		
			if(best_F == -1)
				best_F = cur_F;
		
			if( cur_F <= best_F ) {
				best = cur;
				best_F = cur_F;
			
			}
			//printf( "\n\tcur_F:\t%d\tcur->x: %d\tcur->y: %d", cur_F, cur->x+1, cur->y+1 );
			cur = cur->next;
		}
		//printf( "\n\tBEST:\n\tbest_F:\t%d\tbest->x: %d\tbest->y: %d\n", best_F, best->x+1, best->y+1 );
	
		current_node = best;
		
	
		ite_cnt++;
	
	
	}
	
	//printmatrix(map);
	
	/* CLEAN UP: */
	delete_list(open_list);
	//delete_list(closed_list);
	
	
	return 0;
}


/* RETURN PTR TO ELEMENT IF ITS ON THE LIST ALREADY,
	RETURN NULL OTHERWISE: */
node* nodeInList( node *list, int x, int y ) {
	node *tmp = NULL;
	
	tmp = list;
	
	while( tmp != NULL ) {
		if( tmp->x == x ) {
			if(tmp->y == y) {
				return tmp;
			}
		}
		tmp = tmp->next;
	}
	return NULL;
}




void delete_list( node *list ) {
	node *tmp = NULL;
	node *tmp2 = NULL;
	
	tmp = list;
	while(tmp != NULL){
		tmp2 = tmp;
		tmp = tmp->next;
		delete tmp2;
	}
}

void printmatrix( int **map, int map_height, int map_width ) {
	int i, j;

	for( i = 0; i < map_height; i++ ) {
		for( j = 0; j < map_width; j++ ) {
			cout << "\t" << map[i][j];
		}
		cout << "\n\n";
	}
	cout << "\n";
}
