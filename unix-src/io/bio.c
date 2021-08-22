 /*
  */
 
 #include "../param.h"
 #include "../user.h"
 #include "../buf.h"
 #include "../conf.h"
 #include "../systm.h"
 #include "../proc.h"
 #include "../seg.h"
 
 /*
  * This is the set of buffers proper, whose heads
  * were declared in buf.h.  There can exist buffer
  * headers not pointing here that are used purely
  * as arguments to the I/O routines to describe
  * I/O to be done-- e.g. swbuf, just below, for
  * swapping.
  */
 char    buffers[NBUF][514];
 struct  buf     swbuf;
 
 /*
  * Declarations of the tables for the magtape devices;
  * see bdwrite.
  */
 int     tmtab;
 int     httab;
 
 /*
  * The following several routines allocate and free
  * buffers with various side effects.  In general the
  * arguments to an allocate routine are a device and
  * a block number, and the value is a pointer to
  * to the buffer header; the buffer is marked "busy"
  * so that no on else can touch it.  If the block was
  * already in core, no I/O need be done; if it is
  * already busy, the process waits until it becomes free.
  * The following routines allocate a buffer:
  *      getblk
  *      bread
  *      breada
  * Eventually the buffer must be released, possibly with the
  * side effect of writing it out, by using one of
  *      bwrite
  *      bdwrite
  *      bawrite
  *      brelse
  */
 
 /*
  * Read in (if necessary) the block and return a buffer pointer.
  */
 bread(dev, blkno)
 {
         register struct buf *rbp;
 
         rbp = getblk(dev, blkno);
         if (rbp-&gt;b_flags&amp;B_DONE)
                 return(rbp);
         rbp-&gt;b_flags =| B_READ;
         rbp-&gt;b_wcount = -256;
         (*bdevsw[dev.d_major].d_strategy)(rbp);
         iowait(rbp);
         return(rbp);
 }
 /* ---------------------------       */
 
 /*
  * Read in the block, like bread, but also start I/O on the
  * read-ahead block (which is not allocated to the caller)
  */
 breada(adev, blkno, rablkno)
 {
         register struct buf *rbp, *rabp;
         register int dev;
 
         dev = adev;
         rbp = 0;
         if (!incore(dev, blkno)) {
                 rbp = getblk(dev, blkno);
                 if ((rbp-&gt;b_flags&amp;B_DONE) == 0) {
                         rbp-&gt;b_flags =| B_READ;
                         rbp-&gt;b_wcount = -256;
                         (*bdevsw[adev.d_major].d_strategy)(rbp);
                 }
         }
         if (rablkno &amp;&amp; !incore(dev, rablkno)) {
                 rabp = getblk(dev, rablkno);
                 if (rabp-&gt;b_flags &amp; B_DONE)
                         brelse(rabp);
                 else {
                         rabp-&gt;b_flags =| B_READ|B_ASYNC;
                         rabp-&gt;b_wcount = -256;
                         (*bdevsw[adev.d_major].d_strategy)(rabp);
                 }
         }
         if (rbp==0)
                 return(bread(dev, blkno));
         iowait(rbp);
         return(rbp);
 }
 /* ---------------------------       */
 
 /*
  * Write the buffer, waiting for completion.
  * Then release the buffer.
  */
 bwrite(bp)
 struct buf *bp;
 {
         register struct buf *rbp;
         register flag;
 
         rbp = bp;
         flag = rbp-&gt;b_flags;
         rbp-&gt;b_flags =&amp; ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
         rbp-&gt;b_wcount = -256;
         (*bdevsw[rbp-&gt;b_dev.d_major].d_strategy)(rbp);
         if ((flag&amp;B_ASYNC) == 0) {
                 iowait(rbp);
                 brelse(rbp);
         } else if ((flag&amp;B_DELWRI)==0)
                 geterror(rbp);
 }
 /* ---------------------------       */
 
 /*
  * Release the buffer, marking it so that if it is grabbed
  * for another purpose it will be written out before being
  * given up (e.g. when writing a partial block where it is
  * assumed that another write for the same block will soon follow).
  * This can't be done for magtape, since writes must be done
  * in the same order as requested.
  */
 bdwrite(bp)
 struct buf *bp;
 {
         register struct buf *rbp;
         register struct devtab *dp;
 
         rbp = bp;
         dp = bdevsw[rbp-&gt;b_dev.d_major].d_tab;
         if (dp == &amp;tmtab || dp == &amp;httab)
                 bawrite(rbp);
         else {
                 rbp-&gt;b_flags =| B_DELWRI | B_DONE;
                 brelse(rbp);
         }
 }
 /* ---------------------------       */
 
 /*
  * Release the buffer, start I/O on it, but don't wait for completion.
  */
 bawrite(bp)
 struct buf *bp;
 {
         register struct buf *rbp;
 
         rbp = bp;
         rbp-&gt;b_flags =| B_ASYNC;
         bwrite(rbp);
 }
 /* ---------------------------       */
 
 /* release the buffer, with no I/O implied.
  */
 brelse(bp)
 struct buf *bp;
 {
         register struct buf *rbp, **backp;
         register int sps;
 
         rbp = bp;
         if (rbp-&gt;b_flags&amp;B_WANTED)
                 wakeup(rbp);
         if (bfreelist.b_flags&amp;B_WANTED) {
                 bfreelist.b_flags =&amp; ~B_WANTED;
                 wakeup(&amp;bfreelist);
         }
         if (rbp-&gt;b_flags&amp;B_ERROR)
                 rbp-&gt;b_dev.d_minor = -1;  /* no assoc. on error */
         backp = &amp;bfreelist.av_back;
         sps = PS-&gt;integ;
         spl6();
         rbp-&gt;b_flags =&amp; ~(B_WANTED|B_BUSY|B_ASYNC);
         (*backp)-&gt;av_forw = rbp;
         rbp-&gt;av_back = *backp;
         *backp = rbp;
         rbp-&gt;av_forw = &amp;bfreelist;
         PS-&gt;integ = sps;
 }
 /* ---------------------------       */
 
 /* See if the block is associated with some buffer
  * (mainly to avoid getting hung up on a wait in breada)
  */
 incore(adev, blkno)
 {
         register int dev;
         register struct buf *bp;
         register struct devtab *dp;
 
         dev = adev;
         dp = bdevsw[adev.d_major].d_tab;
         for (bp=dp-&gt;b_forw; bp != dp; bp = bp-&gt;b_forw)
                 if (bp-&gt;b_blkno==blkno &amp;&amp; bp-&gt;b_dev==dev)
                         return(bp);
         return(0);
 }
 /* ---------------------------       */
 
 /* Assign a buffer for the given block.  If the appropriate
  * block is already associated, return it; otherwise search
  * for the oldest non-busy buffer and reassign it.
  * When a 512-byte area is wanted for some random reason
  * (e.g. during exec, for the user arglist) getblk can be called
  * with device NODEV to avoid unwanted associativity.
  */
 getblk(dev, blkno)
 {
         register struct buf *bp;
         register struct devtab *dp;
         extern lbolt;
 
         if(dev.d_major &gt;= nblkdev)
                 panic("blkdev");
 
     loop:
         if (dev &lt; 0)
                 dp = &amp;bfreelist;
         else {
                 dp = bdevsw[dev.d_major].d_tab;
                 if(dp == NULL)
                         panic("devtab");
                 for (bp=dp-&gt;b_forw; bp != dp; bp = bp-&gt;b_forw) {
                         if (bp-&gt;b_blkno!=blkno || bp-&gt;b_dev!=dev)
                                 continue;
                         spl6();
                         if (bp-&gt;b_flags&amp;B_BUSY) {
                                 bp-&gt;b_flags =| B_WANTED;
                                 sleep(bp, PRIBIO);
                                 spl0();
                                 goto loop;
                         }
                         spl0();
                         notavail(bp);
                         return(bp);
                 }
         }
         spl6();
         if (bfreelist.av_forw == &amp;bfreelist) {
                 bfreelist.b_flags =| B_WANTED;
                 sleep(&amp;bfreelist, PRIBIO);
                 spl0();
                 goto loop;
         }
         spl0();
         notavail(bp = bfreelist.av_forw);
         if (bp-&gt;b_flags &amp; B_DELWRI) {
                 bp-&gt;b_flags =| B_ASYNC;
                 bwrite(bp);
                 goto loop;
         }
         bp-&gt;b_flags = B_BUSY | B_RELOC;
         bp-&gt;b_back-&gt;b_forw = bp-&gt;b_forw;
         bp-&gt;b_forw-&gt;b_back = bp-&gt;b_back;
         bp-&gt;b_forw = dp-&gt;b_forw;
         bp-&gt;b_back = dp;
         dp-&gt;b_forw-&gt;b_back = bp;
         dp-&gt;b_forw = bp;
         bp-&gt;b_dev = dev;
         bp-&gt;b_blkno = blkno;
         return(bp);
 }
 /* ---------------------------       */
 
 /* Wait for I/O completion on the buffer; return errors
  * to the user.
  */
 iowait(bp)
 struct buf *bp;
 {
         register struct buf *rbp;
 
         rbp = bp;
         spl6();
         while ((rbp-&gt;b_flags&amp;B_DONE)==0)
                 sleep(rbp, PRIBIO);
         spl0();
         geterror(rbp);
 }
 /* ---------------------------       */
 
 /* Unlink a buffer from the available list and mark it busy.
  * (internal interface)
  */
 notavail(bp)
 struct buf *bp;
 {
         register struct buf *rbp;
         register int sps;
 
         rbp = bp;
         sps = PS-&gt;integ;
         spl6();
         rbp-&gt;av_back-&gt;av_forw = rbp-&gt;av_forw;
         rbp-&gt;av_forw-&gt;av_back = rbp-&gt;av_back;
         rbp-&gt;b_flags =| B_BUSY;
         PS-&gt;integ = sps;
 }
 /* ---------------------------       */
 
 /* Mark I/O complete on a buffer, release it if I/O is asynchronous,
  * and wake up anyone waiting for it.
  */
 iodone(bp)
 struct buf *bp;
 {
         register struct buf *rbp;
 
         rbp = bp;
         if(rbp-&gt;b_flags&amp;B_MAP)
                 mapfree(rbp);
         rbp-&gt;b_flags =| B_DONE;
         if (rbp-&gt;b_flags&amp;B_ASYNC)
                 brelse(rbp);
         else {
                 rbp-&gt;b_flags =&amp; ~B_WANTED;
                 wakeup(rbp);
         }
 }
 /* ---------------------------       */
 
 /* Zero the core associated with a buffer.
  */
 clrbuf(bp)
 int *bp;
 {
         register *p;
         register c;
 
         p = bp-&gt;b_addr;
         c = 256;
         do
                 *p++ = 0;
         while (--c);
 }
 /* ---------------------------       */
 
 /* Initialize the buffer I/O system by freeing
  * all buffers and setting all device buffer lists to empty.
  */
 binit()
 {
         register struct buf *bp;
         register struct devtab *dp;
         register int i;
         struct bdevsw *bdp;
 
         bfreelist.b_forw = bfreelist.b_back =
             bfreelist.av_forw = bfreelist.av_back = &amp;bfreelist;
         for (i=0; i&lt;NBUF; i++) {
                 bp = &amp;buf[i];
                 bp-&gt;b_dev = -1;
                 bp-&gt;b_addr = buffers[i];
                 bp-&gt;b_back = &amp;bfreelist;
                 bp-&gt;b_forw = bfreelist.b_forw;
                 bfreelist.b_forw-&gt;b_back = bp;
                 bfreelist.b_forw = bp;
                 bp-&gt;b_flags = B_BUSY;
                 brelse(bp);
         }
         i = 0;
         for (bdp = bdevsw; bdp-&gt;d_open; bdp++) {
                 dp = bdp-&gt;d_tab;
                 if(dp) {
                         dp-&gt;b_forw = dp;
                         dp-&gt;b_back = dp;
                 }
                 i++;
         }
         nblkdev = i;
 }
 /* ---------------------------       */
 
 /* Device start routine for disks
  * and other devices that have the register
  * layout of the older DEC controllers (RF, RK, RP, TM)
  */
 #define IENABLE 0100
 #define WCOM    02
 #define RCOM    04
 #define GO      01
 devstart(bp, devloc, devblk, hbcom)
 struct buf *bp;
 int *devloc;
 {
         register int *dp;
         register struct buf *rbp;
         register int com;
 
         dp = devloc;
         rbp = bp;
         *dp = devblk;                   /* block address */
         *--dp = rbp-&gt;b_addr;            /* buffer address */
         *--dp = rbp-&gt;b_wcount;          /* word count */
         com = (hbcom&lt;&lt;8) | IENABLE | GO |
                 ((rbp-&gt;b_xmem &amp; 03) &lt;&lt; 4);
         if (rbp-&gt;b_flags&amp;B_READ)        /* command + x-mem */
                 com =| RCOM;
         else
                 com =| WCOM;
         *--dp = com;
 }
 /* ---------------------------       */
 
 /* startup routine for RH controllers. */
 #define RHWCOM  060
 #define RHRCOM  070
 
 rhstart(bp, devloc, devblk, abae)
 struct buf *bp;
 int *devloc, *abae;
 {
         register int *dp;
         register struct buf *rbp;
         register int com;
 
         dp = devloc;
         rbp = bp;
         if(cputype == 70)
                 *abae = rbp-&gt;b_xmem;
         *dp = devblk;                   /* block address */
         *--dp = rbp-&gt;b_addr;            /* buffer address */
         *--dp = rbp-&gt;b_wcount;          /* word count */
         com = IENABLE | GO |
                 ((rbp-&gt;b_xmem &amp; 03) &lt;&lt; 8);
         if (rbp-&gt;b_flags&amp;B_READ)        /* command + x-mem */
                 com =| RHRCOM; else
                 com =| RHWCOM;
         *--dp = com;
 }
 /* ---------------------------       */
 
 /*
  * 11/70 routine to allocate the
  * UNIBUS map and initialize for
  * a unibus device.
  * The code here and in
  * rhstart assumes that an rh on an 11/70
  * is an rh70 and contains 22 bit addressing.
  */
 int     maplock;
 mapalloc(abp)
 struct buf *abp;
 {
         register i, a;
         register struct buf *bp;
 
         if(cputype != 70)
                 return;
         spl6();
         while(maplock&amp;B_BUSY) {
                 maplock =| B_WANTED;
                 sleep(&amp;maplock, PSWP);
         }
         maplock =| B_BUSY;
         spl0();
         bp = abp;
         bp-&gt;b_flags =| B_MAP;
         a = bp-&gt;b_xmem;
         for(i=16; i&lt;32; i=+2)
                 UBMAP-&gt;r[i+1] = a;
         for(a++; i&lt;48; i=+2)
                 UBMAP-&gt;r[i+1] = a;
         bp-&gt;b_xmem = 1;
 }
 /* ---------------------------       */
 
 mapfree(bp)
 struct buf *bp;
 {
 
         bp-&gt;b_flags =&amp; ~B_MAP;
         if(maplock&amp;B_WANTED)
                 wakeup(&amp;maplock);
         maplock = 0;
 }
 /* ---------------------------       */
 
 /*
  * swap I/O
  */
 swap(blkno, coreaddr, count, rdflg)
 {
         register int *fp;
 
         fp = &amp;swbuf.b_flags;
         spl6();
         while (*fp&amp;B_BUSY) {
                 *fp =| B_WANTED;
                 sleep(fp, PSWP);
         }
         *fp = B_BUSY | B_PHYS | rdflg;
         swbuf.b_dev = swapdev;
         swbuf.b_wcount = - (count&lt;&lt;5);  /* 32 w/block */
         swbuf.b_blkno = blkno;
         swbuf.b_addr = coreaddr&lt;&lt;6;     /* 64 b/block */
         swbuf.b_xmem = (coreaddr&gt;&gt;10) &amp; 077;
         (*bdevsw[swapdev&gt;&gt;8].d_strategy)(&amp;swbuf);
         spl6();
         while((*fp&amp;B_DONE)==0)
                 sleep(fp, PSWP);
         if (*fp&amp;B_WANTED)
                 wakeup(fp);
         spl0();
         *fp =&amp; ~(B_BUSY|B_WANTED);
         return(*fp&amp;B_ERROR);
 }
 /* ---------------------------       */
 
 /*
  * make sure all write-behind blocks
  * on dev (or NODEV for all)
  * are flushed out.
  * (from umount and update)
  */
 bflush(dev)
 {
         register struct buf *bp;
 
 loop:
         spl6();
         for (bp = bfreelist.av_forw; bp != &amp;bfreelist; bp = bp-&gt;av_forw) {
 
                 if (bp-&gt;b_flags&amp;B_DELWRI &amp;&amp; (dev == NODEV||dev==bp-&gt;b_dev)) {
                         bp-&gt;b_flags =| B_ASYNC;
                         notavail(bp);
                         bwrite(bp);
                         goto loop;
                 }
         }
         spl0();
 }
 /* ---------------------------       */
 
 /*
  * Raw I/O. The arguments are
  *      The strategy routine for the device
  *      A buffer, which will always be a special buffer
  *        header owned exclusively by the device for this purpose
  *      The device number
  *      Read/write flag
  * Essentially all the work is computing physical addresses and
  * validating them.
  */
 physio(strat, abp, dev, rw)
 struct buf *abp;
 int (*strat)();
 {
         register struct buf *bp;
         register char *base;
         register int nb;
         int ts;
 
         bp = abp;
         base = u.u_base;
         /*
          * Check odd base, odd count, and address wraparound
          */
         if (base&amp;01 || u.u_count&amp;01 || base&gt;=base+u.u_count)
                 goto bad;
         ts = (u.u_tsize+127) &amp; ~0177;
         if (u.u_sep)
                 ts = 0;
         nb = (base&gt;&gt;6) &amp; 01777;
         /*
          * Check overlap with text. (ts and nb now
          * in 64-byte clicks)
          */
         if (nb &lt; ts)
                 goto bad;
         /*
          * Check that transfer is either entirely in the
          * data or in the stack: that is, either
          * the end is in the data or the start is in the stack
          * (remember wraparound was already checked).
          */
         if ((((base+u.u_count)&gt;&gt;6)&amp;01777) &gt;= ts+u.u_dsize
             &amp;&amp; nb &lt; 1024-u.u_ssize)
                 goto bad;
         spl6();
         while (bp-&gt;b_flags&amp;B_BUSY) {
                 bp-&gt;b_flags =| B_WANTED;
                 sleep(bp, PRIBIO);
         }
         bp-&gt;b_flags = B_BUSY | B_PHYS | rw;
         bp-&gt;b_dev = dev;
         /*
          * Compute physical address by simulating
          * the segmentation hardware.
          */
         bp-&gt;b_addr = base&amp;077;
         base = (u.u_sep? UDSA: UISA)-&gt;r[nb&gt;&gt;7] + (nb&amp;0177);
         bp-&gt;b_addr =+ base&lt;&lt;6;
         bp-&gt;b_xmem = (base&gt;&gt;10) &amp; 077;
         bp-&gt;b_blkno = lshift(u.u_offset, -9);
         bp-&gt;b_wcount = -((u.u_count&gt;&gt;1) &amp; 077777);
         bp-&gt;b_error = 0;
         u.u_procp-&gt;p_flag =| SLOCK;
         (*strat)(bp);
         spl6();
         while ((bp-&gt;b_flags&amp;B_DONE) == 0)
                 sleep(bp, PRIBIO);
         u.u_procp-&gt;p_flag =&amp; ~SLOCK;
         if (bp-&gt;b_flags&amp;B_WANTED)
                 wakeup(bp);
         spl0();
         bp-&gt;b_flags =&amp; ~(B_BUSY|B_WANTED);
         u.u_count = (-bp-&gt;b_resid)&lt;&lt;1;
         geterror(bp);
         return;
     bad:
         u.u_error = EFAULT;
 }
 /* ---------------------------       */
 
 /*
  * Pick up the device's error number and pass it to the user;
  * if there is an error but the number is 0 set a generalized
  * code.  Actually the latter is always true because devices
  * don't yet return specific errors.
  */
 geterror(abp)
 struct buf *abp;
 {
         register struct buf *bp;
 
         bp = abp;
         if (bp-&gt;b_flags&amp;B_ERROR)
                 if ((u.u_error = bp-&gt;b_error)==0)
                         u.u_error = EIO;
 }
 /* ---------------------------       */




