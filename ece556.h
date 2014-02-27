// ECE556 - Copyright 2014 University of Wisconsin-Madison.  All Rights Reserved.


#ifndef ECE556_H
#define ECE556_H

#include <stdio.h>

 /**
  * A structure to represent a 2D Point. 
  */
 typedef struct
 {
   int x ; /* x coordinate ( >=0 in the routing grid)*/
   int y ; /* y coordinate ( >=0 in the routing grid)*/

 } point ;


  /**
  * A structure to represent a segment
  */
 typedef struct
 {
   point p1 ; 	/* start point of a segment */
   point p2 ; 	/* end point of a segment */
   
   int numEdges ; 	/* number of edges in the segment*/
   int *edges ;  	/* array of edges representing the segment*/
   
 } segment ;
 
 
  /**
  * A structure to represent a route
  */
  typedef struct
  {
    int numSegs ;  	/* number of segments in a route*/
    segment *segments ;  /* an array of segments (note, a segment may be flat, L-shaped or any other shape, based on your preference */

  } route ;
 
 
  /**
  * A structure to represent nets
  */
  typedef struct
  {

   int id ; 		/* ID of the net */
   int numPins ; 		/* number of pins (or terminals) of the net */
   point *pins ; 		/* array of pins (or terminals) of the net. */
   route nroute ;		/* stored route for the net. */

  } net ;
  
  /**
  * A structure to represent the routing instance
  */
  typedef struct
  {
   int gx ;		/* x dimension of the global routing grid */
   int gy ;		/* y dimension of the global routing grid */
   
   int cap ;
   
   int numNets ;	/* number of nets */
   net *nets ;		/* array of nets */
   
   int numEdges ; 	/* number of edges of the grid */
   int *edgeCaps; 	/* array of the actual edge capacities after considering for blockages */
   int *edgeUtils;	/* array of edge utilizations */  
   
  } routingInst ;
  

/* int readBenchmark(const char *fileName, routingInst *rst)
   Read in the benchmark file and initialize the routing instance.
   This function needs to populate all fields of the routingInst structure.
   input1: fileName: Name of the benchmark input file
   input2: pointer to the routing instance
   output: 1 if successful
*/
int readBenchmark(const char *fileName, routingInst *rst);

  
/* int solveRouting(routingInst *rst)
   This function creates a routing solution
   input: pointer to the routing instance
   output: 1 if successful, 0 otherwise (e.g. the data structures are not populated) 
*/
int solveRouting(routingInst *rst);
  
/* int writeOutput(const char *outRouteFile, routingInst *rst)
   Write the routing solution obtained from solveRouting(). 
   Refer to the project link for the required output format.

   Finally, make sure your generated output file passes the evaluation script to make sure
   it is in the correct format and the nets have been correctly routed. The script also reports
   the total wirelength and overflow of your routing solution.

   input1: name of the output file
   input2: pointer to the routing instance
   output: 1 if successful, 0 otherwise 
  */
  int writeOutput(const char *outRouteFile, routingInst *rst);
  
  /* int release(routingInst *rst)
     Release the memory for all the allocated data structures. 
     Failure to release may cause memory problems after multiple runs of your program. 
     Need to recursively delete all memory allocations from bottom to top 
     (starting from segments then routes then individual fields within a net struct, 
     then nets, then the fields in a routing instance, and finally the routing instance)

     output: 1 if successful, 0 otherwise 
  */
 int release(routingInst *rst);


#endif // ECE556_H

