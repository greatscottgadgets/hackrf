#ifndef __RENDER_H_
#define __RENDER_H_

#include "rad1o_fonts.h"

// ARM supports byte flip natively. Yay!
#define flip(byte) \
	__asm("rbit %[value], %[value];" \
		  "rev  %[value], %[value];" \
			: [value] "+r" (byte) : )

/* Alternatively you could use normal byte flipping:
#define flip(c) do {\
	c = ((c>>1)&0x55)|((c<<1)&0xAA); \
	c = ((c>>2)&0x33)|((c<<2)&0xCC); \
	c = (c>>4) | (c<<4); \
	}while(0)
*/

void setTextColor(uint8_t bg, uint8_t fg);
void setIntFont(const struct FONT_DEF * font);
void setExtFont(const char *file);
int getFontHeight(void);
int DoString(int sx, int sy, const char *s);
int DoChar(int sx, int sy, int c);

// print a rect starting at position (x,y) with size (width, height)
void DoRect( int x, int y, int width, int height );

// print a line starting at position (x1,y1) going to (x2,y2)
void DoLine(int x1, int y1, int x2, int y2 );

// print a 3D-Cube.
// p is a array with 3 elements (x,y,z) and sets the 3d position
// len is the cube edge length
// r is a array with 3 elements (angle around x-axis, angle around y-axis,
// angle around z-axis ) and sets the rotation of the cube
void DoCube(int* p, int len, float* r );

// prints a whole mesh on the screen
// verts is an array of size numbVerts*3 because we have for every vertex
// 3 axis-coordinates ( x,y,z )
// faces is an array of indices of size 3*numbFaces because
// it holds 3-tupels which describe triangles. The indices are the indices
// in the verts array.
// rot is the rotation like the cube has one
// p is a position of the mesh on the screen
// scale is a scaling factor for the whole mesh
void DoMesh( float* verts, int numbVerts, int* faces, int numbfaces, float* rot, int* p, int scale );


#define START_FONT 0
#define SEEK_EXTRAS 1
#define GET_EXTRAS 2
#define SEEK_WIDTH 3
#define GET_WIDTH 4
#define SEEK_DATA 5
#define GET_DATA 6
#define PEEK_DATA 7

int _getFontData(int type, int offset);

#define MAXCHR (30*20)
extern uint8_t charBuf[MAXCHR];

#endif
