typedef struct A A;
typedef struct B B;
typedef union U U;

struct A
{
	int i;
	float f;
	double d;
	char* s;
	void* v;
	char n[64];
};

union U
{
	int i;
	float f;
};

struct B
{
	A a;
 B*	b;
};

extern int i;
extern float f;
extern double d;
extern char* s;
extern void* v;
extern char n[64];
extern A a;
extern B* b;
extern U u;

extern int mi;
extern float mf;
extern double md;
extern char* ms;
extern void* mv;
extern char mn[64];
extern A ma;
extern B* mb;
