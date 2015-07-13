#include <iostream>

using namespace std;

/*	
	=================================================
	NEAREST NEIGHBOR:
	=================================================
	Attempts to find an optimal solution for the TSP
	given through the tasks that the ghost has.
		*/
int nearest_neighbor( int **map, const int map_height, const int map_width, int start_pos_x, int start_pos_y, Task *task_queue ) {
	
	node *closed_list = NULL;
	
	/*	Allocate the adjacency matrix, which will hold the lengths
		of the shortest paths between each 2 of the tasks and
		the length of the shortest path from the starting point to
		the task itself (on the main diagonal) :	*/
	int **adj_matrix = new int*[num_tasks];
	for ( int i = 0; i < num_tasks; i += 1) {
		adj_matrix[i] = new int[num_tasks];
	}
	
	/*	Allocate the list which will hold all the shortest
		paths:
		First it will have the following format
		(given num_tasks == 5):
		
		i=1: 5 4 3 2 1
		i=2: 5 4 3 2
		i=3: 5 4 3
		i=4: 5 4
		i=5: 5
		
		When these paths are calculated,
		the missing ones will be acquired be reversing
		the appropriate ones [ e.g. 5->1 = (-1)* 1->5 ].
		
			*/
		node ***path_matrix = new node**[num_tasks];
		for ( int i = 0; i < num_tasks; i += 1) {
			path_matrix[i] = new node*[num_tasks]
		}
	
	/*	Find the shortest paths between every pair of 2 tasks
		(more or less, see above comment), this yields
		an adjacency matrix:	*/
	for ( int i = 0; i < num_tasks; i += 1) {
		state = Astar( map, map_height, map_width, int start_pos_x, int start_pos_y, int finish_pos_x, int finish_pos_y, node **path, &closed_list );
		
		if (state == -1) {
			cout << "\n\t" << "COULD NOT FIND A PATH BETWEEN " << start_pos_x << ", " << start_pos_y <<
				" AND " << finish_pos_x << ", " << finish_pos_y << " !\n\n";
		}
		delete_list( closed_list );
	}
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	/*	CLEAN UP:	*/
	for ( int i = 0; i < num_tasks; i += 1) {
		delete [] adj_matrix[i];
	}
	delete [] adj_matrix;

	return 0;
}
