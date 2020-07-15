#ifndef BSPATCH_H
#define BSPATCH_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

typedef long ssize_t;

/* ------------------------------------------------------------------------- */
/* -- Patch file magic number ---------------------------------------------- */

/** MUST be 8 bytes long! */
#define BSDIFF_CONFIG_MAGIC "MBSDIF43"

/* ------------------------------------------------------------------------- */
/* -- Slop size for temporary patch buffer --------------------------------- */

#define BSDIFF_PATCH_SLOP_SIZE 102400

/* ------------------------------------------------------------------------- */
/* -- Type definitions ----------------------------------------------------- */
typedef unsigned char u_char;

/*
  Patch file format:
  0        8       BSDIFF_CONFIG_MAGIC (see minibsdiff-config.h)
  8        8       X
  16       8       Y
  24       8       sizeof(newfile)
  32       X       control block
  32+X     Y       diff block
  32+X+Y   ???     extra block
  with control block a set of triples (x,y,z) meaning "add x bytes
  from oldfile to x bytes from the diff block; copy y bytes from the
  extra block; seek forwards in oldfile by z bytes".
*/

static off_t
offtin(u_char *buf)
{
  off_t y;

  y=buf[7]&0x7F;
  y=y*256;y+=buf[6];
  y=y*256;y+=buf[5];
  y=y*256;y+=buf[4];
  y=y*256;y+=buf[3];
  y=y*256;y+=buf[2];
  y=y*256;y+=buf[1];
  y=y*256;y+=buf[0];

  if(buf[7]&0x80) y=-y;

  return y;
}

uint8_t
bspatch_valid_header(u_char* patch, ssize_t patchsz)
{
  ssize_t newsize, ctrllen, datalen;

  if (patchsz < 32) return 0;

  /* Make sure magic and header fields are valid */
  if(memcmp(patch, BSDIFF_CONFIG_MAGIC, 8) != 0) return 0;

  ctrllen=offtin(patch+8);
  datalen=offtin(patch+16);
  newsize=offtin(patch+24);
  if((ctrllen<0) || (datalen<0) || (newsize<0))
    return 0;

  return 1;
}

ssize_t
bspatch_newsize(u_char* patch, ssize_t patchsz)
{
  if (!bspatch_valid_header(patch, patchsz)) return -1;
  return offtin(patch+24);
}

int
bspatch(u_char* oldp,  ssize_t oldsz,
        u_char* patch, ssize_t patchsz,
        u_char* newp,  ssize_t newsz)
{
  ssize_t newsize,ctrllen,datalen;
  u_char *ctrlblock, *diffblock, *extrablock;
  off_t oldpos,newpos;
  off_t ctrl[3];
  off_t i;

  /* Sanity checks */
  if (oldp == NULL || patch == NULL || newp == NULL) return -1;
  if (oldsz < 0    || patchsz < 0   || newsz < 0)    return -1;
  if (!bspatch_valid_header(patch, patchsz)) return -2;

  /* Read lengths from patch header */
  ctrllen=offtin(patch+8);
  datalen=offtin(patch+16);
  newsize=offtin(patch+24);
  if (newsize > newsz) return -1;

  /* Get pointers into the header metadata */
  ctrlblock  = patch+32;
  diffblock  = patch+32+ctrllen;
  extrablock = patch+32+ctrllen+datalen;

  /* Apply patch */
  oldpos=0;newpos=0;
  while(newpos<newsize) {
    /* Read control block */
    ctrl[0] = offtin(ctrlblock);
    ctrl[1] = offtin(ctrlblock+8);
    ctrl[2] = offtin(ctrlblock+16);
    ctrlblock += 24;

    /* Sanity check */
    if(newpos+ctrl[0]>newsize)
      return -3; /* Corrupt patch */

    /* Read diff string */
    memcpy(newp + newpos, diffblock, ctrl[0]);
    diffblock += ctrl[0];

    /* Add old data to diff string */
    for(i=0;i<ctrl[0];i++)
      if((oldpos+i>=0) && (oldpos+i<oldsz))
        newp[newpos+i]+=oldp[oldpos+i];

    /* Adjust pointers */
    newpos+=ctrl[0];
    oldpos+=ctrl[0];

    /* Sanity check */
    if(newpos+ctrl[1]>newsize)
      return -3; /* Corrupt patch */

    /* Read extra string */
    memcpy(newp + newpos, extrablock, ctrl[1]);
    extrablock += ctrl[1];

    /* Adjust pointers */
    newpos+=ctrl[1];
    oldpos+=ctrl[2];
  };

  return 0;
}
#endif