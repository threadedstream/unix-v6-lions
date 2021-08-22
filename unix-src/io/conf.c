
 int     (*bdevsw[])()
 {
         &amp;nulldev,       &amp;nulldev,       &amp;rkstrategy,    &amp;rktab, /* rk */
         &amp;nodev,         &amp;nodev,         &amp;nodev,         0,      /* rp */
         &amp;nodev,         &amp;nodev,         &amp;nodev,         0,      /* rf */
         &amp;nodev,         &amp;nodev,         &amp;nodev,         0,      /* tm */
         &amp;nodev,         &amp;nodev,         &amp;nodev,         0,      /* tc */
         &amp;nodev,         &amp;nodev,         &amp;nodev,         0,      /* hs */
         &amp;nodev,         &amp;nodev,         &amp;nodev,         0,      /* hp */
         &amp;nodev,         &amp;nodev,         &amp;nodev,         0,      /* ht */
         0
 };
 
 int     (*cdevsw[])()
 {
         &amp;klopen,   &amp;klclose,  &amp;klread,   &amp;klwrite,  &amp;klsgtty,   /* console */
 
         &amp;pcopen,   &amp;pcclose,  &amp;pcread,   &amp;pcwrite,  &amp;nodev,     /* pc */
 
         &amp;lpopen,   &amp;lpclose,  &amp;lpread,   &amp;lpwrite,  &amp;nodev,     /* lp */
 
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* dc */
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* dh */
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* dp */
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* dj */
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* dn */
         &amp;nulldev,  &amp;nulldev,  &amp;mmread,   &amp;mmwrite,  &amp;nodev,     /* mem */
 
         &amp;nulldev,  &amp;nulldev,  &amp;rkread,   &amp;rkwrite,  &amp;nodev,     /* rk */
 
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* rf */
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* rp */
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* tm */
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* hs */
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* hp */
         &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,    &amp;nodev,     /* ht */
         0
 };
 
 int     rootdev {(0&lt;&lt;8)|0};
 int     swapdev {(0&lt;&lt;8)|0};
 int     swplo   4000;   /* cannot be zero */
 int     nswap   872;




