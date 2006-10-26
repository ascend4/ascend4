#ifndef ASC_MATHMACROS_H
#define ASC_MATHMACROS_H

/* Macros for MAX, MIN and ABS... */
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

#endif /* ASC_MATHMACROS_H */
