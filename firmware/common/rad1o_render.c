#include "rad1o_render.h"
#include "rad1o_decoder.h"
#include "rad1o_fonts.h"
#include "rad1o_smallfonts.h"
#include "rad1o_display.h"

#include <string.h>
#include <math.h>

void swap( int * a, int * b )
{
    int x = *a;
    *a = *b;
    *b = x;
}

void swapd( float * a, float * b )
{
    float x = *a;
    *a = *b;
    *b = x;
}

typedef struct
{
    float x, y, z;
} Vector3D;

Vector3D addVec3D( Vector3D a, Vector3D b )
{
    Vector3D v = {a.x+b.x, a.y+b.y, a.z+b.z};
    return v;
}

Vector3D subVec3D( Vector3D a, Vector3D b )
{
    Vector3D v = {a.x-b.x, a.y-b.y, a.z-b.z};
    return v;
}

float lengthVec3D( Vector3D v )
{
    return sqrt( v.x*v.x + v.y*v.y + v.z*v.z );
}

Vector3D roundVec3D( Vector3D v )
{
    Vector3D r = { (int)(v.x + 0.5),
                   (int)(v.y + 0.5),
                   (int)(v.z + 0.5) };
    return r;
}

Vector3D multMat3DVec3D( float* mat, Vector3D v )
{
    Vector3D r = { mat[0] * v.x + mat[1] * v.y + mat[2] * v.z,
                   mat[3] * v.x + mat[4] * v.y + mat[5] * v.z,
                   mat[6] * v.x + mat[7] * v.y + mat[8] * v.z };
    return r;
}

/* Global Variables */
const struct FONT_DEF * font = NULL;

struct EXTFONT efont;

uint8_t color_bg = 0xff; /* background color */
uint8_t color_fg = 0x00; /* foreground color */

/* Exported Functions */
void setTextColor(uint8_t bg, uint8_t fg){
  color_bg = bg;
  color_fg = fg;
}

void setIntFont(const struct FONT_DEF * newfont){
    memcpy(&efont.def,newfont,sizeof(struct FONT_DEF));
    efont.type=FONT_INTERNAL;
    font=&efont.def;
}

void setExtFont(const char *fname){
#if 0
    if(strlen(fname)>8+4)
        return;
    strcpy(efont.name,fname);
    efont.type=FONT_EXTERNAL;
    font=NULL;
    UINT res;
    res=f_open(&file, efont.name, FA_OPEN_EXISTING|FA_READ);
    if(res!=FR_OK){
	efont.type=0;
	font=&Font_7x8;
    }else{
	_getFontData(START_FONT,0);
	font=&efont.def;
    };
#endif
}

int getFontHeight(void){
    if(font)
      return font->u8Height;
    return 8; // XXX: Should be done right.
}
#if 0
static uint8_t read_byte (void)
{
  UINT    readbytes;
  uint8_t byte;
  UINT res;
  res=f_read(&file, &byte, sizeof(uint8_t), &readbytes);
  if(res!=FR_OK || readbytes!=1){
    ASSERT(0) ;
  };
  return byte;
}

int _getFontData(int type, int offset){
    UINT readbytes;
    UINT res;
    static uint16_t extras;
    static uint16_t character;
//    static const void * ptr;

    if(efont.type == FONT_EXTERNAL){

    if (type == START_FONT){
        efont.def.u8Width = read_byte ();
        efont.def.u8Height = read_byte ();
        efont.def.u8FirstChar = read_byte ();
        efont.def.u8LastChar = read_byte ();
        res = f_read(&file, &extras, sizeof(uint16_t), &readbytes);
        ASSERT(res==FR_OK);
        return 0;
    };
    if (type == SEEK_EXTRAS){
        f_lseek(&file,6);
        return 0;
    };
    if(type == GET_EXTRAS){
        uint16_t word;
        res = f_read(&file, &word, sizeof(uint16_t), &readbytes);
        ASSERT(res==FR_OK);
        return word;
    };
    if (type == SEEK_WIDTH){
        f_lseek(&file,6+(extras*sizeof(uint16_t)));
        return 0;
    };
    if(type == GET_WIDTH || type == GET_DATA){
        return read_byte ();
    };
    if(type == SEEK_DATA){
        character=offset;
        f_lseek(&file,6+
                (extras*sizeof(uint16_t))+
                ((extras+font->u8LastChar-font->u8FirstChar)*sizeof(uint8_t))+
                (offset*sizeof(uint8_t))
                );
        return 0;
    };
    if(type == PEEK_DATA){
        uint8_t width;
        width = read_byte ();
        f_lseek(&file,6+
                (extras*sizeof(uint16_t))+
                ((extras+font->u8LastChar-font->u8FirstChar)*sizeof(uint8_t))+
                (character*sizeof(uint8_t))
                );
        return width;
    };
#ifdef NOTYET
    }else{ // efont.type==FONT_INTERNAL

        if (type == START_FONT){
            memcpy(&efont.def,font,sizeof(struct FONT_DEF));
            return 0;
        };
        if (type == SEEK_EXTRAS){
            ptr=efont.def.charExtra;
            return 0;
        };
        if(type == GET_EXTRAS){
            uint16_t word;
            word=*(uint16_t*)ptr;
            ptr=((uint16_t*)ptr)+1;
            return word;
        };
        if (type == SEEK_WIDTH){
            ptr=efont.def.charInfo;
            return 0;
        };
        if(type == GET_WIDTH || type == GET_DATA){
            uint8_t width;
            width=*(uint8_t*)ptr;
            ptr=((uint8_t*)ptr)+1;
            return width;
        };
        if(type == SEEK_DATA){
            if(offset==REPEAT_LAST_CHARACTER)
                offset=character;
            else
                character=offset;

            ptr=efont.def.au8FontTable;
            return 0;
        };
#endif
    };

    /* NOTREACHED */
    return 0;
}
#endif

static int _getIndex(int c){
#define ERRCHR (font->u8FirstChar+1)
    /* Does this font provide this character? */
    if(c<font->u8FirstChar)
        c=ERRCHR;
    if(c>font->u8LastChar && efont.type!=FONT_EXTERNAL && font->charExtra == NULL)
        c=ERRCHR;

    if(c>font->u8LastChar && (efont.type==FONT_EXTERNAL || font->charExtra != NULL)){
        if(efont.type==FONT_EXTERNAL){
#if 0
            _getFontData(SEEK_EXTRAS,0);
            int cc=0;
            int cache;
            while( (cache=_getFontData(GET_EXTRAS,0)) < c)
                cc++;
            if( cache > c)
                c=ERRCHR;
            else
                c=font->u8LastChar+cc+1;
#endif
        }else{
            int cc=0;
            while( font->charExtra[cc] < c)
                cc++;
            if(font->charExtra[cc] > c)
                c=ERRCHR;
            else
                c=font->u8LastChar+cc+1;
        };
    };
    c-=font->u8FirstChar;
    return c;
}

uint8_t charBuf[MAXCHR];

int DoChar(int sx, int sy, int c){

    if(font==NULL){
	font=&Font_7x8;
    };

	/* how many bytes is it high? */
    char height=(font->u8Height-1)/8+1;
    char hoff=(8-(font->u8Height%8))%8;


	const uint8_t * data;
    int width,preblank=0,postblank=0;
    do { /* Get Character data */
        /* Get intex into character list */
        c=_getIndex(c);

        /* starting offset into character source data */
        int toff=0;

        if(font->u8Width==0){
            if(efont.type == FONT_EXTERNAL){
#if 0
                _getFontData(SEEK_WIDTH,0);
                for(int y=0;y<c;y++)
                    toff+=_getFontData(GET_WIDTH,0);
                width=_getFontData(GET_WIDTH,0);

                _getFontData(SEEK_DATA,toff);
                UINT res;
                UINT readbytes;
                UINT size = width * height;
                if(size > MAXCHR) size = MAXCHR;
                res = f_read(&file, charBuf, size, &readbytes);
                if(res != FR_OK || readbytes<width*height)
                    return sx;
                data=charBuf;
#endif
            }else{
                for(int y=0;y<c;y++)
                    toff+=font->charInfo[y].widthBits;
                width=font->charInfo[c].widthBits;

                toff*=height;
                data=&font->au8FontTable[toff];
            };
            postblank=1;
        }else if(font->u8Width==1){ // NEW CODE
            if(efont.type == FONT_EXTERNAL){
#if 0
                _getFontData(SEEK_WIDTH,0);
                for(int y=0;y<c;y++)
                    toff+=_getFontData(GET_WIDTH,0);
                width=_getFontData(GET_WIDTH,0);
                _getFontData(SEEK_DATA,toff);
                UINT res;
                UINT readbytes;
                uint8_t testbyte;
                testbyte = read_byte ();
                if(testbyte>>4 ==15){
                    preblank = read_byte ();
                    postblank = read_byte ();
                    width-=3;
                    width/=height;
                    UINT size = width * height;
                    if(size > MAXCHR) size = MAXCHR;
                    res = f_read(&file, charBuf, size, &readbytes);
                    if(res != FR_OK || readbytes<width*height)
                        return sx;
                    data=charBuf;
                }else{
                    _getFontData(SEEK_DATA,toff);
                    data=pk_decode(NULL,&width); // Hackety-hack
                };
#endif
            }else{
                // Find offset and length for our character
                for(int y=0;y<c;y++)
                    toff+=font->charInfo[y].widthBits;
                width=font->charInfo[c].widthBits;
                if(font->au8FontTable[toff]>>4 == 15){ // It's a raw character!
                    preblank = font->au8FontTable[toff+1];
                    postblank= font->au8FontTable[toff+2];
                    data=&font->au8FontTable[toff+3];
                    width=(width-3/height);
                }else{
                    data=pk_decode(&font->au8FontTable[toff],&width);
                }
            };

        }else{
            toff=(c)*font->u8Width*1;
            width=font->u8Width;
            data=&font->au8FontTable[toff];
        };

    }while(0);

#define xy_(x,y) ((x<0||y<0||x>=RESX||y>=RESY)?0:(y)*RESX+(x))
#define gPx(x,y) (data[x*height+(height-y/8-1)]&(1<<(y%8)))

	int x=0;

    /* Our display height is non-integer. Adjust for that. */
//    sy+=RESY%8;

    /* Fonts may not be byte-aligned, shift up so the top matches */
    sy-=hoff;

    sx+=preblank;

	/* per line */
	for(int y=hoff;y<height*8;y++){
        if(sy+y>=RESY)
            continue;

        /* Optional: empty space to the left */
		for(int b=1;b<=preblank;b++){
            if(sx>=RESX)
                continue;
			lcdBuffer[xy_(sx-b,sy+y)]=color_bg;
		};
        /* Render character */
		for(x=0;x<width;x++){
            if(sx+x>=RESX)
                continue;
			if (gPx(x,y)){
				lcdBuffer[xy_(sx+x,sy+y)]=color_fg;
			}else{
				lcdBuffer[xy_(sx+x,sy+y)]=color_bg;
			};
		};
        /* Optional: empty space to the right */
		for(int m=0;m<postblank;m++){
            if(sx+x+m>=RESX)
                continue;
			lcdBuffer[xy_(sx+x+m,sy+y)]=color_bg;
		};
	};
	return sx+(width+postblank);
}

#define UTF8
// decode 2 and 3-byte utf-8 strings.
#define UT2(a)   ( ((a[0]&31)<<6)  + (a[1]&63) )
#define UT3(a)   ( ((a[0]&15)<<12) + ((a[1]&63)<<6) + (a[2]&63) )

int DoString(int sx, int sy, const char *s){
	const char *c;
	int uc;
	for(c=s;*c!=0;c++){
#ifdef UTF8
		/* will b0rk on non-utf8 */
		if((*c&(128+64+32))==(128+64) && c[1]!=0){
			uc=UT2(c); c+=1;
			sx=DoChar(sx,sy,uc);
		}else if( (*c&(128+64+32+16))==(128+64+32) && c[1]!=0 && c[2] !=0){
			uc=UT3(c); c+=2;
			sx=DoChar(sx,sy,uc);
		}else
#endif
		sx=DoChar(sx,sy,*c);
	};
	return sx;
}

void DoRect( int x, int y, int width, int height )
{
    for(int py = y; py < y+height; ++py)
    {
        uint8_t * p = &lcdBuffer[py*RESX];
        for(int px = x; px < x+width; ++px)
        {
            p[px] = color_fg;
        }
    }
}

int abs( int x )
{
    return x < 0 ? -x : x;
}

void DoLine(int x1, int y1, int x2, int y2 )
{
    if( (x1 < 0 && x2 < 0)
     || (y1 < 0 && y2 < 0)
     || (x1 >= RESX  && x2 >= RESX )
     || (y1 >= RESY && y2 >= RESY )) return;

    int dx = abs(x2-x1), sx = x1<x2 ? 1 : -1;
    int dy = abs(y2-y1), sy = y1<y2 ? 1 : -1;

    if(dx != 0)
    {
        float m = dy/(float)(dx*sx*sy);
        float n = y1 - m*x1+0.5;
        // y = mx+n
        if( x1 < 0 )
        {
            x1 = 0; y1 = (int)(n);
        }
        else if( x1 >= RESX )
        {
            x1 = RESX - 1; y1 = (int)(x1*m + n);
        }

        if( x2 < 0 )
        {
            x2 = 0; y2 = (int)(n);
        }
        else if( x2 >= RESX)
        {
            x2 = RESX - 1; y2 = (int)(x2*m + n);
        }
    }

    if(dy != 0)
    {
        float q = dx/(float)(dy*sx*sy);
        float r = x1 - q*y1+0.5;
        // x = qy+r
        if( y1 < 0 )
        {
            y1 = 0; x1 = (int)(r);
        }
        else if( y1 >= RESY)
        {
            y1 = RESY - 1; x1 = (int)(y1*q + r);
        }

        if( y2 < 0 )
        {
            y2 = 0; x2 = (int)(r);
        }
        else if( y2 >= RESY)
        {
            y2 = RESY - 1; x2 = (int)(y2*q + r);
        }
    }

    if(x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0
       || x1 >= RESX || y1 >= RESY
       || x2 >= RESX || y2 >= RESY)
        return;

    if(y1>y2)
    {
        int xz = x1, yz = y1;
        x1 = x2;     y1 = y2;
        x2 = xz;     y2 = yz;
    }

    dx =  abs(x2-x1); sx = x1<x2 ? 1 : -1;
    dy = y1-y2; sy = y1<y2 ? 1 : -1;
    int err = dx+dy, e2;

    int* pxmin = x1<x2 ? &x1: &x2;
    int* pxmax = x1>x2 ? &x1: &x2;

    while(*pxmin < *pxmax || y1 < y2)
    {
        lcdBuffer[xy_(x1,y1)]=color_fg;
        e2 = 2*err;
        if (e2 > dy) { err += dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 ++; }
    }
}

void DoCube( int* p, int len, float* r )
{
    int px = p[0];
    int py = p[1];
    int pz = p[2];

    float rx = r[0];
    float ry = r[1];
    float rz = r[2];

    float cx = cos(rx), sx = sin(rx);
    float cy = cos(ry), sy = sin(ry);
    float cz = cos(rz), sz = sin(rz);
    float rotation[9] = { cy*cz,         -cy*sz,          sy,
                          cx*sz+cz*sx*sy, cx*cz-sx*sy*sz, -cy*sx,
                          sx*sz-cx*cz*sy, cz*sx+cx*sy*sz, cx*cy };
    Vector3D xAxis = { len * 0.5, 0, 0 };
    Vector3D yAxis = { 0, len * 0.5, 0 };
    Vector3D zAxis = { 0, 0, len * 0.5 };
    xAxis = multMat3DVec3D( rotation, xAxis );
    yAxis = multMat3DVec3D( rotation, yAxis );
    zAxis = multMat3DVec3D( rotation, zAxis );
    Vector3D p000 = { px, py, pz };
    Vector3D p100 = { px, py, pz };
    Vector3D p010 = { px, py, pz };
    Vector3D p110 = { px, py, pz };
    Vector3D p001 = { px, py, pz };
    Vector3D p101 = { px, py, pz };
    Vector3D p011 = { px, py, pz };
    Vector3D p111 = { px, py, pz };

    p000 = roundVec3D( subVec3D( subVec3D( subVec3D( p000, xAxis ), yAxis ), zAxis ) );
    p100 = roundVec3D( subVec3D( subVec3D( addVec3D( p100, xAxis ), yAxis ), zAxis ) );
    p010 = roundVec3D( subVec3D( addVec3D( subVec3D( p010, xAxis ), yAxis ), zAxis ) );
    p110 = roundVec3D( subVec3D( addVec3D( addVec3D( p110, xAxis ), yAxis ), zAxis ) );
    p001 = roundVec3D( addVec3D( subVec3D( subVec3D( p001, xAxis ), yAxis ), zAxis ) );
    p101 = roundVec3D( addVec3D( subVec3D( addVec3D( p101, xAxis ), yAxis ), zAxis ) );
    p011 = roundVec3D( addVec3D( addVec3D( subVec3D( p011, xAxis ), yAxis ), zAxis ) );
    p111 = roundVec3D( addVec3D( addVec3D( addVec3D( p111, xAxis ), yAxis ), zAxis ) );


    //    010_________110
    //      /|       /|
    //     / |      / |
    // 000/__+____/100|
    //    |  |    |   |
    //    011|____|___|111
    //    |  /    |  /
    //    | /     | /
    //    |/______|/
    //  001        101

    DoLine( p000.x, p000.y, p100.x, p100.y );
    DoLine( p000.x, p000.y, p010.x, p010.y );
    DoLine( p000.x, p000.y, p001.x, p001.y );

    DoLine( p111.x, p111.y, p011.x, p011.y );
    DoLine( p111.x, p111.y, p101.x, p101.y );
    DoLine( p111.x, p111.y, p110.x, p110.y );

    DoLine( p011.x, p011.y, p001.x, p001.y );
    DoLine( p011.x, p011.y, p010.x, p010.y );
    DoLine( p010.x, p010.y, p110.x, p110.y );

    DoLine( p100.x, p100.y, p110.x, p110.y );
    DoLine( p100.x, p100.y, p101.x, p101.y );
    DoLine( p001.x, p001.y, p101.x, p101.y );
}

void DoMesh( float* verts, int numbVerts, int* faces, int numbfaces, float* rot, int* p, int scale )
{
    float rx = rot[0];
    float ry = rot[1];
    float rz = rot[2];

    float cx = cos(rx), sx = sin(rx);
    float cy = cos(ry), sy = sin(ry);
    float cz = cos(rz), sz = sin(rz);
    float rotation[9] = { scale*cy*cz,          scale*-cy*sz,         -scale*sy,
                          scale*cx*sz+cz*sx*sy, scale*cx*cz-sx*sy*sz, -scale*-cy*sx,
                          scale*sx*sz-cx*cz*sy, scale*cz*sx+cx*sy*sz, -scale*cx*cy };

    Vector3D transformed[numbVerts];
    Vector3D* vertsVec = (Vector3D*)verts;
    Vector3D pos = {p[0], p[1], p[2] };
    for( int i = 0; i < numbVerts; ++i )
    {
        transformed[i] = roundVec3D( addVec3D( multMat3DVec3D( rotation, vertsVec[i]), pos ) );
    }
    for( int i = 0; i < numbfaces; ++i )
    {
        Vector3D v1 = transformed[ faces[i*3 + 0] ];
        Vector3D v2 = transformed[ faces[i*3 + 1] ];
        Vector3D v3 = transformed[ faces[i*3 + 2] ];

        DoLine( v1.x, v1.y, v2.x, v2.y );
        DoLine( v2.x, v2.y, v3.x, v3.y );
        DoLine( v3.x, v3.y, v1.x, v1.y );
    }
}
