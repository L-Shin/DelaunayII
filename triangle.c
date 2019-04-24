#include "predicates.h"
#include <stdlib.h>
#include <stdio.h>
#include "triangle.h"




///////// POINT UTILS /////////////




// negative points (-1 and -2) are symbolic, see Dutch book
int symbolic(Point *p) 
{
	return (p->id < 0);
}

int real(Point *p) 
{
	return !symbolic(p);
}

//return if points identical by id comparison
int equal(Point *p1, Point *p2) 
{
	return p1->id == p2->id;
}

// returns 1 if p1>p2, 0 if equal, -1 if p1<p2
int lexico_comp(Point *p1, Point *p2) 
{	
	if (p1->id == -1) return -1; // p-1 is smaller than all points
	else if (p2->id == -1) return 1;

	else if (p1->id == -2) return 1; // p-2 is larger than all points
	else if (p2->id == -2) return -1;
	// both points real
	else if ((p1->x == p2->x) && (p1->y == p2->y)) return 0;
	else if ((p1->x > p2->x) || ((p1->x == p2->x) && (p1->y > p2->y)))
		return 1;
	else return -1;
}
int ccw_symbolic(Point *p1, Point *p2, Point *p3)
{
	if (p1->id == -2) {	
		if (p2->id == -1) return 0; // all points left of edge -2 ---> -1
		else return lexico_comp(p3, p2) < 0; // see Dutch book 
	} else if (p2->id == -2) return ccw_symbolic(p2, p3, p1); // move p2 to beginning to reduce cases
	else if (p3->id == -2) return ccw_symbolic(p3, p1, p2); // move p3 to beginning
	else if (p1->id == -1) return lexico_comp(p3, p2) > 0; 
	else if (p2->id == -1) return ccw_symbolic(p2, p3, p1); // move p2 to beginning
	else return ccw_symbolic(p3, p1, p2); // move p3 = -1 to beginning (some point must be symbolic)
}	
// Return 1 if (p1, p2, p3) CCW, 0 if collinear, -1 if clockwise
int ccw(Point *p1, Point *p2, Point *p3) 
{
	if (symbolic(p1) || symbolic(p2) || symbolic(p3)) return ccw_symbolic(p1, p2, p3);
	else {
		double ap1[2] = {p1->x, p1->y};
		double ap2[2] = {p2->x, p2->y};
		double ap3[3] = {p3->x, p3->y};
		// from Shewchuk's robust predicates
		double o2d = orient2d(ap1, ap2, ap3);
		if (o2d < 0) return -1;
		else if (o2d == 0) return 0;
		else return 1;
	}
}

// Return 1 if p4 lies inside p1p2p3, 0 if cocircular, -1 if p4 lies outside p1p2p3
// Should only be used for real points
int in_circle(Point *p1, Point *p2, Point *p3, Point *p4)
{
		
	double ap1[2] = {p1->x, p1->y};
	double ap2[2] = {p2->x, p2->y};
	double ap3[2] = {p3->x, p3->y};
	double ap4[2] = {p4->x, p4->y};
	// from Shewchuk's robust predicates
	if (incircle(ap1, ap2, ap3, ap4) > 0) {
		return 1;
	}
	else return 0;		 
} 

// set fields of P1 to equal fields of p2
// Used to update the Org/Dest of e and sym(e) simultaneously
// and to create alias edges in the DAG
void set_equal(Point *p1, Point *p2)
{
	p1->x = p2->x;
	p1->y = p2->y;
	p1->id = p2->id;
}





/////////// DAG UTILS CALLED FROM location.c ////////////




// retrieve node in DAG corresponding to e's left face
Node *left_face(Edge *e) 
{
	return e->face;
}
// update pointers to DAG for all edges that share e's left face
void set_left_face(Edge *e, Node *f)
{
	Edge *next = lnext(e);
	e->face = f;
	while(next != e) {
		next->face = f;
		next = lnext(next);
	}
}





///////////// BASIC QUAD-EDGE OPERATIONS ////////////





// rotate 90 deg ccw
Edge *rot(Edge *e) 
	{return e->Rot;}
// next edge ccw around origin
Edge *onext(Edge *e) 
	{return e->Next;}
// same edge but opposite direction
Edge *sym(Edge *e) 
	{return e->Rot->Rot;}
// rotate 90 deg clockwise
Edge *rot_inv(Edge *e) 
	{return e->Rot->Rot->Rot;}
// next edge clockwise around origin
Edge *oprev(Edge *e)
	{return rot(onext(rot(e)));}
// next edge clockwise around left face
Edge *lprev(Edge *e)
	{return sym(onext(e));}
// next edge ccw around left face
Edge *lnext(Edge *e) 
	{return rot(onext(rot_inv(e)));}
// next edge clockwise around destination point
Edge *dprev(Edge *e)
	{return rot_inv(onext(rot_inv(e)));}

// make a new edge and allocate space for its endpoints
// returns edge e. The following holds for e: 
// lnext(e1) = e3
// lnext(e3) = e1
// onext(e1) = e1
// onext(e3) = e3
// lnext(e2) = e2
// lnext(e4) = e4
// onext(e2) = e4
// onext(e4) = e2
// with e1 = e, e2 = rot(e), e3 = sym(e), e4 = rot_inv(e)
Edge *make_edge()
{
	Edge *e1 = malloc(sizeof(Edge)); Edge *e2 = malloc(sizeof(Edge));
	Edge *e3 = malloc(sizeof(Edge)); Edge *e4 = malloc(sizeof(Edge));
	Point *org = malloc(sizeof(Point));
	Point *dest = malloc(sizeof(Point));
	e1->trav = 0; e3->trav = 0;	
	e1->Rot = e2; e2->Rot = e3; e3->Rot = e4; e4->Rot = e1;
	e1->Next = e1; e3->Next = e3;
	e2->Next = e4; e4->Next = e2;
	e1->Org = org; e1->Dest = dest;
	e3->Org = dest; e3->Dest = org;
	return e1;
}

//splice edges. see Guibas/Stolfi paper
void splice(Edge *a, Edge *b) 
{
	Edge *alpha = rot(onext(a));
	Edge *beta = rot(onext(b));
	Edge *bOnext = onext(b); Edge *aOnext = onext(a);
	Edge *betaOnext = onext(beta); Edge *alphaOnext = onext(alpha);
	
	a->Next = bOnext; b->Next = aOnext;
	alpha->Next = betaOnext; beta->Next = alphaOnext;
}
// delete edge and free its quadedge data structure
void delete_edge(Edge *e) 
{
	Edge *a = oprev(e);
	Edge *b = oprev(sym(e));
	splice(e, a);
	splice(sym(e), b);
	free(e->Org); free(e->Dest);
	
	for (int i = 0; i < 3; i++) {
		Edge *prev = e; e = rot(e);
		free(prev);
	}
	free(e);
}
// switch triangulation of e's two adjacent faces
void swap(Edge *e) 
{
	Edge *a = oprev(e);
	Edge *b = oprev(sym(e));
	splice(e, a); splice(sym(e), b);
	splice(e, lnext(a)); splice(sym(e), lnext(b));
	set_equal(e->Org, a->Dest); set_equal(e->Dest, b->Dest);
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




///////// POINT INSERTION HELPERS ///////////////




// Use for real or symbolic points
// Decides whether point p lies on the edge 
int on(Point *p, Edge *e)
{
	if (symbolic(p) || symbolic(e->Org) || symbolic(e->Dest)) return 0;
	else {
		double point[2] = {p->x, p->y};
		double p1[2] = {e->Org->x, e->Org->y};
		double p2[2] = {e->Dest->x, e->Dest->y};
		return (orient2d(point, p1, p2) == 0) && 
		(((point[0] <= p1[0]) || (point[0] <= p2[0])) && 
		((point[0] >= p1[0]) || (point[0] >= p2[0])));
	}
}
// Determine whether point pj lies to the left of the line from pi -> pk
int left_of_symbolic(Point *pj, Point *pi, Point *pk)
{
	if (pk->id == -1) {
		// all points lie right of line -2 ---> -1
		if (pi->id == -2) return 0; 
		else if (pj->id == -2) return 1;
		else return lexico_comp(pj, pi) < 0;
	} else if (pi->id == -1) {
		// all points lie left of line -1 ----> -2
		if (pk->id == -2) return 1;
		else if (pj->id == -2) return 0;
		else return lexico_comp(pj, pk) > 0;
	} else if (pi->id == -2) {
		if (pj->id == -1) return 1;
		else return lexico_comp(pj, pk) < 0;
	} else if (pk->id == -2) {
		if (pj->id == -1) return 0;
		else return lexico_comp(pj, pi) > 0;
	} else if (pj->id == -1) return lexico_comp(pi, pk) < 0;
	// pj->id == -2
	else return lexico_comp(pi, pk) > 0;
}

int right_of(Point *p, Edge *e) 
{
	if (symbolic(p) || symbolic(e->Org) || symbolic(e->Dest)) return left_of_symbolic(p, e->Dest, e->Org);
	else return ccw(e->Org, e->Dest, p) == -1;
}
int left_of(Point *p, Edge *e)
{
	if (symbolic(p) || symbolic(e->Org) || symbolic(e->Dest)) return left_of_symbolic(p, e->Org, e->Dest);
	else return ccw(e->Org, e->Dest, p) == 1;
}
// locate point p in triangulation that includes edge e somewhere
// using Green/Sibson "walking" method
// with some fixes to avoid infinite loops
Edge *locate(Point *x, Edge *e)
{
	do {
		if (on(x, e)) return e;
		else if (right_of(x, e)) {
			
			e = sym(e);
		
		} else if (left_of(x, onext(e))) { 
			e = onext(e);
		} else if (left_of(x, dprev(e))) {
			e = dprev(e);
		} else {
			// in this triangle somewhere but might 
			// lie on one of the other two edges
			if (on(x, onext(e))) return onext(e);
			else if (on(x, dprev(e))) return dprev(e);
			else return e;
		}
	} while(1);
}
// check edge from pi -> pj with pl, pk the apex vertices to the left/right respectively
int legal(Point *pi, Point *pj, Point *pk, Point *pl)
{
	// make sure pl -> pi -> pk are ccw oriented, otherwise edge cannot be flipped
	if ((ccw(pl, pi, pk) != 1) || (ccw(pk, pj, pl) != 1)) return 1;
	// check if edge pi -> pj on bounding triangle (always legal)
	if ((pi->id <= 0) && (pj->id <=0)) return 1;
	// if all points real use predicate
	else if (real(pi) && real(pj) && real(pk) && real(pl)) 
		return !in_circle(pi, pk, pj, pl);
	// See Dutch book p. 204
	else return min(pk->id, pl->id) < min(pi->id, pj->id);  
}




////////// POINT INSERTION ///////////////




// assume x lies on edge e.
// delete edge e, and connect x to surrounding vertices
// t = oprev(e)
Edge *prepare_on(Point *x, Edge *e, Edge *t) 
{
	//printf("Deleting edge %d -> %d\n", e->Org->id, e->Dest->id);
	delete_edge(e);
	e = t;
	return prepare_in(x, e);
}
// assume x lies strictly inside e's left face.
// connect x to surrounding vertices
Edge *prepare_in(Point *x, Edge *e)
{
	Edge *base = make_edge();
	Point *first = e->Org;
	set_equal(base->Org, first);
	set_equal(base->Dest, x);
	splice(base, e);
	do {
		base = connect(e, sym(base));
		e = oprev(base);
	} while (!equal(e->Dest, first));
	return base;
}	
//Edge *insert_site(Point *x, Edge *existing, int fast, Node **loc_tree)
void insert_site(Point *x, Edge *existing, int fast, Node **loc_tree)

{
	Edge *e;
	if (fast) e = find(x, *loc_tree);
	else e = locate(x, existing);
	// make sure x doesn't lie on an existing vertex
//	if (!lexico_comp(x, e->Org) || !lexico_comp(x, e->Dest)) return existing;
	if (!lexico_comp(x, e->Org) || !lexico_comp(x, e->Dest)) return;
	Edge *base;
	Point first;
	set_equal(&first, e->Org);
	if (on(x, e)) {
		if (fast) base = extend_on(x, e);
		else {
			Edge *t = oprev(e);
			base = prepare_on(x, e, t);
		}
	} else {
		if (fast) base = extend_in(x, e);
		else base = prepare_in(x, e);
	}
	e = oprev(base);
	// flip illegal edges
	do {
		Edge *t = oprev(e);
		if (right_of(t->Dest, e) && (!legal(e->Org, e->Dest, t->Dest, x))) {
			if (fast) swap_location(e);
			else swap(e);
			e = oprev(e);
//		} else if (equal(e->Org, &first)) return e;
		} else if (equal(e->Org, &first)) return;
		else {
			e = lprev(onext(e));
		}
	} while(1);
}




///////////// TRIANGULATION //////////////////




// triangulation handler
Edge *delaunay(int random, int fast, read_io *io, int *max_index)
{
	int i;
	double x,y;
	Point max = {io->point_list[0], io->point_list[1], 0};
	int index = 0;
	// find lexico. max input point to use in bounding triangle
	//printf("Finding max point\n");
	for (i = 1; i < io->num_points; i++) {
		x = io->point_list[2*i]; 
		y = io->point_list[2*i+1];
		if ((x > max.x) || ((x == max.x) && (y > max.y))) {
			max.x = x;
			max.y = y;
			index = i;
		}
	
	}
	// remember its index so can swap it out when writing output file
	*max_index = index;

	Point neg1 = {0.0,0.0,-1};
	Point neg2 = {0.0,0.0,-2};
	// form bounding triangle using two symbolic points as in the Dutch book
	Edge *a = make_edge();
	Edge *b = make_edge();
	splice(sym(a), b);
	set_equal(a->Org, &neg2); 
	set_equal(a->Dest, &max);
	// initialize DAG if fast flag is set
	Node *loc_tree;
	if (fast) initialize(a, &loc_tree);

	set_equal(b->Org, &max);
	set_equal(b->Dest, &neg1); 
	Edge *e = connect(b, a); 
	// randomize indices if random flag is set
	int random_indices[io->num_points];
	if (random) {
		for (i = 0; i < io->num_points; i++) random_indices[i] = i;
		shuffle(random_indices, io->num_points);
	}
	// insert points! 
	for (i = 0; i < io->num_points; i++) {
		if (random) index = random_indices[i];
		else index = i;
		if (index == *max_index) continue;
		else {
			x = io->point_list[2*index];
			y = io->point_list[2*index+1];
			//printf("Inserting point (%f, %f)\n", x, y);
			Point p = {x, y, index+1};
//			e = insert_site(&p, e, fast, &loc_tree);
			insert_site(&p, a, fast, &loc_tree);
		}
	}
	// free DAG
	// free_nodes(&loc_tree); // this is broken
	return a;
}

int main(int argc, char *argv[])
{
	// initialize robust predicates
	exactinit();

	command_line input_args = {0,0,NULL,NULL};
	printf("Reading command line\n");
	int error = read_input(argc, argv, &input_args);
	if (error) {
		printf("Error reading command line\n");
		return 1;
	}
	FILE *file;
	file = fopen(input_args.in_filename, "r");
	printf("Reading input file\n");
	if (file == (FILE *) NULL) {
		printf("Error reading input file\n");
		return 1;
	}
	int first_node;
	read_io io;
	error = file_readnodes(file, &first_node, &io);
	fclose(file);
	if (error) {
		printf("Error reading input file\n");
		return 1;
	}
	clock_t time = clock();
	int max_index;
	// construct triangulation. e is any edge in the result
	printf("Triangulating\n");
	Edge *e = delaunay(input_args.randomized, input_args.fast, &io, &max_index);
	time = clock() - time;
	printf("Running time %lu ms\n", time * 1000 / CLOCKS_PER_SEC);
	// write results
	file = fopen(input_args.out_filename, "w");
	if (file == (FILE *) NULL) {
		printf("Error writing results\n");
		return 1;
	}
	int num_tris = file_writenodes(file, e, &io, first_node, max_index);
	fclose(file);
	file = fopen(input_args.out_filename, "r+");
	if (file == (FILE *) NULL) {
		printf("Error writing results\n");
		return 1;
	}
	// overwrite header of output file
	fprintf(file, "%d 3 0\n", num_tris);
	fclose(file);
	free(io.point_list);
	return 0;
}

