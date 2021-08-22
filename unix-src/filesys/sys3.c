 #include "../param.h"
 #include "../systm.h"
 #include "../reg.h"
 #include "../buf.h"
 #include "../filsys.h"
 #include "../user.h"
 #include "../inode.h"
 #include "../file.h"
 #include "../conf.h"
 
 /*
  * the fstat system call.
  */
 fstat()
 {
         register *fp;
 
         fp = getf(u.u_ar0[R0]);
         if(fp == NULL)
                 return;
         stat1(fp-&gt;f_inode, u.u_arg[0]);
 }
 /* ---------------------------       */
 
 /*
  * the stat system call.
  */
 stat()
 {
         register ip;
         extern uchar;
 
         ip = namei(&amp;uchar, 0);
         if(ip == NULL)
                 return;
         stat1(ip, u.u_arg[1]);
         iput(ip);
 }
 /* ---------------------------       */
 
 /*
  * The basic routine for fstat and stat:
  * get the inode and pass appropriate parts back.
  */
 stat1(ip, ub)
 int *ip;
 {
         register i, *bp, *cp;
 
         iupdat(ip, time);
         bp = bread(ip-&gt;i_dev, ldiv(ip-&gt;i_number+31, 16));
         cp = bp-&gt;b_addr + 32*lrem(ip-&gt;i_number+31, 16) + 24;
         ip = &amp;(ip-&gt;i_dev);
         for(i=0; i&lt;14; i++) {
                 suword(ub, *ip++);
                 ub =+ 2;
         }
         for(i=0; i&lt;4; i++) {
                 suword(ub, *cp++);
                 ub =+ 2;
         }
         brelse(bp);
 }
 /* ---------------------------       */
 
 /*
  * the dup system call.
  */
 dup()
 {
         register i, *fp;
 
         fp = getf(u.u_ar0[R0]);
         if(fp == NULL)
                 return;
         if ((i = ufalloc()) &lt; 0)
                 return;
         u.u_ofile[i] = fp;
         fp-&gt;f_count++;
 }
 /* ---------------------------       */
 
 /*
  * the mount system call.
  */
 smount()
 {
         int d;
         register *ip;
         register struct mount *mp, *smp;
         extern uchar;
 
         d = getmdev();
         if(u.u_error)
                 return;
         u.u_dirp = u.u_arg[1];
         ip = namei(&amp;uchar, 0);
         if(ip == NULL)
                 return;
         if(ip-&gt;i_count!=1 || (ip-&gt;i_mode&amp;(IFBLK&amp;IFCHR))!=0)
                 goto out;
         smp = NULL;
         for(mp = &amp;mount[0]; mp &lt; &amp;mount[NMOUNT]; mp++) {
                 if(mp-&gt;m_bufp != NULL) {
                         if(d == mp-&gt;m_dev)
                                 goto out;
                 } else
                 if(smp == NULL)
                         smp = mp;
         }
         if(smp == NULL)
                 goto out;
         (*bdevsw[d.d_major].d_open)(d, !u.u_arg[2]);
         if(u.u_error)
                 goto out;
         mp = bread(d, 1);
         if(u.u_error) {
                 brelse(mp);
                 goto out1;
         }
         smp-&gt;m_inodp = ip;
         smp-&gt;m_dev = d;
         smp-&gt;m_bufp = getblk(NODEV);
         bcopy(mp-&gt;b_addr, smp-&gt;m_bufp-&gt;b_addr, 256);
         smp = smp-&gt;m_bufp-&gt;b_addr;
         smp-&gt;s_ilock = 0;
         smp-&gt;s_flock = 0;
         smp-&gt;s_ronly = u.u_arg[2] &amp; 1;
         brelse(mp);
         ip-&gt;i_flag =| IMOUNT;
         prele(ip);
         return;
 
 out:
         u.u_error = EBUSY;
 out1:
         iput(ip);
 }
 /* ---------------------------       */
 
 /*
  * the umount system call.
  */
 sumount()
 {
         int d;
         register struct inode *ip;
         register struct mount *mp;
 
         update();
         d = getmdev();
         if(u.u_error)
                 return;
         for(mp = &amp;mount[0]; mp &lt; &amp;mount[NMOUNT]; mp++)
                 if(mp-&gt;m_bufp!=NULL &amp;&amp; d==mp-&gt;m_dev)
                         goto found;
         u.u_error = EINVAL;
         return;
 
 found:
         for(ip = &amp;inode[0]; ip &lt; &amp;inode[NINODE]; ip++)
                 if(ip-&gt;i_number!=0 &amp;&amp; d==ip-&gt;i_dev) {
                         u.u_error = EBUSY;
                         return;
                 }
         (*bdevsw[d.d_major].d_close)(d, 0);
         ip = mp-&gt;m_inodp;
         ip-&gt;i_flag =&amp; ~IMOUNT;
         iput(ip);
         ip = mp-&gt;m_bufp;
         mp-&gt;m_bufp = NULL;
         brelse(ip);
 }
 /* ---------------------------       */
 
 /*
  * Common code for mount and umount.
  * Check that the user's argument is a reasonable
  * thing on which to mount, and return the device number if so.
  */
 getmdev()
 {
         register d, *ip;
         extern uchar;
 
         ip = namei(&amp;uchar, 0);
         if(ip == NULL)
                 return;
         if((ip-&gt;i_mode&amp;IFMT) != IFBLK)
                 u.u_error = ENOTBLK;
         d = ip-&gt;i_addr[0];
         if(ip-&gt;i_addr[0].d_major &gt;= nblkdev)
                 u.u_error = ENXIO;
         iput(ip);
         return(d);
 }
 /* ---------------------------       */




