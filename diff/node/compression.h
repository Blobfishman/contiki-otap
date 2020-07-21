#ifndef COMPRESSION_H
#define COMPRESSION_H

#include "config.h"
#include "fs-dummy.h"

// #define MIN(a,b) (((a)<(b))?(a):(b))
// #define MAX(a,b) (((a)>(b))?(a):(b))

unsigned int fastlz_decompress(int fin, unsigned int length, int fout,
                               unsigned int maxout);

/*
 * Always check for bound when decompressing.
 * Generally it is best to leave it defined.
 */
#define FASTLZ_SAFE

#define FASTLZ_EXPECT_CONDITIONAL(c) (c)
#define FASTLZ_UNEXPECT_CONDITIONAL(c) (c)

#define FASTLZ_INLINE

/*
 * Prevent accessing more than 8-bit at once, except on x86 architectures.
 */
#define FASTLZ_STRICT_ALIGN

/*
 * FIXME: use preprocessor magic to set this on different platforms!
 */
typedef unsigned char flzuint8;
typedef unsigned short flzuint16;
typedef unsigned int flzuint32;

#define FASTLZ_LEVEL 2

static unsigned char buf[BUF_SIZE];

unsigned int fastlz_decompress(int fin, unsigned int length, int fout,
                               unsigned int maxout) {
  flzuint32 ip = 0;
  flzuint32 ip_limit = length;
  flzuint32 op = 0; // output;
  flzuint32 op_limit = maxout;
  fsd_read(fin, buf, 1);
  flzuint32 ctrl = ((unsigned int)buf[0]) & 31;
  flzuint32 ref, len, ofs;
  unsigned int bytes;
  ip++;
  int loop = 1;

  do {
    ref = op;
    len = ctrl >> 5;
    ofs = (ctrl & 31) << 8;
    if (ctrl >= 32) {
      len--;
      ref -= ofs;

      if (len == 7 - 1) {
        fsd_read(fin, buf, 1);
        ip++;
        len += (unsigned char)buf[0];
      }
      fsd_read(fin, buf, 1);
      ref -= (unsigned char)buf[0];
      ip++;

      if (FASTLZ_UNEXPECT_CONDITIONAL(op + len + 3 > op_limit))
        return 0;

      if (FASTLZ_UNEXPECT_CONDITIONAL(ref - 1 < 0))
        return 0;

      if (FASTLZ_EXPECT_CONDITIONAL(ip < ip_limit)) {
        fsd_read(fin, &ctrl, 1);
        ip++;
      } else
        loop = 0;

      if (ref == op) {

        /* optimize copy for a run */
        fsd_seek(fout, ref - 1, FSD_SEEK_SET);
        fsd_read(fout, buf, sizeof(unsigned char));
        fsd_seek(fout, op, FSD_SEEK_SET);

        len += 3;
        op += len;
        memset(buf, buf[0], BUF_SIZE);

        while (len > BUF_SIZE) {
          fsd_write(fout, buf, BUF_SIZE);
          len -= BUF_SIZE;
        }
        if (len > 0) {
          fsd_write(fout, buf, len);
        }
      } else {
#if !defined(FASTLZ_STRICT_ALIGN)
        const flzuint16 *p;
        flzuint16 *q;
#endif
        /* copy from reference */
        ref--;
        /**/
        if ((op - ref) > 3) {
          if (BUF_SIZE < 3) {
            fsd_seek(fout, ref, FSD_SEEK_SET);
            fsd_read(fout, buf, BUF_SIZE);
            fsd_seek(fout, op, FSD_SEEK_SET);
            fsd_write(fout, buf, BUF_SIZE);

            ref += BUF_SIZE;
            op += BUF_SIZE;
            fsd_seek(fout, ref, FSD_SEEK_SET);
            fsd_read(fout, buf, 3 - BUF_SIZE);
            fsd_seek(fout, op, FSD_SEEK_SET);
            fsd_write(fout, buf, 3 - BUF_SIZE);
            ref += 3 - BUF_SIZE;
            op += 3 - BUF_SIZE;
          } else {
            fsd_seek(fout, ref, FSD_SEEK_SET);
            fsd_read(fout, buf, BUF_SIZE);
            fsd_seek(fout, op, FSD_SEEK_SET);
            fsd_write(fout, buf, BUF_SIZE);

            op += 3;
            ref += 3;
          }
        } else {
          fsd_seek(fout, ref++, FSD_SEEK_SET);
          fsd_read(fout, buf, 1);
          fsd_seek(fout, op++, FSD_SEEK_SET);
          fsd_write(fout, buf, 1);

          fsd_seek(fout, ref++, FSD_SEEK_SET);
          fsd_read(fout, buf, 1);
          fsd_seek(fout, op++, FSD_SEEK_SET);
          fsd_write(fout, buf, 1);

          fsd_seek(fout, ref++, FSD_SEEK_SET);
          fsd_read(fout, buf, 1);
          fsd_seek(fout, op++, FSD_SEEK_SET);
          fsd_write(fout, buf, 1);
        }

#if !defined(FASTLZ_STRICT_ALIGN)
        /* copy a byte, so that now it's word aligned */
        if (len & 1) {
          *op++ = *ref++;
          len--;
        }

        /* copy 16-bit at once */
        q = (flzuint16 *)op;
        op += len;
        p = (const flzuint16 *)ref;
        for (len >>= 1; len > 4; len -= 4) {
          *q++ = *p++;
          *q++ = *p++;
          *q++ = *p++;
          *q++ = *p++;
        }
        for (; len; --len)
          *q++ = *p++;
#else
        while (len > 0) {
          bytes = MIN(BUF_SIZE, len);
          if ((op - ref) < bytes)
            bytes = op - ref;

          fsd_seek(fout, ref, FSD_SEEK_SET);
          fsd_read(fout, buf, bytes);
          fsd_seek(fout, op, FSD_SEEK_SET);
          fsd_write(fout, buf, bytes);

          op += bytes;
          ref += bytes;
          len -= bytes;
        }
#endif
      }
    } else {
      ctrl++;
#ifdef FASTLZ_SAFE
      if (FASTLZ_UNEXPECT_CONDITIONAL(op + ctrl > op_limit))
        return 0;
      if (FASTLZ_UNEXPECT_CONDITIONAL(ip + ctrl > ip_limit))
        return 0;
#endif

      fsd_read(fin, buf, 1);
      fsd_write(fout, buf, 1);
      op++;
      ip++;

      ctrl--;
      op += ctrl;
      ip += ctrl;
      while (ctrl > 0) {
        fsd_read(fin, buf, MIN(BUF_SIZE, ctrl));
        fsd_write(fout, buf, MIN(BUF_SIZE, ctrl));
        ctrl = ctrl - MIN(BUF_SIZE, ctrl);
      }
      loop = FASTLZ_EXPECT_CONDITIONAL(ip < ip_limit);
      if (loop) {
        fsd_read(fin, &ctrl, 1);
        ip++;
      }
    }
  } while (FASTLZ_EXPECT_CONDITIONAL(loop));

  return op;
}

#endif