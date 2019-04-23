#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#ifndef TRIANGLE_H_
#define TRIANGLE_H_

typedef struct Node Node;
typedef struct Edge Edge;
typedef struct Point Point;
struct Node {
	int type; // 0 for edge, 1 for face

	Node *left; // nodes to the left/right by comparison with edge org -> dest 
	Node *right; 

	Edge *edge; // pointer to an edge in the triangulation 
		    // such that this face lies to the left (if this node is a face)

	Point *Org; // coordinates of origin and destination of corresponding edge in triangulation
	Point *Dest; // to be used if node is an edge node

	Node ***pointers; // addresses of the (only!) <=2 node->left or node->right
	                  // pointers that point to this face, used for updates 
	int num_pointers; // number of pointers (0 for edge nodes, 1 or 2 for faces)
};
	
struct Point {
	double x;
	double y;
	int id; // negative id indicates symbolic point, see Dutch book. 
	        // lexico. max point is aliased as 0
		// all other points have ids that indicate their (1-indexed) 
		// location in the .node input file
};
int symbolic(Point *p);
struct Edge {
	Point *Org; 
	Point *Dest;
	Edge *Next; // next ccw edge with the same left face
	Edge *Rot; // edge in quadedge obtained by 90 deg ccw rotation
	int trav; // boolean indicates whether this edge's left face has been 
		  // recorded in the .ele output file
	Node *face; // pointer to the left face node in the DAG

};

void delete_edge(Edge *e);

void initialize(Edge *a, Node **root);

Edge *extend_on(Point *p, Edge *e);

Edge *extend_in(Point *p, Edge *e);

void flip_location(Edge *e);

Edge *find(Point *p, Node *n);

Edge *prepare_in(Point *x, Edge *e); 

Edge *prepare_on(Point *x, Edge *e, Edge *t); 

Node *left_face(Edge *e);

int ccw(Point *p1, Point *p2, Point *p3);

Edge *sym(Edge *e);

Edge *lnext(Edge *e);

Edge *lprev(Edge *e);

Edge *oprev(Edge *e);

void swap(Edge *e);

void swap_location(Edge *e);

Node *left_face(Edge *e);

void set_left_face(Edge *e, Node *f);

void set_equal(Point *p1, Point *p2);

int on(Point *p, Edge *e);

void free_nodes(Node **root);
// data structure for queue
typedef struct node {
	Edge *val;
	struct node *next;
	struct node *prev;
} node_t;

void enqueue(node_t **head, Edge *val);

Edge *dequeue(node_t **head);

void shuffle(int *array, int length); 
 
int min(int i, int j);

// Utility for parsing command line
typedef struct {
	int randomized;
	int fast;
	char *in_filename;
	char *out_filename;
} command_line;
// Utility for reading the input .node file
typedef struct {
	int num_points;
	double *point_list;
} read_io;

char *readline(char *string, FILE *infile);
char *findfield(char *string);
int file_readnodes(FILE *file, int *firstnode, read_io *io);
int file_writenodes(FILE *file, Edge *e, read_io *io, int first_node, int max_index);
int read_input(int argc, char *argv[], command_line *input_args);
#endif // TRIANGLE_H_
