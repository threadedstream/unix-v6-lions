 #include "../param.h"
 #include "../systm.h"
 #include "../user.h"
 #include "../reg.h"
 #include "../file.h"
 #include "../inode.h"
 
 /*
  * read system call
  */
 read()
 {
         rdwr(FREAD);
 }
 /* ---------------------------       */
 
 /*
  * write system call
  */
 write()
 {
         rdwr(FWRITE);
 }
 /* ---------------------------       */
 
 /*
  * common code for read and write calls:
  * check permissions, set base, count, and offset,
  * and switch out to readi, writei, or pipe code.
  */
 rdwr(mode)
 {
         register *fp, m;
 
         m = mode;
         fp = getf(u.u_ar0[R0]);
         if(fp == NULL)
                 return;
         if((fp-&gt;f_flag&amp;m) == 0) {
                 u.u_error = EBADF;
                 return;
         }
         u.u_base = u.u_arg[0];
         u.u_count = u.u_arg[1];
         u.u_segflg = 0;
         if(fp-&gt;f_flag&amp;FPIPE) {
                 if(m==FREAD)
                         readp(fp); else
                         writep(fp);
         } else {
                 u.u_offset[1] = fp-&gt;f_offset[1];
                 u.u_offset[0] = fp-&gt;f_offset[0];
                 if(m==FREAD)
                         readi(fp-&gt;f_inode); else
                         writei(fp-&gt;f_inode);
                 dpadd(fp-&gt;f_offset, u.u_arg[1]-u.u_count);
         }
         u.u_ar0[R0] = u.u_arg[1]-u.u_count;
 }
 /* ---------------------------       */
 
 /*
  * open system call
  */
 open()
 {
         register *ip;
         extern uchar;
 
         ip = namei(&amp;uchar, 0);
         if(ip == NULL)
                 return;
         u.u_arg[1]++;
         open1(ip, u.u_arg[1], 0);
 }
 /* ---------------------------       */
 
 /*
  * creat system call
  */
 creat()
 {
         register *ip;
         extern uchar;
 
         ip = namei(&amp;uchar, 1);
         if(ip == NULL) {
                 if(u.u_error)
                         return;
                 ip = maknode(u.u_arg[1]&amp;07777&amp;(~ISVTX));
                 if (ip==NULL)
                         return;
                 open1(ip, FWRITE, 2);
         } else
                 open1(ip, FWRITE, 1);
 }
 /* ---------------------------       */
 
 /*
  * common code for open and creat.
  * Check permissions, allocate an open file structure,
  * and call the device open routine if any.
  */
 open1(ip, mode, trf)
 int *ip;
 {
         register struct file *fp;
         register *rip, m;
         int i;
 
         rip = ip;
         m = mode;
         if(trf != 2) {
                 if(m&amp;FREAD)
                         access(rip, IREAD);
                 if(m&amp;FWRITE) {
                         access(rip, IWRITE);
                         if((rip-&gt;i_mode&amp;IFMT) == IFDIR)
                                 u.u_error = EISDIR;
                 }
         }
         if(u.u_error)
                 goto out;
         if(trf)
                 itrunc(rip);
         prele(rip);
         if ((fp = falloc()) == NULL)
                 goto out;
         fp-&gt;f_flag = m&amp;(FREAD|FWRITE);
         fp-&gt;f_inode = rip;
         i = u.u_ar0[R0];
         openi(rip, m&amp;FWRITE);
         if(u.u_error == 0)
                 return;
         u.u_ofile[i] = NULL;
         fp-&gt;f_count--;
 
 out:
         iput(rip);
 }
 /* ---------------------------       */
 
 /*
  * close system call
  */
 close()
 {
         register *fp;
 
         fp = getf(u.u_ar0[R0]);
         if(fp == NULL)
                 return;
         u.u_ofile[u.u_ar0[R0]] = NULL;
         closef(fp);
 }
 /* ---------------------------       */
 
 /*
  * seek system call
  */
 seek()
 {
         int n[2];
         register *fp, t;
 
         fp = getf(u.u_ar0[R0]);
         if(fp == NULL)
                 return;
         if(fp-&gt;f_flag&amp;FPIPE) {
                 u.u_error = ESPIPE;
                 return;
         }
         t = u.u_arg[1];
         if(t &gt; 2) {
                 n[1] = u.u_arg[0]&lt;&lt;9;
                 n[0] = u.u_arg[0]&gt;&gt;7;
                 if(t == 3)
                         n[0] =&amp; 0777;
         } else {
                 n[1] = u.u_arg[0];
                 n[0] = 0;
                 if(t!=0 &amp;&amp; n[1]&lt;0)
                         n[0] = -1;
         }
         switch(t) {
 
         case 1:
         case 4:
                 n[0] =+ fp-&gt;f_offset[0];
                 dpadd(n, fp-&gt;f_offset[1]);
                 break;
 
         default:
                 n[0] =+ fp-&gt;f_inode-&gt;i_size0&amp;0377;
                 dpadd(n, fp-&gt;f_inode-&gt;i_size1);
 
         case 0:
         case 3:
                 ;
         }
         fp-&gt;f_offset[1] = n[1];
         fp-&gt;f_offset[0] = n[0];
 }
 /* ---------------------------       */
 
 /*
  * link system call
  */
 link()
 {
         register *ip, *xp;
         extern uchar;
 
         ip = namei(&amp;uchar, 0);
         if(ip == NULL)
                 return;
         if(ip-&gt;i_nlink &gt;= 127) {
                 u.u_error = EMLINK;
                 goto out;
         }
         if((ip-&gt;i_mode&amp;IFMT)==IFDIR &amp;&amp; !suser())
                 goto out;
         /*
          * unlock to avoid possibly hanging the namei
          */
         ip-&gt;i_flag =&amp; ~ILOCK;
         u.u_dirp = u.u_arg[1];
         xp = namei(&amp;uchar, 1);
         if(xp != NULL) {
                 u.u_error = EEXIST;
                 iput(xp);
         }
         if(u.u_error)
                 goto out;
         if(u.u_pdir-&gt;i_dev != ip-&gt;i_dev) {
                 iput(u.u_pdir);
                 u.u_error = EXDEV;
                 goto out;
         }
         wdir(ip);
         ip-&gt;i_nlink++;
         ip-&gt;i_flag =| IUPD;
 
 out:
         iput(ip);
 }
 /* ---------------------------       */
 
 /*
  * mknod system call
  */
 mknod()
 {
         register *ip;
         extern uchar;
 
         if(suser()) {
                 ip = namei(&amp;uchar, 1);
                 if(ip != NULL) {
                         u.u_error = EEXIST;
                         goto out;
                 }
         }
         if(u.u_error)
                 return;
         ip = maknode(u.u_arg[1]);
         if (ip==NULL)
                 return;
         ip-&gt;i_addr[0] = u.u_arg[2];
 
 out:
         iput(ip);
 }
 /* ---------------------------       */
 
 /* sleep system call
  * not to be confused with the sleep internal routine.
  */
 sslep()
 {
         char *d[2];
 
         spl7();
         d[0] = time[0];
         d[1] = time[1];
         dpadd(d, u.u_ar0[R0]);
 
         while(dpcmp(d[0], d[1], time[0], time[1]) &gt; 0) {
                 if(dpcmp(tout[0], tout[1], time[0], time[1]) &lt;= 0 ||
                    dpcmp(tout[0], tout[1], d[0], d[1]) &gt; 0) {
                         tout[0] = d[0];
                         tout[1] = d[1];
                 }
                 sleep(tout, PSLEP);
         }
         spl0();
 }
 /* ---------------------------       */




