
 /*
  */
 
 /*
  * This table is the switch used to transfer
  * to the appropriate routine for processing a system call.
  * Each row contains the number of arguments expected
  * and a pointer to the routine.
  */
 int     sysent[]
 {
         0, &amp;nullsys,                    /*  0 = indir */
         0, &amp;rexit,                      /*  1 = exit */
         0, &amp;fork,                       /*  2 = fork */
         2, &amp;read,                       /*  3 = read */
         2, &amp;write,                      /*  4 = write */
         2, &amp;open,                       /*  5 = open */
         0, &amp;close,                      /*  6 = close */
         0, &amp;wait,                       /*  7 = wait */
         2, &amp;creat,                      /*  8 = creat */
         2, &amp;link,                       /*  9 = link */
         1, &amp;unlink,                     /* 10 = unlink */
         2, &amp;exec,                       /* 11 = exec */
         1, &amp;chdir,                      /* 12 = chdir */
         0, &amp;gtime,                      /* 13 = time */
         3, &amp;mknod,                      /* 14 = mknod */
         2, &amp;chmod,                      /* 15 = chmod */
         2, &amp;chown,                      /* 16 = chown */
         1, &amp;sbreak,                     /* 17 = break */
         2, &amp;stat,                       /* 18 = stat */
         2, &amp;seek,                       /* 19 = seek */
         0, &amp;getpid,                     /* 20 = getpid */
         3, &amp;smount,                     /* 21 = mount */
         1, &amp;sumount,                    /* 22 = umount */
         0, &amp;setuid,                     /* 23 = setuid */
         0, &amp;getuid,                     /* 24 = getuid */
         0, &amp;stime,                      /* 25 = stime */
         3, &amp;ptrace,                     /* 26 = ptrace */
         0, &amp;nosys,                      /* 27 = x */
         1, &amp;fstat,                      /* 28 = fstat */
         0, &amp;nosys,                      /* 29 = x */
         1, &amp;nullsys,                    /* 30 = smdate; inoperative */
         1, &amp;stty,                       /* 31 = stty */
         1, &amp;gtty,                       /* 32 = gtty */
         0, &amp;nosys,                      /* 33 = x */
         0, &amp;nice,                       /* 34 = nice */
         0, &amp;sslep,                      /* 35 = sleep */
         0, &amp;sync,                       /* 36 = sync */
         1, &amp;kill,                       /* 37 = kill */
         0, &amp;getswit,                    /* 38 = switch */
         0, &amp;nosys,                      /* 39 = x */
         0, &amp;nosys,                      /* 40 = x */
         0, &amp;dup,                        /* 41 = dup */
         0, &amp;pipe,                       /* 42 = pipe */
         1, &amp;times,                      /* 43 = times */
         4, &amp;profil,                     /* 44 = prof */
         0, &amp;nosys,                      /* 45 = tiu */
         0, &amp;setgid,                     /* 46 = setgid */
         0, &amp;getgid,                     /* 47 = getgid */
         2, &amp;ssig,                       /* 48 = sig */
         0, &amp;nosys,                      /* 49 = x */
         0, &amp;nosys,                      /* 50 = x */
         0, &amp;nosys,                      /* 51 = x */
         0, &amp;nosys,                      /* 52 = x */
         0, &amp;nosys,                      /* 53 = x */
         0, &amp;nosys,                      /* 54 = x */
         0, &amp;nosys,                      /* 55 = x */
         0, &amp;nosys,                      /* 56 = x */
         0, &amp;nosys,                      /* 57 = x */
         0, &amp;nosys,                      /* 58 = x */
         0, &amp;nosys,                      /* 59 = x */
         0, &amp;nosys,                      /* 60 = x */
         0, &amp;nosys,                      /* 61 = x */
         0, &amp;nosys,                      /* 62 = x */
         0, &amp;nosys                       /* 63 = x */
 };
 /* ---------------------------       */




