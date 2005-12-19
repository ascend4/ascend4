#ifndef ASC_MATHMACROS_H
#define ASC_MATHMACROS_H

/* Macros for MAX, MIN and ABS... */
#ifdef __GNUC__
# define MAX(X,Y) \
	( { typeof (X) x_ = (X); \
    	typeof (Y) y_ = (Y); \
    	(x_ > y_) ? x_ : y_; \
	} )
# define MIN(X,Y) \
	( { typeof (X) x_ = (X); \
    	typeof (Y) y_ = (Y); \
    	(x_ < y_) ? x_ : y_; \
	} )
# define ABS(X) \
	( { typeof (X) x_ = (X); \
    	(x_ > 0) ? x_ : -x_; \
	} )
#else
# define MAX(a,b) ( (a) < (b) ? (b) : (a) )
# define MIN(a,b) ( (a) < (b) ? (a) : (b) )
# define ABS(x) ( ((x) > 0) ? (x) : -(x) )
#endif

#endif
