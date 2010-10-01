
typedef struct Point Point;
struct Point
{
	float x;
	float y;
};


extern int a[10];
extern Point p[10]; 
extern Point* pp[10];


extern int ma[10];
extern Point mp[10]; 
extern Point* mpp[10];

typedef struct Array Array;
struct Array
{
 int a[10];
 Point p[10]; 
 Point* pp[10];
};

extern Array array;
extern Array* parray;
