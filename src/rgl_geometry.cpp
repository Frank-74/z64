/*
 * z64
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

#include "Gfx #1.3.h"
#include "rdp.h"
#include "rgl.h"

#include <SDL/SDL.h>

void rglTextureRectangle(rdpTexRect_t * rect, int flip)
{
  int tilenum = rect->tilenum;
  int x1,x2,y1,y2,z;
  int s, t;
  int dx, dy;

#define tile rdpTiles[tilenum]
  
	x1 = (rect->xh / 4);
	x2 = (rect->xl / 4);
	y1 = (rect->yh / 4);
	y2 = (rect->yl / 4);
  s = int(rect->s)<<5;
  t = int(rect->t)<<5;

  DUMP("texrect %d x %d --> %d x %d s %d t %d flip %d mask %d x %d\n",
       x1, y1, x2, y2, s, t, flip,
       (1<<tile.mask_s+10)-1, (1<<tile.mask_t+10)-1);

	if (RDP_GETOM_CYCLE_TYPE(rdpState.otherModes) == RDP_CYCLE_TYPE_FILL ||
      RDP_GETOM_CYCLE_TYPE(rdpState.otherModes) == RDP_CYCLE_TYPE_COPY)
	{
		rect->dsdx /= 4;
    //s /= 4;
    x2 += 1;
    y2 += 1;
  }
  
  int t1 = rglT1Usage(rdpState)? RGL_STRIP_TEX1:0;
  int t2 = (rect->tilenum < 7 && rglT2Usage(rdpState))? RGL_STRIP_TEX2:0;
  if (t1)
    rglPrepareRendering(1, rect->tilenum, y2-y1, 1);
  if (t2)
    rglPrepareRendering(1, rect->tilenum+1, y2-y1, 1);
  else if (!t1)
    rglPrepareRendering(0, 0, 0, 1);

  // TO BE REMOVED when we implement depth texture writing
  curRBuffer->flags |= RGL_RB_HASTRIANGLES;
    
  // TO CHECK should this before or after the rescaling above ?
//   s -= (rdpTiles[tilenum].sl << 8);
//   t -= (rdpTiles[tilenum].tl << 8);
//   if (/*!tile.ms && */tile.mask_s)
//     s &= (1<<tile.mask_s+10) - 1;
//   if (/*!tile.mt && */tile.mask_t)
//     t &= (1<<tile.mask_t+10) - 1;

	int height = (vi_control & 0x40) ? 479 : 239;
#define XSCALE(x) (float(x))
#define YSCALE(y) (float(y))
#define ZSCALE(z) (float(uint32_t(z))/0xffff)
#define SSCALE(s) (float(s)/(1 << 10))
#define TSCALE(s) (float(s)/(1 << 10))
// #define glTexCoord2f(s, t) printf("tex %g %g\n", s, t), glTexCoord2f(s, t)
// #define glVertex3f(s, t, z) printf("vert %g %g %g\n", s, t, z), glVertex3f(s, t, z)

  dx = x2 - x1;
  dy = y2 - y1;
  if (RDP_GETOM_Z_SOURCE_SEL(rdpState.otherModes))
    z = rdpState.primitiveZ;
  else
    z = 0xffff;
//   if (dump)
//     fprintf(stderr, "fillrect cycle %d\n", other_modes.cycle_type);

  rglStrip_t * strip = strips + nbStrips++;
  rglAssert(nbStrips < MAX_STRIPS);
  curChunk->nbStrips++;
  rglVertex_t * vtx = vtxs + nbVtxs;

  strip->flags = t1 | t2 | RGL_STRIP_ZBUFFER;
  strip->vtxs = vtx;
  strip->tilenum = tilenum;

  float s2, tr;
  s2 = s+int(rect->dsdx)*dx;
  tr = t+int(rect->dtdy)*dy;
  //LOG("%d %d\n", rect->dsdx, rect->dtdy);
	if (0 && RDP_GETOM_CYCLE_TYPE(rdpState.otherModes) < 2)
	{
    //if (rect->dsdx == (1<<10))
    {
      s += 1<<9;
      s2 -= 1<<9;
    }
    //if (rect->dtdy == (1<<10))
    {
      t += 1<<9;
      tr -= 1<<9;
    }
  }

  if (flip) {
    vtx->t = SSCALE(s2); vtx->s = TSCALE(t);
  } else {
    vtx->s = SSCALE(s2); vtx->t = TSCALE(t);
  }
  vtx->x = XSCALE(x2); vtx->y = YSCALE(y1); vtx->z = ZSCALE(z); vtx++->w = 1;
  if (flip) {
    vtx->t = SSCALE(s); vtx->s = TSCALE(t);
  } else {
    vtx->s = SSCALE(s); vtx->t = TSCALE(t);
  }
  vtx->x = XSCALE(x1); vtx->y = YSCALE(y1); vtx->z = ZSCALE(z); vtx++->w = 1;
  if (flip) {
    vtx->t = SSCALE(s2); vtx->s = TSCALE(tr);
  } else {
    vtx->s = SSCALE(s2); vtx->t = TSCALE(tr);
  }
  vtx->x = XSCALE(x2); vtx->y = YSCALE(y2); vtx->z = ZSCALE(z); vtx++->w = 1;
  if (flip) {
    vtx->t = SSCALE(s); vtx->s = TSCALE(tr);
  } else {
    vtx->s = SSCALE(s); vtx->t = TSCALE(tr);
  }
  vtx->x = XSCALE(x1); vtx->y = YSCALE(y2); vtx->z = ZSCALE(z); vtx++->w = 1;
  
  strip->nbVtxs = vtx - strip->vtxs;
  nbVtxs = vtx - vtxs;
#undef tile
}

void rglFillRectangle(rdpRect_t * rect)
{
  int x1,x2,y1,y2,z;
  int s, t;
  int dx, dy;

  rglPrepareRendering(0, 0, 0, 1);
  DUMP("fillrect curRBuffer->flags %x %x %x\n", curRBuffer->flags, curRBuffer->addressStart, rdpZbAddress);
//   if (/*(curRBuffer->flags & RGL_RB_DEPTH) &&*/
//       RDP_GETOM_CYCLE_TYPE(rdpState.otherModes) == RDP_CYCLE_TYPE_FILL &&
//       rect->xh-4 <= rdpState.clip.xh && rect->xl+8 >= rdpState.clip.xl &&
//       rect->yh-4 <= rdpState.clip.yh && rect->yl+8 >= rdpState.clip.yl
//   ) {
//     curChunk->flags |= RGL_CHUNK_CLEAR;
//     return;
//   }

	x1 = (rect->xh / 4);
	x2 = (rect->xl / 4);
	y1 = (rect->yh / 4);
	y2 = (rect->yl / 4);

	if (RDP_GETOM_CYCLE_TYPE(rdpState.otherModes) == RDP_CYCLE_TYPE_FILL ||
      RDP_GETOM_CYCLE_TYPE(rdpState.otherModes) == RDP_CYCLE_TYPE_COPY)
	{
		x2 += 1;
		y2 += 1;
  } else {
    //rglAssert(!(curRBuffer->flags & RGL_RB_DEPTH));
  // 		x2 -= 1;
// 		y2 -= 1;
	}

#define XSCALE(x) (float(x))
#define YSCALE(y) (float(y))
#define ZSCALE(z) (float(uint32_t(z))/0xffff)

  if (RDP_GETOM_Z_SOURCE_SEL(rdpState.otherModes))
    z = rdpState.primitiveZ;
  else
    z = 0xffff;
//   if (dump)
//     fprintf(stderr, "fillrect cycle %d\n", other_modes.cycle_type);

  rglStrip_t * strip = strips + nbStrips++;
  rglAssert(nbStrips < MAX_STRIPS);
  curChunk->nbStrips++;
  rglVertex_t * vtx = vtxs + nbVtxs;

  strip->flags = RGL_STRIP_ZBUFFER;
  strip->vtxs = vtx;
  
  vtx->x = XSCALE(x2); vtx->y = YSCALE(y1); vtx->z = ZSCALE(z); vtx++->w = 1;
  vtx->x = XSCALE(x1); vtx->y = YSCALE(y1); vtx->z = ZSCALE(z); vtx++->w = 1;
  vtx->x = XSCALE(x2); vtx->y = YSCALE(y2); vtx->z = ZSCALE(z); vtx++->w = 1;
  vtx->x = XSCALE(x1); vtx->y = YSCALE(y2); vtx->z = ZSCALE(z); vtx++->w = 1;
  
  strip->nbVtxs = vtx - strip->vtxs;
  nbVtxs = vtx - vtxs;
}

void rglTriangle(uint32_t w1, uint32_t w2, int shade, int texture, int zbuffer,
                 uint32_t * rdp_cmd)
{
	int tilenum = (w1 >> 16) & 0x7;
	int j;
	int xleft, xright, xleft_inc, xright_inc;
	int xstart, xend;
	int r, g, b, a, z, s, t, w;
	int dr, dg, db, da;
	int drdx = 0, dgdx = 0, dbdx = 0, dadx = 0, dzdx = 0, dsdx = 0, dtdx = 0, dwdx = 0;
	int drdy = 0, dgdy = 0, dbdy = 0, dady = 0, dzdy = 0, dsdy = 0, dtdy = 0, dwdy = 0;
	int drde = 0, dgde = 0, dbde = 0, dade = 0, dzde = 0, dsde = 0, dtde = 0, dwde = 0;
	int flip = (w1 & 0x800000) ? 1 : 0;

	int32_t yl, ym, yh;
	int32_t xl, xm, xh;
	int64_t dxldy, dxhdy, dxmdy;
	uint32_t w3, w4, w5, w6, w7, w8;

	uint32_t * shade_base = rdp_cmd + 8;
	uint32_t * texture_base = rdp_cmd + 8;
	uint32_t * zbuffer_base = rdp_cmd + 8;

  int t1 = (texture && rglT1Usage(rdpState))? RGL_STRIP_TEX1:0;
  int t2 = (texture && tilenum < 7 && rglT2Usage(rdpState))? RGL_STRIP_TEX2:0;
  if (t1)
    rglPrepareRendering(1, tilenum, 0, zbuffer);
  if (t2)
    rglPrepareRendering(1, tilenum+1, 0, zbuffer);
  else if (!t1)
    rglPrepareRendering(0, tilenum, 0, zbuffer);

  curRBuffer->flags |= RGL_RB_HASTRIANGLES;
    
	if (shade)
	{
		texture_base += 16;
		zbuffer_base += 16;
	}
	if (texture)
	{
		zbuffer_base += 16;
	}

	w3 = rdp_cmd[2];
	w4 = rdp_cmd[3];
	w5 = rdp_cmd[4];
	w6 = rdp_cmd[5];
	w7 = rdp_cmd[6];
	w8 = rdp_cmd[7];

	yl = (w1 & 0x3fff);
	ym = ((w2 >> 16) & 0x3fff);
	yh = ((w2 >>  0) & 0x3fff);
	xl = (int32_t)(w3);
	xh = (int32_t)(w5);
	xm = (int32_t)(w7);
	dxldy = (int32_t)(w4);
	dxhdy = (int32_t)(w6);
	dxmdy = (int32_t)(w8);

	if (yl & (0x800<<2)) yl |= 0xfffff000<<2;
	if (ym & (0x800<<2)) ym |= 0xfffff000<<2;
  if (yh & (0x800<<2)) yh |= 0xfffff000<<2;

  yh &= ~3;

	r = 0xff;	g = 0xff;	b = 0xff;	a = 0xff;	z = 0xffff0000;	s = 0;	t = 0;	w = 0x30000;
	dr = 0;		dg = 0;		db = 0;		da = 0;

	if (shade)
	{
		r    = (shade_base[0] & 0xffff0000) | ((shade_base[+4 ] >> 16) & 0x0000ffff);
		g    = ((shade_base[0 ] << 16) & 0xffff0000) | (shade_base[4 ] & 0x0000ffff);
		b    = (shade_base[1 ] & 0xffff0000) | ((shade_base[5 ] >> 16) & 0x0000ffff);
		a    = ((shade_base[1 ] << 16) & 0xffff0000) | (shade_base[5 ] & 0x0000ffff);
		drdx = (shade_base[2 ] & 0xffff0000) | ((shade_base[6 ] >> 16) & 0x0000ffff);
		dgdx = ((shade_base[2 ] << 16) & 0xffff0000) | (shade_base[6 ] & 0x0000ffff);
		dbdx = (shade_base[3 ] & 0xffff0000) | ((shade_base[7 ] >> 16) & 0x0000ffff);
		dadx = ((shade_base[3 ] << 16) & 0xffff0000) | (shade_base[7 ] & 0x0000ffff);
		drde = (shade_base[8 ] & 0xffff0000) | ((shade_base[12] >> 16) & 0x0000ffff);
		dgde = ((shade_base[8 ] << 16) & 0xffff0000) | (shade_base[12] & 0x0000ffff);
		dbde = (shade_base[9 ] & 0xffff0000) | ((shade_base[13] >> 16) & 0x0000ffff);
		dade = ((shade_base[9 ] << 16) & 0xffff0000) | (shade_base[13] & 0x0000ffff);
		drdy = (shade_base[10] & 0xffff0000) | ((shade_base[14] >> 16) & 0x0000ffff);
		dgdy = ((shade_base[10] << 16) & 0xffff0000) | (shade_base[14] & 0x0000ffff);
		dbdy = (shade_base[11] & 0xffff0000) | ((shade_base[15] >> 16) & 0x0000ffff);
		dady = ((shade_base[11] << 16) & 0xffff0000) | (shade_base[15] & 0x0000ffff);
	}
	if (texture)
	{
		s    = (texture_base[0 ] & 0xffff0000) | ((texture_base[4 ] >> 16) & 0x0000ffff);
		t    = ((texture_base[0 ] << 16) & 0xffff0000)	| (texture_base[4 ] & 0x0000ffff);
		w    = (texture_base[1 ] & 0xffff0000) | ((texture_base[5 ] >> 16) & 0x0000ffff);
		dsdx = (texture_base[2 ] & 0xffff0000) | ((texture_base[6 ] >> 16) & 0x0000ffff);
		dtdx = ((texture_base[2 ] << 16) & 0xffff0000)	| (texture_base[6 ] & 0x0000ffff);
		dwdx = (texture_base[3 ] & 0xffff0000) | ((texture_base[7 ] >> 16) & 0x0000ffff);
		dsde = (texture_base[8 ] & 0xffff0000) | ((texture_base[12] >> 16) & 0x0000ffff);
		dtde = ((texture_base[8 ] << 16) & 0xffff0000)	| (texture_base[12] & 0x0000ffff);
		dwde = (texture_base[9 ] & 0xffff0000) | ((texture_base[13] >> 16) & 0x0000ffff);
		dsdy = (texture_base[10] & 0xffff0000) | ((texture_base[14] >> 16) & 0x0000ffff);
		dtdy = ((texture_base[10] << 16) & 0xffff0000)	| (texture_base[14] & 0x0000ffff);
		dwdy = (texture_base[11] & 0xffff0000) | ((texture_base[15] >> 16) & 0x0000ffff);
	}
	if (zbuffer)
	{
    //rglAssert(!(curRBuffer->flags & RGL_RB_DEPTH));
  
		z    = zbuffer_base[0];
		dzdx = zbuffer_base[1];
		dzde = zbuffer_base[2];
		dzdy = zbuffer_base[3];
	}

  xh <<= 2;  xm <<= 2;  xl <<= 2;
  r <<= 2;  g <<= 2;  b <<= 2;  a <<= 2;
  dsde >>= 2;  dtde >>= 2;  dsdx >>= 2;  dtdx >>= 2;
  dzdx >>= 2;  dzde >>= 2;  dzdy >>= 2;
  dwdx >>= 2;  dwde >>= 2;  dwdy >>= 2;


// #define tile rdpTiles[tilenum]
//   s -= (rdpTiles[tilenum].sl << 8);
//   t -= (rdpTiles[tilenum].tl << 8);
//   if (/*!tile.ms && */tile.mask_s)
//     s &= (1<<tile.mask_s+10) - 1;
//   if (/*!tile.mt && */tile.mask_t)
//     t &= (1<<tile.mask_t+10) - 1;
// #undef tile
  

	xleft = xm;
	xright = xh;
	xleft_inc = dxmdy;
	xright_inc = dxhdy;

  while (yh<ym &&
         !((!flip && xleft < xright+0x10000) ||
            (flip && xleft > xright-0x10000))) {
    xleft += xleft_inc;    xright += xright_inc;
    s += dsde;    t += dtde;    w += dwde;
    r += drde;    g += dgde;    b += dbde;    a += dade;
    z += dzde;
    yh++;
  }

  j = ym-yh;
  //rglAssert(j >= 0);
#define XSCALE(x) (float(x)/(1<<18))
#define YSCALE(y) (float(y)/(1<<2))
#define ZSCALE(z) (RDP_GETOM_Z_SOURCE_SEL(rdpState.otherModes)? float(rdpState.primitiveZ)/0xffff : float(uint32_t(z))/0xffff0000)
#define WSCALE(z) (RDP_GETOM_PERSP_TEX_EN(rdpState.otherModes)? (float(uint32_t(z) + 0x10000)/0xffff0000) : 1.0f)
#define CSCALE(c) (((c)>0x3ff0000? 0x3ff0000:((c)<0? 0 : (c)))>>18)
#define _PERSP(w) ( RDP_GETOM_PERSP_TEX_EN(rdpState.otherModes)? (w) : 0x10000 )
#define PERSP(s, w) ( RDP_GETOM_PERSP_TEX_EN(rdpState.otherModes)? (((int64_t)(s) << 20) / (_PERSP(w)? _PERSP(w):1)) : ((s)>>11) )
#define SSCALE(s, _w) (float(PERSP(s, _w))/(1 << 10))
#define TSCALE(s, w) (float(PERSP(s, w))/(1 << 10))

  rglStrip_t * strip = strips + nbStrips++;
  rglAssert(nbStrips < MAX_STRIPS);
  curChunk->nbStrips++;
  rglVertex_t * vtx = vtxs + nbVtxs;

  strip->flags = (shade? RGL_STRIP_SHADE : 0) | t1 | t2
    | RGL_STRIP_ZBUFFER;
  //| (zbuffer? RGL_STRIP_ZBUFFER : 0);
  strip->vtxs = vtx;
  strip->tilenum = tilenum;
  
  int sw;
  if (j > 0)
  {
    int dx = (xleft-xright>>16);
    if ((!flip && xleft < xright) ||
         (flip/* && xleft > xright*/))
    {
      if (shade) {
        vtx->r = CSCALE(r+drdx*dx);
        vtx->g = CSCALE(g+dgdx*dx);
        vtx->b = CSCALE(b+dbdx*dx);
        vtx->a = CSCALE(a+dadx*dx);
      }
      if (texture) {
        vtx->s = SSCALE(s+dsdx*dx, w+dwdx*dx);
        vtx->t = TSCALE(t+dtdx*dx, w+dwdx*dx);
      }
      vtx->x = XSCALE(xleft);
      vtx->y = YSCALE(yh);
      vtx->z = ZSCALE(z+dzdx*dx);
      vtx->w = 1.0f/WSCALE(w+dwdx*dx);
      vtx++;
    }
    if ((!flip/* && xleft < xright*/) ||
         (flip && xleft > xright))
    {
      if (shade) {
        vtx->r = CSCALE(r);
        vtx->g = CSCALE(g);
        vtx->b = CSCALE(b);
        vtx->a = CSCALE(a);
      }
      if (texture) {
        vtx->s = SSCALE(s, w);
        vtx->t = TSCALE(t, w);
      }
      vtx->x = XSCALE(xright);
      vtx->y = YSCALE(yh);
      vtx->z = ZSCALE(z);
      vtx->w = 1.0f/WSCALE(w);
      vtx++;
    }
  }
  xleft += xleft_inc*j;  xright += xright_inc*j;
  s += dsde*j;  t += dtde*j;  w += dwde*j;
  r += drde*j;  g += dgde*j;  b += dbde*j;  a += dade*j;
  z += dzde*j;
  // render ...

	xleft = xl;
  
  //if (yl-ym > 0)
  {
    int dx = (xleft-xright>>16);
    if ((!flip && xleft <= xright) ||
        (flip/* && xleft >= xright*/))
    {
      if (shade) {
        vtx->r = CSCALE(r+drdx*dx);
        vtx->g = CSCALE(g+dgdx*dx);
        vtx->b = CSCALE(b+dbdx*dx);
        vtx->a = CSCALE(a+dadx*dx);
      }
      if (texture) {
        vtx->s = SSCALE(s+dsdx*dx, w+dwdx*dx);
        vtx->t = TSCALE(t+dtdx*dx, w+dwdx*dx);
      }
      vtx->x = XSCALE(xleft);
      vtx->y = YSCALE(ym);
      vtx->z = ZSCALE(z+dzdx*dx);
      vtx->w = 1.0f/WSCALE(w+dwdx*dx);
      vtx++;
    }
    if ((!flip/* && xleft <= xright*/) ||
        (flip && xleft >= xright))
    {
      if (shade) {
        vtx->r = CSCALE(r);
        vtx->g = CSCALE(g);
        vtx->b = CSCALE(b);
        vtx->a = CSCALE(a);
      }
      if (texture) {
        vtx->s = SSCALE(s, w);
        vtx->t = TSCALE(t, w);
      }
      vtx->x = XSCALE(xright);
      vtx->y = YSCALE(ym);
      vtx->z = ZSCALE(z);
      vtx->w = 1.0f/WSCALE(w);
      vtx++;
    }
  }
	xleft_inc = dxldy;
	xright_inc = dxhdy;

  j = yl-ym;
  //rglAssert(j >= 0);
  //j--; // ?
  xleft += xleft_inc*j;  xright += xright_inc*j;
  s += dsde*j;  t += dtde*j;  w += dwde*j;
  r += drde*j;  g += dgde*j;  b += dbde*j;  a += dade*j;
  z += dzde*j;

  while (yl>ym &&
         !((!flip && xleft < xright+0x10000) ||
            (flip && xleft > xright-0x10000))) {
    xleft -= xleft_inc;    xright -= xright_inc;
    s -= dsde;    t -= dtde;    w -= dwde;
    r -= drde;    g -= dgde;    b -= dbde;    a -= dade;
    z -= dzde;
    j--;
    yl--;
  }
  
  // render ...
  if (j >= 0) {
    int dx = (xleft-xright>>16);
    if ((!flip && xleft <= xright) ||
        (flip/* && xleft >= xright*/))
    {
      if (shade) {
        vtx->r = CSCALE(r+drdx*dx);
        vtx->g = CSCALE(g+dgdx*dx);
        vtx->b = CSCALE(b+dbdx*dx);
        vtx->a = CSCALE(a+dadx*dx);
      }
      if (texture) {
        vtx->s = SSCALE(s+dsdx*dx, w+dwdx*dx);
        vtx->t = TSCALE(t+dtdx*dx, w+dwdx*dx);
      }
      vtx->x = XSCALE(xleft);
      vtx->y = YSCALE(yl);
      vtx->z = ZSCALE(z+dzdx*dx);
      vtx->w = 1.0f/WSCALE(w+dwdx*dx);
      vtx++;
    }
    if ((!flip/* && xleft <= xright*/) ||
        (flip && xleft >= xright))
    {
      if (shade) {
        vtx->r = CSCALE(r);
        vtx->g = CSCALE(g);
        vtx->b = CSCALE(b);
        vtx->a = CSCALE(a);
      }
      if (texture) {
        vtx->s = SSCALE(s, w);
        vtx->t = TSCALE(t, w);
      }
      vtx->x = XSCALE(xright);
      vtx->y = YSCALE(yl);
      vtx->z = ZSCALE(z);
      vtx->w = 1.0f/WSCALE(w);
      vtx++;
    }
  }

  strip->nbVtxs = vtx - strip->vtxs;
  nbVtxs = vtx - vtxs;
}