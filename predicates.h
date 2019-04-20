#ifndef PREDICATES_H_
#define PREDICATES_H_
#define REAL double

REAL orient2d(REAL *pa, REAL *pb, REAL *pc);
REAL incircle(REAL *pa, REAL *pb, REAL *pc, REAL *pd);
void exactinit();

#endif // PREDICATES_H_
