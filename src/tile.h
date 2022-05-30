/* Copyright (c) 2001 Dave Tweed <tweed@cs.bris.ac.uk>  *
 * GNU General Public Licence                           */

#ifndef __TILE_RECTANGLES_H__
#define __TILE_RECTANGLES_H__

struct RectRec {
  int sizeX,x;
  int sizeY,y;
  int idNo;
  void assign(int sSizeX,int sSizeY,int sIdNo){
    sizeX=sSizeX;
    x=0;
    sizeY=sSizeY;
    y=0;
    idNo=sIdNo;
  }
  void setPosn(int sX,int sY){
    x=sX;
    y=sY;
  }
  //we want the list sorted by _increasing_ area
  bool operator<(const RectRec &r2) const{
    return sizeX*sizeY > r2.sizeX *r2.sizeY;
  }
};

/*return 0 if we failed to tile the region*/
bool
tileArea(int maxX,int maxY,int noRectangles,RectRec *rects);

#endif /*__TILE_RECTANGLES_H__*/
