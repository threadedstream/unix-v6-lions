 #include "../param.h"
 #include "../systm.h"
 #include "../user.h"
 #include "../inode.h"
 #include "../file.h"
 #include "../reg.h"
 
 /* 
  * Max allowable buffering per pipe.
  * This is also the max size of the
  * file created to implement the pipe.
  * If this size is bigger than 4096,
  * pipes will be implemented in LARG
  * files, which is probably not good.
  */
 #define PIPSIZ  4096
 
 /*
  * The sys-pipe entry.
  * Allocate an inode on the root device.
  * Allocate 2 file structures.
  * Put it all together with flags.
  */
 pipe()
 {
         register *ip, *rf, *wf;
         int r;
 
         ip = ialloc(rootdev);
         if(ip == NULL)
                 return;
         rf = falloc();
         if(rf == NULL) {
                 iput(ip);
                 return;
         }
         r = u.u_ar0[R0];
         wf = falloc();
         if(wf == NULL) {
                 rf-&gt;f_count = 0;
                 u.u_ofile[r] = NULL;
                 iput(ip);
                 return;
         }
         u.u_ar0[R1] = u.u_ar0[R0];
         u.u_ar0[R0] = r;
         wf-&gt;f_flag = FWRITE|FPIPE;
         wf-&gt;f_inode = ip;
         rf-&gt;f_flag = FREAD|FPIPE;
         rf-&gt;f_inode = ip;
         ip-&gt;i_count = 2;
         ip-&gt;i_flag = IACC|IUPD;
         ip-&gt;i_mode = IALLOC;
 }
 /* ---------------------------       */
 
 /* Read call directed to a pipe.
  */
 readp(fp)
 int *fp;
 {
         register *rp, *ip;
 
         rp = fp;
         ip = rp-&gt;f_inode;
 loop:
         /* Very conservative locking.
          */
         plock(ip);
         /* If the head (read) has caught up with
          * the tail (write), reset both to 0.
          */
         if(rp-&gt;f_offset[1] == ip-&gt;i_size1) {
                 if(rp-&gt;f_offset[1] != 0) {
                         rp-&gt;f_offset[1] = 0;
                         ip-&gt;i_size1 = 0;
                         if(ip-&gt;i_mode&amp;IWRITE) {
                                 ip-&gt;i_mode =&amp; ~IWRITE;
                                 wakeup(ip+1);
                         }
                 }
 
                 /* If there are not both reader and
                  * writer active, return without
                  * satisfying read.
                  */
                 prele(ip);
                 if(ip-&gt;i_count &lt; 2)
                         return;
                 ip-&gt;i_mode =| IREAD;
                 sleep(ip+2, PPIPE);
                 goto loop;
         }
         /* Read and return
          */
         u.u_offset[0] = 0;
         u.u_offset[1] = rp-&gt;f_offset[1];
         readi(ip);
         rp-&gt;f_offset[1] = u.u_offset[1];
         prele(ip);
 }
 /* ---------------------------       */
 
 /* Write call directed to a pipe.
  */
 writep(fp)
 {
         register *rp, *ip, c;
 
         rp = fp;
         ip = rp-&gt;f_inode;
         c = u.u_count;
 loop:
         /* If all done, return.
          */
         plock(ip);
         if(c == 0) {
                 prele(ip);
                 u.u_count = 0;
                 return;
         }
         /* If there are not both read and
          * write sides of the pipe active,
          * return error and signal too.
          */
         if(ip-&gt;i_count &lt; 2) {
                 prele(ip);
                 u.u_error = EPIPE;
                 psignal(u.u_procp, SIGPIPE);
                 return;
         }
         /* If the pipe is full,
          * wait for reads to deplete
          * and truncate it.
          */
         if(ip-&gt;i_size1 == PIPSIZ) {
                 ip-&gt;i_mode =| IWRITE;
                 prele(ip);
                 sleep(ip+1, PPIPE);
                 goto loop;
         }
         /* Write what is possible and
          * loop back.
          */
         u.u_offset[0] = 0;
         u.u_offset[1] = ip-&gt;i_size1;
         u.u_count = min(c, PIPSIZ-u.u_offset[1]);
         c =- u.u_count;
         writei(ip);
         prele(ip);
         if(ip-&gt;i_mode&amp;IREAD) {
                 ip-&gt;i_mode =&amp; ~IREAD;
                 wakeup(ip+2);
         }
         goto loop;
 }
 /* ---------------------------       */
 
 /* Lock a pipe.
  * If its already locked,
  * set the WANT bit and sleep.
  */
 plock(ip)
 int *ip;
 {
         register *rp;
 
         rp = ip;
         while(rp-&gt;i_flag&amp;ILOCK) {
                 rp-&gt;i_flag =| IWANT;
                 sleep(rp, PPIPE);
         }
         rp-&gt;i_flag =| ILOCK;
 }
 /* ---------------------------       */
 
 /* Unlock a pipe.
  * If WANT bit is on,
  * wakeup.
  * This routine is also used
  * to unlock inodes in general.
  */
 prele(ip)
 int *ip;
 {
         register *rp;
 
         rp = ip;
         rp-&gt;i_flag =&amp; ~ILOCK;
         if(rp-&gt;i_flag&amp;IWANT) {
                 rp-&gt;i_flag =&amp; ~IWANT;
                 wakeup(rp);
         }
 }
 /* ---------------------------       */




