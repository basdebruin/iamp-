# Instant Amplitude PD External
*made as a DSP3 assignment*
> tested on Pd-0.51-1

See `iamp~.c` for external code

Based on concepts and coefficients from the following articles: 
* [http://yehar.com/blog/?p=368](http://yehar.com/blog/?p=368)
* [http://www.katjaas.nl/hilbert/hilbert.html](http://www.katjaas.nl/hilbert/hilbert.html)


## Diagram
``` c
   +->allpass->allpass->allpass->allpass->delay->abs-+ 
in-|                                                 |->*0.5->out
   +->allpass->allpass->allpass->allpass-------->abs-+ 
```