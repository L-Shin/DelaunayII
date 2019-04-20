#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "predicates.h"

//Data structures//

// Point will store vertex coordinates, but each quad-edge will have its OWN two Point fields, quad-edges that share a vertex will NOT reference the same Point objects, but the two edges in the same quad-edge WILL reference the same point fields if appropriate. This makes updating Org/Dest easy. 
struct Point {
	double x;
	double y;
};

typedef struct Point Point; 

//TODO: figure out if this is ok fp arithmetic
int equal(Point *p1, Point *p2) 
{
	return (p1->x==p2->x) && (p1->y==p2->y);
}
// Return 1 if (p1, p2, p3) CCW, 0 if collinear, -1 if clockwise
int ccw(Point *p1, Point *p2, Point *p3) 
{

	double ap1[2] = {p1->x, p1->y};
	double ap2[2] = {p2->x, p2->y};
	double ap3[3] = {p3->x, p3->y};
	double o2d = orient2d(ap1, ap2, ap3);
	if (o2d < 0) return -1;
	else if (o2d == 0) return 0;
	else return 1;
}

// Return 1 if p4 lies inside p1p2p3, 0 if cocircular, -1 if p4 lies outside p1p2p3
int in_circle(Point *p1, Point *p2, Point *p3, Point *p4)
{
	double ap1[2] = {p1->x, p1->y};
	double ap2[2] = {p2->x, p2->y};
	double ap3[2] = {p3->x, p3->y};
	double ap4[2] = {p4->x, p4->y};
	return incircle(ap1, ap2, ap3, ap4) > 0;
} 
// Utility for parsing command line, not important
typedef struct {
	int randomized;
	int fast;
	char *in_filename;
	char *out_filename;
} Input;


// Quad-edge will be 4 Edge structures pointing to each other. Two will correspond to an actual edge in the triangulation, and two their duals
typedef struct Edge Edge;

struct Edge {
	Point *Org;
	Point *Dest;
	Edge *Next;
	Edge *Rot;
};


//Basic edge operations//

Edge *rot(Edge *e) 
	{return e->Rot;}
Edge *onext(Edge *e) 
	{return e->Next;}
Edge *sym(Edge *e) 
	{return e->Rot->Rot;}
Edge *rot_inv(Edge *e) 
	{return e->Rot->Rot->Rot;}
Edge *oprev(Edge *e)
	{return rot(onext(rot(e)));}
Edge *lprev(Edge *e)
	{return sym(onext(e));}
Edge *lnext(Edge *e) 
	{return rot_inv(onext(rot(e)));}
Edge *dprev(Edge *e)
	{return rot_inv(onext(rot_inv(e)));}
Edge *make_edge()
{
	Edge *e1 = malloc(sizeof(Edge)); Edge *e2 = malloc(sizeof(Edge));
	Edge *e3 = malloc(sizeof(Edge)); Edge *e4 = malloc(sizeof(Edge));
	Point *org = malloc(sizeof(Point));
	Point *dest = malloc(sizeof(Point));	
	e1->Rot = e2; e2->Rot = e3; e3->Rot = e4; e4->Rot = e1;
	e1->Next = e1; e3->Next = e3;
	e2->Next = e4; e4->Next = e2;
	e1->Org = org; e1->Dest = dest;
	e3->Org = dest; e3->Dest = org;
	return e1;
}
//Used to update the Org/Dest of e and sym(e) simultaneously
void set_equal(Point *p1, Point *p2)
{
	p1->x = p2->x;
	p1->y = p2->y;
}

//splice edges
void splice(Edge *a, Edge *b) 
{
	Edge *alpha = rot(onext(a));
	Edge *beta = rot(onext(b));
	Edge *bOnext = onext(b); Edge *aOnext = onext(a);
	Edge *betaOnext = onext(beta); Edge *alphaOnext = onext(alpha);
	
	a->Next = bOnext; b->Next = aOnext;
	alpha->Next = betaOnext; beta->Next = alphaOnext;
	rot(a)->Next = beta; rot(b)->Next = alpha;
	rot(alpha)->Next = b; rot(beta)->Next = a;
}

void delete_edge(Edge *e) 
{
	splice(e, oprev(e));
	splice(sym(e), oprev(sym(e)));
	free(e->Org); free(e->Dest);
	
	for (int i = 0; i < 3; i++) {
		Edge *prev = e; e = rot(e);
		free(prev);
	}
	free(e);
}

void swap(Edge *e) 
{
	Edge *a = oprev(e);
	Edge *b = oprev(sym(e));
	splice(e, a); splice(sym(e), b);
	splice(e, lnext(e)); splice(sym(e), lnext(b));
	set_equal(e->Org, a->Dest); set_equal(e->Dest, b->Dest);
}

// p = alpha*org + (1-alpha)*dest <-> alpha = (p - dest)/(org - dest)
int on(Point *p, Edge *e)
{
	double point[2] = {p->x, p->y};
	double p1[2] = {e->Org->x, e->Org->y};
	double p2[2] = {e->Dest->x, e->Dest->y};
	return (orient2d(point, p1, p2) == 0) && (((point[0] <= p1[0]) || (point[0] <= p2[0])) && ((point[0] >= p1[0]) || (point[0] >= p2[0])));
}
int right_of(Point *p, Edge *e) 
{
	return ccw(e->Dest, e->Org, p) == 1;
}
int left_of(Point *p, Edge *e)
{
	return ccw(e->Org, e->Dest, p) == -1;
}

Edge *connect(Edge *a, Edge *b) 
{
	Edge *e = make_edge();
	set_equal(e->Org, a->Dest);
	set_equal(e->Dest, b->Org);
	splice(e, lnext(a));
	splice(sym(e), b);
	return e;
}

Edge *locate(Point *x, Edge *e)
{
	do {
		if (on(x, e)) return e;
		else if (right_of(x, e)) e = sym(e);
		else if (left_of(x, onext(e))) e = onext(e);
		else if (left_of(x, dprev(e))) e = dprev(e);
		else {
			if (on(x, onext(e))) return onext(e);
			else if (on(x, dprev(e))) return dprev(e);
			else return e;
		}
	} while(1);
}

void insert_site(Point *x, Edge *existing)
{
	Edge *e = locate(x, existing);
	if (equal(x, e->Org) || equal(x, e->Dest)) return;
	else if (on(x, e)) {
		Edge *t = oprev(e);
		delete_edge(e);
		e = t;
	}
	Edge *base = make_edge();
	Point *first = e->Org;
	set_equal(base->Org, first);
	set_equal(base->Dest, x);
	splice(base, e);
	do {
		base = connect(e, sym(base));
		e = oprev(base);
	} while (!equal(e->Dest, first));
	e = oprev(base);
	do {
		Edge *t = oprev(e);
		if (right_of(t->Dest, e) && in_circle(e->Org, t->Dest, e->Dest, x)) {
			swap(e); 
			e = oprev(e);
		} else if (equal(e->Org, first)) return;
		else e = lprev(onext(e));
	} while(1);
}

//read command line arguments

int read_input(int argc, char *argv[], Input *input_args)
{
	int opt;
	opterr = 0;

	while((opt = getopt(argc, argv, "rfi:o:")) != -1)
	{
		switch(opt)
		{
			case 'r':
				input_args->randomized = 1;
				break;
			case 'f':
				input_args->fast = 1;
				break;
			case 'i':
				input_args->in_filename =optarg;
				break;
			case 'o':
				input_args->out_filename = optarg;
				break;
			case '?':
				if (optopt == 'i')
					fprintf(stderr, "Please specify input filename\n");
				else if (isprint (optopt))
					fprintf(stderr, "Unknown option '-%c'\n",optopt);
				else 
					fprintf(stderr, "Unknown option character '\\x%x'\n", optopt);
				return 1;
			default:
				abort();
		}
	}
	if (!input_args->in_filename) {
		fprintf(stderr, "Please specify input filename\n");
		return 1;
 	}	
	if (!input_args->out_filename) {
		input_args->out_filename = "output.ele";
	}
	printf("randomized insertion = %d\nfast point location = %d\ninput filename = %s\noutput_filename = %s\n", input_args->randomized, input_args->fast, input_args->in_filename, input_args->out_filename);
 
	for(; optind < argc; optind++){
		printf("extra argument: %s\n", argv[optind]);
	}
	return 0;
} 

//read input file (.node)

int read_node(char *filename) 
{
	FILE *file;
	file = fopen(filename, "r");
	return 0;
}

int main(int argc, char *argv[])
{
	exactinit();
	Input input_args = {0,0,NULL,NULL};
	int error = read_input(argc, argv, &input_args);
	if (error) return 1;
	
	return 0;
}

