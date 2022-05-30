/* Copyright (c) 2001 Dave Tweed <tweed@cs.bris.ac.uk>  *
 * GNU General Public Licence                           */

#include "tile.h"
#include <algorithm>
#include <stack>
//#include <stdio.h>

/* 
 * changed<05.02.2003> by Rudolf Polzer: including namespace std (C++)
 */

using namespace std;

/***************************************************************************
 *this file implements a quick and nasty routine for attempting to find
 *a compact placement of rectangles within a larger rectangular area,
 *essentially by trying a pruned-search using the following algorithm:
 * o sort the rectangles so they are placed in order of decreasing size
 * o initialise a `frontier' of possible places for the `top-left' corner
 *   of the next rectangle with (0,0)
 * o for the current rectangle R_n to be placed:
 * +  try to place R_n at an untried point on the frontier
 * + IF R_n doesn't overlap with any other rectangle currently placed
 *   THEN add its bottom left corner and top right corner to the frontier
 *        and try to place R_n+1 using the same technique
 *   ELSE IF the frontier is exhausted without a successful configuration
 *   THEN try a different position for R_n-1
 *Since this is exponential in the number of rectangles to place, a limit is
 *set on the amount of `work cycles' that will be spent trying to place the
 *rectangles before the best solution found so far is returned
 *(Clearly there are problems where a tiling is possible but it won't be
 *found by this algorithm but it works well for most real situations)
 **************************************************************************/

const int MAX_WORK_CYCLES_ACCEPTABLE=10000;

/**************************************************************************
 *frontier is a list of easily computed positions where placing the
 *next rectangle could be good
 *************************************************************************/
struct FrontierRec {
  int x,y;
  int currentlyUsed;
  void set(int sX,int sY,int sCurrentlyUsed){
    x=sX;
    y=sY;
    currentlyUsed=sCurrentlyUsed;
  }
};

/**************************************************************************
 *make the code slightly simpler; of course most C++ compilers will
 *produce abysmyal code for this case even under high optimisation. Ho hum.
 *************************************************************************/
//typedef pair<int,int> PairVals;
struct PairVals {
  int first;
  int second;
};

inline
int
rectFitsOnScreen(int maxX,int maxY,const RectRec &r)
{
  return r.x+r.sizeX < maxX  &&  r.y+r.sizeY < maxY;
}

inline
bool
rectanglesOverlap(const RectRec &r1,const RectRec &r2)
{
  //compute the distance between centres along each axis and see if they are
  //both less than the 1/2 (sum of sizes rectangles dim) for each axis
  //I _think_ this is the computationally best test for overlapping-ness
  return 2*abs(r1.x+r1.sizeX/2-(r2.x+r2.sizeX/2)) < (r1.sizeX+r2.sizeX)
    && 2*abs(r1.y+r1.sizeY/2-(r2.y+r2.sizeY/2)) < (r1.sizeY+r2.sizeY);
}

/**************************************************************************
 *return true if we've either given a correct solution or exceeded our
 *work cycles limit. this was originally written recursively but changed to
 *be recursive, even though this obscures what's going on slightly
 *************************************************************************/
static
void
addRectToTiling(int maxX,int maxY,int noItemsOnFrontP,FrontierRec* front,
                int noRectangles,RectRec *rects,int curRect,
                RectRec *bestPlacement,PairVals *maxDimens,int &lowestArea,
                int maxWorkCycles)
{
  int workCyclesLeft=maxWorkCycles;
  int noItemsOnFront=noItemsOnFrontP;
  int curPosOnFront=0;
  stack<pair<int,int> > theStack;
  do{
    if(!front[curPosOnFront].currentlyUsed){
      //only count this as working cycle if we do reasonable amount of work
      --workCyclesLeft;
      //place rectangle at current frontier position & disable it
      rects[curRect].setPosn(front[curPosOnFront].x,front[curPosOnFront].y);
      front[curPosOnFront].currentlyUsed=1;
      /*check latest placement is consistent (using a simple O(n) alg)*/
      int consistentSoFar=rectFitsOnScreen(maxX,maxY,rects[curRect]);
      int i;
      for(i=curRect-1;/*consistentSoFar &&*/ i>=0;--i){
        consistentSoFar &= !rectanglesOverlap(rects[i],rects[curRect]);
      }
      if(consistentSoFar){
        maxDimens[curRect+1].first=max(maxDimens[curRect].first,
                                       rects[curRect].x+rects[curRect].sizeX);
        maxDimens[curRect+1].second=max(maxDimens[curRect].second,
                                        rects[curRect].y+rects[curRect].sizeY);
        if(curRect==noRectangles-1){
          //if we've got this far, then we must have a valid configuration,
          //so record it if it's better than the current best
          int bboxArea=maxDimens[curRect+1].first*maxDimens[curRect+1].second;
          if(bboxArea < lowestArea){
            lowestArea=bboxArea;
            //is there any advantage to using a bitwise copy?
            copy(rects,rects+noRectangles,bestPlacement);
          }
        }else{
          theStack.push(make_pair(curPosOnFront,noItemsOnFront));
          //expand frontier to accomodate the points from this rectangle
          front[noItemsOnFront++]
            .set(rects[curRect].x+rects[curRect].sizeX+1,rects[curRect].y,0);
          front[noItemsOnFront++]
            .set(rects[curRect].x,rects[curRect].y+rects[curRect].sizeY+1,0);
          ++curRect;
          curPosOnFront=0;
        }
      }
    }//ELSE we try the next position for the current rectangle
    ++curPosOnFront;
    while(curPosOnFront>=noItemsOnFront){
      if(theStack.empty()){
        break;
      }
#if 0
      assert(!theStack.empty());
#endif
      curPosOnFront=theStack.top().first+1;
      noItemsOnFront=theStack.top().second;
      //NOW BEFORE ANYTHING ELSE, remember to reactivate posn on frontier
      front[curPosOnFront].currentlyUsed=0;
      --curRect;
      theStack.pop();
    }
  }while(workCyclesLeft>0 && !theStack.empty());
  //fprintf(stderr,
  //        workCyclesLeft>0 ? "finished ok\n" : "ran out of cycles\n");
}

/*return false if we failed to tile the region*/
bool
tileArea(int maxX,int maxY,int noRectangles,RectRec *rects)
{
  //ok, first we do a simple check whether a tiling is possible
  int areaOfConstituents=0;
  int i;
  for(i=0;i<noRectangles;++i){
    areaOfConstituents+=rects[i].sizeX*rects[i].sizeY;
  }
  if(noRectangles<=1 || areaOfConstituents>maxX*maxY){
    return false;
  }
  //order the rectangles by decreasing area (using STL sort for convenience)
  sort(rects,rects+noRectangles);
  RectRec *bestPlacement=new RectRec[5*noRectangles];
  int lowestBBoxArea=maxX*maxY+1;
  FrontierRec *front=new FrontierRec[5*2*noRectangles+2];
  front[0].set(0,0,0);
  PairVals *maxDimens=new PairVals[5*noRectangles+1];
  maxDimens[0].first = 0;//=make_pair(0,0);
  maxDimens[0].second = 0;
  addRectToTiling(maxX,maxY,1,front,noRectangles,rects,0,bestPlacement,
                  maxDimens,lowestBBoxArea,MAX_WORK_CYCLES_ACCEPTABLE);
  delete[] maxDimens;
  delete[] front;
  if(lowestBBoxArea<maxX*maxY+1){
    copy(bestPlacement,bestPlacement+noRectangles,rects);
  }
  delete[] bestPlacement;
  return lowestBBoxArea<maxX*maxY+1;
}
