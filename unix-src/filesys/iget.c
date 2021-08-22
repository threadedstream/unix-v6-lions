 #include "../param.h"
 #include "../systm.h"
 #include "../user.h"
 #include "../inode.h"
 #include "../filsys.h"
 #include "../conf.h"
 #include "../buf.h"
 
 /*
  * Look up an inode by device,inumber.
  * If it is in core (in the inode structure),
  * honor the locking protocol.
  * If it is not in core, read it in from the
  * specified device.
  * If the inode is mounted on, perform
  * the indicated indirection.
  * In all cases, a pointer to a locked
  * inode structure is returned.
  *
  * printf warning: no inodes -- if the inode
  *      structure is full
  * panic: no imt -- if the mounted file
  *      system is not in the mount table.
  *      "cannot happen"
  */
 iget(dev, ino)
 {
         register struct inode *p;
         register *ip2;
         int *ip1;
         register struct mount *ip;
 
 loop:
         ip = NULL;
         for(p = &amp;inode[0]; p &lt; &amp;inode[NINODE]; p++) {
                 if(dev==p-&gt;i_dev &amp;&amp; ino==p-&gt;i_number) {
                         if((p-&gt;i_flag&amp;ILOCK) != 0) {
                                 p-&gt;i_flag =| IWANT;
                                 sleep(p, PINOD);
                                 goto loop;
                         }
                         if((p-&gt;i_flag&amp;IMOUNT) != 0) {
 
                                 for(ip = &amp;mount[0]; ip &lt; &amp;mount[NMOUNT]; ip++)
                                 if(ip-&gt;m_inodp == p) {
                                         dev = ip-&gt;m_dev;
                                         ino = ROOTINO;
                                         goto loop;
                                 }
                                 panic("no imt");
                         }
                         p-&gt;i_count++;
                         p-&gt;i_flag =| ILOCK;
                         return(p);
                 }
                 if(ip==NULL &amp;&amp; p-&gt;i_count==0)
                         ip = p;
         }
         if((p=ip) == NULL) {
                 printf("Inode table overflow\n");
                 u.u_error = ENFILE;
                 return(NULL);
         }
         p-&gt;i_dev = dev;
         p-&gt;i_number = ino;
         p-&gt;i_flag = ILOCK;
         p-&gt;i_count++;
         p-&gt;i_lastr = -1;
         ip = bread(dev, ldiv(ino+31,16));
         /*
          * Check I/O errors
          */
         if (ip-&gt;b_flags&amp;B_ERROR) {
                 brelse(ip);
                 iput(p);
                 return(NULL);
         }
         ip1 = ip-&gt;b_addr + 32*lrem(ino+31, 16);
         ip2 = &amp;p-&gt;i_mode;
         while(ip2 &lt; &amp;p-&gt;i_addr[8])
                 *ip2++ = *ip1++;
         brelse(ip);
         return(p);
 }
 /* ---------------------------       */
 
 /*
  * Decrement reference count of
  * an inode structure.
  * On the last reference,
  * write the inode out and if necessary,
  * truncate and deallocate the file.
  */
 iput(p)
 struct inode *p;
 {
         register *rp;
 
         rp = p;
         if(rp-&gt;i_count == 1) {
                 rp-&gt;i_flag =| ILOCK;
                 if(rp-&gt;i_nlink &lt;= 0) {
                         itrunc(rp);
                         rp-&gt;i_mode = 0;
                         ifree(rp-&gt;i_dev, rp-&gt;i_number);
                 }
                 iupdat(rp, time);
                 prele(rp);
                 rp-&gt;i_flag = 0;
                 rp-&gt;i_number = 0;
         }
         rp-&gt;i_count--;
         prele(rp);
 }
 /* ---------------------------       */
 
 /*
  * Check accessed and update flags on
  * an inode structure.
  * If either is on, update the inode
  * with the corresponding dates
  * set to the argument tm.
  */
 iupdat(p, tm)
 int *p;
 int *tm;
 {
         register *ip1, *ip2, *rp;
         int *bp, i;
 
         rp = p;
         if((rp-&gt;i_flag&amp;(IUPD|IACC)) != 0) {
                 if(getfs(rp-&gt;i_dev)-&gt;s_ronly)
                         return;
                 i = rp-&gt;i_number+31;
                 bp = bread(rp-&gt;i_dev, ldiv(i,16));
                 ip1 = bp-&gt;b_addr + 32*lrem(i, 16);
                 ip2 = &amp;rp-&gt;i_mode;
                 while(ip2 &lt; &amp;rp-&gt;i_addr[8])
                         *ip1++ = *ip2++;
                 if(rp-&gt;i_flag&amp;IACC) {
                         *ip1++ = time[0];
                         *ip1++ = time[1];
                 } else
                         ip1 =+ 2;
                 if(rp-&gt;i_flag&amp;IUPD) {
                         *ip1++ = *tm++;
                         *ip1++ = *tm;
                 }
                 bwrite(bp);
         }
 }
 /* ---------------------------       */
 
 /*
  * Free all the disk blocks associated
  * with the specified inode structure.
  * The blocks of the file are removed
  * in reverse order. This FILO
  * algorithm will tend to maintain
  * a contiguous free list much longer
  * than FIFO.
  */
 itrunc(ip)
 int *ip;
 {
         register *rp, *bp, *cp;
         int *dp, *ep;
 
         rp = ip;
         if((rp-&gt;i_mode&amp;(IFCHR&amp;IFBLK)) != 0)
                 return;
         for(ip = &amp;rp-&gt;i_addr[7]; ip &gt;= &amp;rp-&gt;i_addr[0]; ip--)
         if(*ip) {
                 if((rp-&gt;i_mode&amp;ILARG) != 0) {
                         bp = bread(rp-&gt;i_dev, *ip);
 
                         for(cp = bp-&gt;b_addr+512; cp &gt;= bp-&gt;b_addr; cp--)
                         if(*cp) {
                                 if(ip == &amp;rp-&gt;i_addr[7]) {
                                         dp = bread(rp-&gt;i_dev, *cp);
 
                                         for(ep = dp-&gt;b_addr+512; ep &gt;= dp-&gt;b_addr; ep--)
                                         if(*ep)
                                                 free(rp-&gt;i_dev, *ep);
                                         brelse(dp);
                                 }
                                 free(rp-&gt;i_dev, *cp);
                         }
                         brelse(bp);
                 }
                 free(rp-&gt;i_dev, *ip);
                 *ip = 0;
         }
         rp-&gt;i_mode =&amp; ~ILARG;
         rp-&gt;i_size0 = 0;
         rp-&gt;i_size1 = 0;
         rp-&gt;i_flag =| IUPD;
 }
 /* ---------------------------       */
 
 /*
  * Make a new file.
  */
 maknode(mode)
 {
         register *ip;
 
         ip = ialloc(u.u_pdir-&gt;i_dev);
         if (ip==NULL)
                 return(NULL);
         ip-&gt;i_flag =| IACC|IUPD;
         ip-&gt;i_mode = mode|IALLOC;
         ip-&gt;i_nlink = 1;
         ip-&gt;i_uid = u.u_uid;
         ip-&gt;i_gid = u.u_gid;
         wdir(ip);
         return(ip);
 }
 /* ---------------------------       */
 
 /*
  * Write a directory entry with
  * parameters left as side effects
  * to a call to namei.
  */
 wdir(ip)
 int *ip;
 {
         register char *cp1, *cp2;
 
         u.u_dent.u_ino = ip-&gt;i_number;
         cp1 = &amp;u.u_dent.u_name[0];
         for(cp2 = &amp;u.u_dbuf[0]; cp2 &lt; &amp;u.u_dbuf[DIRSIZ];)
                 *cp1++ = *cp2++;
         u.u_count = DIRSIZ+2;
         u.u_segflg = 1;
         u.u_base = &amp;u.u_dent;
         writei(u.u_pdir);
         iput(u.u_pdir);
 }
 /* ---------------------------       */




