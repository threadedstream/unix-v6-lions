# unix-v6-lions

6th edition of Unix operating system with John Lions' commentary. Here's a <a href="https://pages.lip6.fr/Pierre.Sens/srcv6/">link</a> to the dedicated web page

## Notes

Files were programmatically downloaded as .c.html/.h.html/.s.html and further converted to .c/.h/.html, respectively. In order to repeat the same process,
you need to build and run tool.cpp. I should notice that it requires jemalloc shared library for normal functioning, although you are free to omit -ljemalloc option 
in CMakeLists.txt. 

## Inspiration
The inspiration for creation of and further contribution to this repository was drawn from Brian Kernighan's excellent book "UNIX: A History and a Memoir".  
