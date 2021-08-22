 #include "../param.h"
 #include "../conf.h"
 #include "../inode.h"
 #include "../user.h"
 #include "../buf.h"
 #include "../systm.h"
 
 /* Bmap defines the structure of file system storage
  * by returning the physical block number on a device given the
  * inode and the logical block number in a file.
  * When convenient, it also leaves the physical
  * block number of the next block of the file in rablock
  * for use in read-ahead.
  */
 bmap(ip, bn)
 struct inode *ip;
 int bn;
 {
         register *bp, *bap, nb;
         int *nbp, d, i;
 
         d = ip-&gt;i_dev;
         if(bn &amp; ~077777) {
                 u.u_error = EFBIG;
                 return(0);
         }
         if((ip-&gt;i_mode&amp;ILARG) == 0) {
 
                 /* small file algorithm */
 
                 if((bn &amp; ~7) != 0) {
 
                         /* convert small to large */
 
                         if ((bp = alloc(d)) == NULL)
                                 return(NULL);
                         bap = bp-&gt;b_addr;
                         for(i=0; i&lt;8; i++) {
                                 *bap++ = ip-&gt;i_addr[i];
                                 ip-&gt;i_addr[i] = 0;
                         }
                         ip-&gt;i_addr[0] = bp-&gt;b_blkno;
                         bdwrite(bp);
                         ip-&gt;i_mode =| ILARG;
                         goto large;
                 }
                 nb = ip-&gt;i_addr[bn];
                 if(nb == 0 &amp;&amp; (bp = alloc(d)) != NULL) {
                         bdwrite(bp);
                         nb = bp-&gt;b_blkno;
                         ip-&gt;i_addr[bn] = nb;
                         ip-&gt;i_flag =| IUPD;
                 }
                 rablock = 0;
                 if (bn&lt;7)
                         rablock = ip-&gt;i_addr[bn+1];
                 return(nb);
         }
 
         /* large file algorithm */
 
     large:
         i = bn&gt;&gt;8;
         if(bn &amp; 0174000)
                 i = 7;
         if((nb=ip-&gt;i_addr[i]) == 0) {
                 ip-&gt;i_flag =| IUPD;
                 if ((bp = alloc(d)) == NULL)
                         return(NULL);
                 ip-&gt;i_addr[i] = bp-&gt;b_blkno;
         } else
                 bp = bread(d, nb);
         bap = bp-&gt;b_addr;
 
         /* "huge" fetch of double indirect block */
 
         if(i == 7) {
                 i = ((bn&gt;&gt;8) &amp; 0377) - 7;
                 if((nb=bap[i]) == 0) {
                         if((nbp = alloc(d)) == NULL) {
                                 brelse(bp);
                                 return(NULL);
                         }
                         bap[i] = nbp-&gt;b_blkno;
                         bdwrite(bp);
                 } else {
                         brelse(bp);
                         nbp = bread(d, nb);
                 }
                 bp = nbp;
                 bap = bp-&gt;b_addr;
         }
 
         /* normal indirect fetch */
 
         i = bn &amp; 0377;
         if((nb=bap[i]) == 0 &amp;&amp; (nbp = alloc(d)) != NULL) {
                 nb = nbp-&gt;b_blkno;
                 bap[i] = nb;
                 bdwrite(nbp);
                 bdwrite(bp);
         } else
                 brelse(bp);
         rablock = 0;
         if(i &lt; 255)
                 rablock = bap[i+1];
         return(nb);
 }
 /* ---------------------------       */
 
 /*
  * Pass back  c  to the user at his location u_base;
  * update u_base, u_count, and u_offset.  Return -1
  * on the last character of the user's read.
  * u_base is in the user address space unless u_segflg is set.
  */
 passc(c)
 char c;
 {
 
         if(u.u_segflg)
                 *u.u_base = c; else
                 if(subyte(u.u_base, c) &lt; 0) {
                         u.u_error = EFAULT;
                         return(-1);
                 }
         u.u_count--;
         if(++u.u_offset[1] == 0)
                 u.u_offset[0]++;
         u.u_base++;
         return(u.u_count == 0? -1: 0);
 }
 /* ---------------------------       */
 
 /*
  * Pick up and return the next character from the user's
  * write call at location u_base;
  * update u_base, u_count, and u_offset.  Return -1
  * when u_count is exhausted.  u_base is in the user's
  * address space unless u_segflg is set.
  */
 cpass()
 {
         register c;
 
         if(u.u_count == 0)
                 return(-1);
         if(u.u_segflg)
                 c = *u.u_base; else
                 if((c=fubyte(u.u_base)) &lt; 0) {
                         u.u_error = EFAULT;
                         return(-1);
                 }
         u.u_count--;
         if(++u.u_offset[1] == 0)
                 u.u_offset[0]++;
         u.u_base++;
         return(c&amp;0377);
 }
 /* ---------------------------       */
 
 /*
  * Routine which sets a user error; placed in
  * illegal entries in the bdevsw and cdevsw tables.
  */
 nodev()
 {
 
         u.u_error = ENODEV;
 }
 /* ---------------------------       */
 
 /*
  * Null routine; placed in insignificant entries
  * in the bdevsw and cdevsw tables.
  */
 nulldev()
 {
 }
 /* ---------------------------       */
 
 /*
  * copy count words from from to to.
  */
 bcopy(from, to, count)
 int *from, *to;
 {
         register *a, *b, c;
 
         a = from;
         b = to;
         c = count;
         do
                 *b++ = *a++;
         while(--c);
 }
 /* ---------------------------       */




