#ifndef ASC_MATHMACROS_H
#define ASC_MATHMACROS_H

/* Macros for MAX, MIN and ABS... 
There is nothing portable or standard about this behavior and syntax.
Thus, if a program depends on it for correctness, that program is
unportable and in need of repair. We should not in any case
be using min/max/abs on any functions with side-effects in
ascend code. This, though a clever bit of gnu-ism, just serves
to generate warnings obscuring real problems when testing, so
it's going away until ISO C supports braced expressions which
it doesn't now.
*/
#ifdef NONSTANDARD_MAX
#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define MAX(X,Y) \
	( { __typeof__ (X) xmax_ = (X); \
    	__typeof__ (Y) ymax_ = (Y); \
    	(xmax_ > ymax_) ? xmax_ : ymax_; \
	} )
# define MIN(X,Y) \
	( { __typeof__ (X) xmin_ = (X); \
    	__typeof__ (Y) ymin_ = (Y); \
    	(xmin_ < ymin_) ? xmin_ : ymin_; \
	} )
# define ABS(X) \
	( { __typeof__ (X) xabs_ = (X); \
    	(xabs_ > 0) ? xabs_ : -xabs_; \
	} )
#else
# define MAX(a,b) ( (a) < (b) ? (b) : (a) )
# define MIN(a,b) ( (a) < (b) ? (a) : (b) )
# define ABS(x) ( ((x) > 0) ? (x) : -(x) )
#endif
#else
/* in the mean time we have a nice reliable standby that worked for 30 years
when used as specified */
# define MAX(a,b) ( (a) < (b) ? (b) : (a) )
# define MIN(a,b) ( (a) < (b) ? (a) : (b) )
# define ABS(x) ( ((x) > 0) ? (x) : -(x) )
#endif

#endif /* ASC_MATHMACROS_H */
