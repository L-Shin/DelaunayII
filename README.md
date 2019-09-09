# Delaunay Triangulation by Incremental Insertion
This project implements two incremental insertion algorithms for constructing two-dimensional Delaunay triangulations, as described by [Leonidas J. Guibas and Jorge Stolfi, *Primitives for the Manipulation of General Subdivisions and the Computation of Voronoi Diagrams*](https://portal.acm.org/citation.cfm?doid=282918.282923), ACM Transactions on Graphics 4(2):74-123, April 1985. The algorithms use Guibas and Stolfi's quad-edge data structure. 

The first algorithm uses Guibas and Stolfi's suboptimal "walking" method for point location. The second maintains a history DAG for optimal point location. In order to use a history DAG, points are inserted into an outer bounding triangle, consisting of two symbolic vertices and the lexicographically maximum point in the point set. See [Mark de Berk, Otfried Cheong, Marc van Krevald, and Mark Overmars, *Computational Geometry: Algorithms and Applications*, third edition, Springer-Verlag, 2008](http://www.cs.uu.nl/geobook/), Section 9, for more information.
## Interface
The program uses the same file formats as Jonathan R. Shewchuk's program [Triangle](https://www.cs.cmu.edu/~quake/triangle.html). It reads a file with the suffix [.node](https://www.cs.cmu.edu/~quake/triangle.node.html) and writes a file with suffix [.ele](https://www.cs.cmu.edu/~quake/triangle.ele.html). 
### Compile
The project folder contains a makefile. Specify your favorite C compiler in the makefile (default is gcc) and run `make` to compile the program into executable `triangle`. Alternatively, manually compile `triangle.c`, `utils.c`, and predicates.c`. 
### Run
Use the following command line switches:
* `-r` specifies randomized point insertion (default is to insert in order from input file)
* `-f` specifies fast point location using a history DAG (default is the walking method)
* `-i` followed by the input file name specifies input file. You must input the file extension, for example `-i input.node`
* `-o` followed by a file name specifies output file name. If no output file name is specified the results will be stored in `output.ele`. Again, you must include the `.ele` extension
For example, for randomized insertion and fast point location, run `./triangle -rf -i spiral.node -o spiral.ele`. My program ignores boundary markers or attributes.
## Timings
All timings exclude time required to read/write files, and use randomized point insertion.

|              Runtime (ms)              |
|----------------------------------------|
|    | 10k nodes | 100k nodes | 1m nodes |
|----|-----------|------------|----------|
|slow|    276    |    12250   |  421769  |
|fast|    183    |    2020    |  24791   |

The time limiting step in the incremental insertion algorithm is the point location. The "walking method" could visit every face in the worst case, with asymptotic `O(n)` cost for point location. On the other hand, point location with a history DAG has expected `O(log n)` time, allowing the overall triangulation to run in expected `O(n log n)` time.
## Running time vs. random insertion
I created a square grid of 10,000 points in order `(0,0),(0,1),...,(0,99),(1,0),(1,1),...,(1,99),...` and so on. With fast point location, the algorithm required 179ms for random point insertion and 868ms for point insertion in order. Clearly randomizing insertion order significantly decreases running time with fast point insertion method. 
Interestingly, with the walking method the running time was 180ms with unput in order and 319ms with input randomized. This could have to do with the added time needed to randomize inputs, but it could also indicate that the walking method performs better with structured inputs.
I was curious whether, in the case of structured inputs, the walking method might perform better if the most recently created edge was used as the starting edge (previously I was using the same edge (-2)->(0) to start every search). So I changed the output of `insert\_site` to return an edge near the recently inserted point, and then used that as the starting point for the walking method. This reduced my running time to 96ms for structured input with the walking method. I believe this is an improvement if you choose the walking method because inputs will often be structured so the next point is near the previous point.
However, with this change the walking method with randomized insertion took so long to run that I had to halt it. 
See the test file `orderlygrid.node`.
## Borrowed code
I borrowed most of the code from `readline()`, `findfield()`, and `file\_readnodes\_internal()` in `triangle\_io.c` of Jonathan R. Shewchuk's [Triangle](https://github.com/wo80/Triangle) program for reading input `.node` files. I also used `orient2d()` and `incircle()` from Shewchuk's [Fast Robust Predicates for Computational Geometry](https://www.cs.cmu.edu/~quake/robust.html). 
