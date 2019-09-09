#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "predicates.h"
#include "utils.h"

////////// POINT UTILITIES //////////

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
int equal_id(Point *p1, Point *p2) 
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
int equal_val(Point *p1, Point *p2) 
{
	return !lexico_comp(p1, p2);
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
		double ap3[2] = {p3->x, p3->y};
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
// Use for real or symbolic points
// Decides whether point p lies on the edge 
int on_helper(Point *p, Point *org, Point *dest)
{
	if (symbolic(p) || symbolic(org) || symbolic(dest)) return 0;
	else {
		double point[2] = {p->x, p->y};
		double p1[2] = {org->x, org->y};
		double p2[2] = {dest->x, dest->y};
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


int right_of_helper(Point *p, Point *org, Point *dest) 
{
	if (symbolic(p) || symbolic(org) || symbolic(dest)) return left_of_symbolic(p, dest, org);
	else return ccw(org, dest, p) == -1;
}
int left_of_helper(Point *p, Point *org, Point *dest)
{
	if (symbolic(p) || symbolic(org) || symbolic(dest)) return left_of_symbolic(p, org, dest);
	else return ccw(org, dest, p) == 1;
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


////////// QUAD-EDGE OPERATIONS //////////

Node *left_face(Edge *e) { return e->face; }

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
	//free(e);
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
// create a triangle with the given three points. Return edge p1 -----> p2
Edge *triangle(Point *p1, Point *p2, Point *p3) 
{
	Edge *a = make_edge();
	Edge *b = make_edge();
	splice(sym(a), b);
	set_equal(a->Org, p1); 
	set_equal(a->Dest, p2);
	set_equal(b->Org, p2);
	set_equal(b->Dest, p3); 
	connect(b, a); 
	return a;
}



///////// POINT/EDGE COMPARISONS ///////////////

// Use for real or symbolic points
// Decides whether point p lies on the edge 
int on(Point *p, Edge *e)
{
	return on_helper(p, e->Org, e->Dest);
}
int right_of(Point *p, Edge *e) 
{
	return right_of_helper(p, e->Org, e->Dest);
}
int left_of(Point *p, Edge *e)
{
	return left_of_helper(p, e->Org, e->Dest);
}

////////// POINT LOCATION ////////////

// locate point p in triangulation that includes edge e somewhere
// using Green/Sibson "walking" method
// with some fixes to avoid infinite loops
Edge *locate_slow(Point *x, Edge *e)
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

// Point location with history DAG 

int leaf(Node *n) { return n->type; }
// update pointers to DAG for all edges that share e's left face
void set_left_face(Edge *e, Node *f)
{
	Edge *next = lnext(e);
	e->face = f;
	while (next != e) {
		next->face = f;
		next = lnext(next);
	}
}

Node *init_edge(Edge *e)
{
	Node *n = malloc(sizeof(Node));
	n->type = 0; // edge node for navigating through DAG
	n->edge = NULL; // won't use this since doesn't represent a face 
	n->Org = malloc(sizeof(Point)); n->Dest = malloc(sizeof(Point)); // create copies in case e is later destroyed
	set_equal(n->Org, e->Org); set_equal(n->Dest, e->Dest);
	return n;
}
// create new face node that lies to the left of e
Node *init_face(Edge *e)
{
	Node *n = malloc(sizeof(Node));
	n->type = 1; // face node (leaf)
	n->left = NULL; n->right = NULL; // won't use this
	n->edge = e; 
	n->Org = NULL; n->Dest = NULL; // won't use these
	return n;
}

// initialize DAG with the first (outer bounding) triangle. Assume the universe lies to the left of a
void initialize(Edge *a, Node **root)
{
	*root = init_face(a); // initialize face node
	set_left_face(a, *root); // set pointers from all edges in the triangulation 
				// for which this is their left face
}
// turn a face (leaf) node into an edge (internal) node so that you can add to the DAG
Node *repurpose(Node *n, Edge *e) {
	n->type = 0;
	n->edge = NULL;
	n->Org = malloc(sizeof(Point)); n->Dest = malloc(sizeof(Point));
	set_equal(n->Org, e->Org); set_equal(n->Dest, e->Dest);
	return n;
}

int compare(Point *p, Node *n) // -1 if p left, 0 if p on, 1 if p right
{
	if (leaf(n)) {
		//printf("leaf\n");
		return 0; // search reached leaf node, time to stop
	} else { // n is an edge (internal node of the DAG) 
		//printf("not leaf\n");
		if (ccw(n->Dest, n->Org, p) > 0) {
			return 1;
		} else return -1; // if p lies on n's edge, go left
	}

}

Node *search(Point *p, Node *n) 
{
	int val;
	while(1) {
		val = compare(p, n);
		if (val < 0) {
			n = n->left;
		} else if (val > 0) {
			n = n->right;
		} else {
			return n;
		}
	}
}

Edge *locate_fast(Point *p, Node *n)
{
	Edge *found = search(p, n)->edge;
	// must check whether p is on any edge of the face 
	if (on(p, lnext(found))) found = lnext(found);
	else if (on(p, lprev(found))) found = lprev(found);
	return found;
}

Edge *locate(Point *x, Edge *e, int fast, Node *loc_tree)
{
	if (fast) return locate_fast(x, loc_tree);
	else return locate_slow(x, e);
}


////////// POINT INSERTION ///////////////

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
	} while (!equal_id(e->Dest, first));
	return base;
}	

// Assume D is a face and p lies strictly inside D. 
// Form edges from p to all vertices of D.
// 
//  
//            /^
//           /  \
//          /    \
//       e3/      \e2
//        /        \
//       L          \
//       ----------->
//            e
//
Edge *prepare_in_fast(Point *x, Edge *e)
{
	//printf("Extending point inside face\n");
	Edge *e1 = e;
	Edge *e2 = lnext(e1);
	Edge *e3 = lprev(e1);
	Node *D = left_face(e);
	
	// do updates in triangulation
	// keep base to give back to insert_site
	Edge *base = prepare_in(x, e);
	
	// create 3 new faces
	Node *D1 = init_face(e1);
	Node *D2 = init_face(e2);
	Node *D3 = init_face(e3);
	
	// alias old face as new comparison edge
	Node *f1 = repurpose(D, lnext(e1));

	// create 2 new comparison edges
	Node *f2 = init_edge(lnext(e2));
	Node *f3 = init_edge(lnext(e3));

	// set pointers
	f1->left = f3;
	f3->left = D3;
	f3->right = D1;
	
	f1->right = f2;
	f2->left = D2;
	f2->right = D3;
	
	// set pointers from appropriate edges in triangulation
	set_left_face(e1, D1); set_left_face(e2, D2); set_left_face(e3, D3);
	
	return base;	
	
}


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


// Assume p lies on edge e.
// Delete the edge and form edges from p to all vertices
// of the new face.
//         e2                      e2
//       <-----                  <------
//      |^     ^                 |\  ep/^
//      | \ D2 |                 | \e / |
//   e1 |  \e  |e4   ---->    e1 |  \/  |e4
//      |   \  |                 |  /\e |
//      | D1 \ |                 | /  \ |
//      |     \|                 |/epp \|
//	------>                  L------>
//         e3                       e3
//
Edge *prepare_on_fast(Point *p, Edge *e)
{
	// need this to pass into the method in triangle.c
	Edge *t = oprev(e);

	Node *D1 = left_face(e);
	Node *D2 = left_face(sym(e));

	Edge *e1 = lnext(e);
	Edge *e3 = lprev(e);
	Edge *e4 = lnext(sym(e));
	Edge *e2 = lprev(sym(e));

	// do updates in triangulation
	// keep edge "base" to give back to insert_site method
	Edge *base = prepare_on(p, e, t);
	
	// create 4 new faces
	Node *D1p = init_face(e1);	
	Node *D2p = init_face(e2);
	Node *D3p = init_face(e3);
	Node *D4p = init_face(e4);
	
	// alias D1 as new comparison node ep
	Node *ep = repurpose(D1, lnext(e1));
	
	// set pointers for ep
	ep->right = D3p; 

	ep->left = D1p;

	set_left_face(e1, D1p); set_left_face(e3, D3p);		
	
	// do same for epp
	Node *epp = repurpose(D2, lprev(e2));

	epp->right = D4p;
	
	epp->left = D2p;

	set_left_face(e2, D2p); set_left_face(e4, D4p);
	
	return base;		
}


Edge *prepare(Point *x, Edge *e, int fast)
{
	if (on(x, e)) {
		if (fast) return prepare_on_fast(x, e);
		else return prepare_on(x, e, oprev(e));
	} else {
		if (fast) return prepare_in_fast(x, e);
		else return prepare_in(x, e);
	}
}

// switch triangulation of e's two adjacent faces.
// if using slow point locations, will call this directly to swap edge
// instead of updating DAG
void swap_slow(Edge *e) 
{
	Edge *a = oprev(e);
	Edge *b = oprev(sym(e));
	splice(e, a); splice(sym(e), b);
	splice(e, lnext(a)); splice(sym(e), lnext(b));
	set_equal(e->Org, a->Dest); set_equal(e->Dest, b->Dest);
}

//                 
//       <-----     
//      |^     ^     
//      | \ D2 |      
//   e1 |  \e  |e2   
//      |   \  |        
//      | D1 \ |         
//      L     \|
//	------>  
//                
//
void swap_fast(Edge *e)
{
	//printf("Swapping edge %d -> %d\n", e->Org->id, e->Dest->id);	
	
	Edge *e1 = lnext(e);
	Edge *e2 = lnext(sym(e));
	Node *D1 = left_face(e);
	Node *D2 = left_face(sym(e));
	
	// swap e in triangulation
	swap_slow(e);
	
	// create 2 new faces
	Node *D1p = init_face(e1);
	Node *D2p = init_face(e2);
	
	// D1 and D2 must now be comparison edges for new swapped e
	Node *ep = repurpose(D1, lnext(e1));
	Node *epp = repurpose(D2, lnext(e1));

	// set pointers
	ep->left = D1p; ep->right = D2p;
	epp->left = D1p; epp->right = D2p;

	set_left_face(e1, D1p);
	set_left_face(e2, D2p);
}


void swap(Edge *e, int fast)
{
	if (fast) swap_fast(e);
	else swap_slow(e);
}

//Edge *insert_site(Point *x, Edge *existing, int fast, Node **loc_tree)
void insert_site(Point *x, Edge *existing, int fast, Node **loc_tree)

{	

	Edge *e;
	// find the triangle in which x lies
	// this is the left face of e
	// if x lies on an edge, e is this edge
	e = locate(x, existing, fast, *loc_tree);
	// make sure x doesn't lie on an existing vertex of the triangulation
	if (equal_val(x, e->Org) || equal_val(x, e->Dest)) return;
	
	Point first;
	set_equal(&first, e->Org);
	
	// start by deleting e if x lies on e, and then
	// connecting x to all vertices of the face. 
	Edge *base = prepare(x, e, fast);
	
	e = oprev(base);
	// flip illegal edges
	do {
		Edge *t = oprev(e);
		if (right_of(t->Dest, e) && (!legal(e->Org, e->Dest, t->Dest, x))) {
			swap(e, fast);
			e = oprev(e);
		} else if (equal_id(e->Org, &first)) return;
		else {
			e = lprev(onext(e));
		}
	} while(1);

}


///////////// TRIANGULATION //////////////////

// triangulation handler
Edge *delaunay(int random, int fast, read_io *io, int *max_index)
{
	// find lexico. max input point to use in bounding triangle.
	// its id will be aliased to 0.
	// remember its index so can swap it out when writing output file
	Point max;
	find_max(io, max_index, &max);
	
	// form bounding triangle using two symbolic points as in the Dutch book
	Point neg1 = {0.0, 0.0, -1};
	Point neg2 = {0.0, 0.0, -2};
	Edge *a = triangle(&neg2, &max, &neg1);
	
	// initialize DAG if necessary
	Node *loc_tree;
	if (fast) initialize(a, &loc_tree);
	
	// generate indices for order of point insertion.
	// randomize indices if random flag is set
	int indices[io->num_points];
	generate_indices(indices, io->num_points, random);
	
	// insert points!
	double x, y;
	int index;
	for (int i = 0; i < io->num_points; i++) {
		index = indices[i];
		if (index == *max_index) continue;
		else {
			x = io->point_list[2*index];
			y = io->point_list[2*index + 1];
			Point p = {x, y, index + 1};
			insert_site(&p, a, fast, &loc_tree);
		}
	}
	return a;
}


//////////// WRITE .ele FILE OUTPUT //////////////////

int file_writenodes(FILE *file, Edge *e, read_io *io, int first_node, int max_index)
{
	// header
	// this is wrong but we'll overwrite it later
	fprintf(file, "%d 3 0\n", io->num_points);
	
	// initialize queue for finding all triangles 
	node_t *q = NULL;
	enqueue(&q, e);
	
	Point *p1; Point *p2; Point *p3;
	int i = 0;
	while (q) {
		e = dequeue(&q);
		if (e->trav == 2) continue; // left face already processed
		else {	
			e->trav = 2; // make sure each face only processed once
			lnext(e)->trav = 2; lprev(e)->trav = 2;
			// add edges of neighboring faces if they haven't yet been added to the queue
			if (!(sym(e)->trav)) {
				enqueue(&q, sym(e));
				sym(e)->trav = 1; // make sure each edge only added once
			} 
			if (!(sym(lnext(e))->trav)) {
				enqueue(&q, sym(lnext(e)));
				sym(lnext(e))->trav = 1;
			}
			if (!(sym(lprev(e))->trav)) {
				enqueue(&q, sym(lprev(e)));
				sym(lprev(e))->trav = 1;
			}
			p1 = e->Org; p2 = e->Dest; p3 = lnext(e)->Dest;
			// if any point is symbolic, skip the face
			if (symbolic(p1) || symbolic(p2) || symbolic(p3)) {	
				continue;

			} else {
				// write left face of current edge
				// change id of lexico. max vertex
				// points indexed at 1 in the data structure, so 
				// update if were indexed at 0 in input file
				// first_node is label of first node in input file (0 or 1)
				if (p1->id == 0) p1->id = max_index + first_node;
				if (p2->id == 0) p2->id = max_index + first_node;
				if (p3->id == 0) p3->id = max_index + first_node;
				fprintf(file, "%d %d %d %d\n", i + first_node, p1->id, p2->id, p3->id); 
				i++;	

			}
		}
	}
	return i;
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
	//free(io.point_list);
	return 0;
}

