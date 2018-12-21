#ifndef tfunction_h
#define tfunction_h

#include <stdio.h>
#include <string.h>

typedef enum {
 FIRST = 1,
	SECOND = 2
} Order;

class Point
{          
	char m_s[64];
 float m_x;
	float m_y;

public:

	enum Error {
		SUCCESS = 0,
		ERROR = 1
	};

	Point (float x=0, float y=0)
	: m_x(x), m_y(y)
	{
	}
	virtual ~Point ()
	{
	}

	void set (float x, float y)
	{
		m_x = x, m_y = y;
	}
	void set (float v[2]=0)
	{
		m_x = v[0], m_y=v[1];
	}
	void setpointer (Point* p)
	{
		*this = *p;
	}
	void setref (Point& p)
	{
		*this = p;
	}
	void setvalue (Point p)
	{
		*this = p;
	}
	void setconst (const Point* p)
	{
		*this = *p;
	}
	void setname (const char* s)
	{
		strncpy(m_s,s,63);
	}

	void get (float* x, float* y) const
	{
		*x = m_x, *y = m_y;
	}
	void get (float v[2]) const
	{
		v[0] = m_x, v[1] = m_y;
	}
	Point* getpointer ()
	{
		return this;
	}
	Point& getref ()
	{
		return *this;
	}
	Point getvalue ()
	{
		return *this;
	}
	const Point* getconst () const
	{
		return this;
	}
	const char* getname () const
	{
		return m_s;
	}

	Point operator+ (const Point& p) const
	{
		return Point(m_x+p.m_x,m_y+p.m_y);
	}
	Point operator- (const Point& p) const
	{
		return Point(m_x-p.m_x,m_y-p.m_y);
	}
	Point operator* (const Point& p) const
	{
		return Point(m_x*p.m_x,m_y*p.m_y);
	}
	Point operator/ (float n) const
	{
		return Point(m_x/n,m_y/n);
	}
	bool operator< (const Point& p) const
	{
		if (m_x < p.m_x) return true;
		else if (m_x > p.m_x) return false;
		else return m_y < p.m_y;
	}
	bool operator<= (const Point& p) const
	{
		return operator<(p) || operator==(p);
	}
	bool operator== (const Point& p) const
	{
		return m_x==p.m_x && m_y==p.m_y;
	}

 float operator[] (int i) const
	{
		return (i==0) ? m_x : m_y;
	}
	float& operator[] (int i)
	{
		return (i==0) ? m_x : m_y;
	}

	static Error echo (Error e)
	{
		return e;
	}

};


inline Point add (const Point& p1, const Point& p2)
{
	return p1+p2;
}
inline Point sub (const Point& p1, const Point& p2)
{
	return p1-p2;
}
inline Point mult (const Point& p1, const Point& p2)
{
	return p1*p2;
}
inline Point div (const Point& p1, float n)
{
	return p1/n;
}

inline void getpoint (const Point* p, float* x, float* y)
{
	p->get(x,y);
}
inline void setpoint (Point* p, float x=0, float y=0)
{
	p->set(x,y);
}

inline	Point average (int n, Point v[])
{
	Point p(0,0);
	for (int i=0; i<n; ++i)
		p = p+v[i];
	return p/n;
}

inline	Point averagepointer (int n, Point* v[])
{
	Point p(0,0);
	for (int i=0; i<n; ++i)
		p = p+(*v[i]);
	return p/n;
}

inline	void copyvector (int n, const Point v[], Point u[])
{
	for (int i=0; i<n; ++i)
		u[i] = v[i];
}

inline Order invert (Order o)
{
	if (o==FIRST)
		return SECOND;
	else
		return FIRST;
}


class ColorPoint : public Point   //tolua_export
{                                 //tolua_export
 float r, g, b;
public:

//tolua_begin
 ColorPoint (float px, float py, float cr=0.0f, float cg=0.0f, float cb=0.0f)
 : Point(px,py), r(cr), g(cg), b(cb)
 {
 }
 virtual ~ColorPoint () { }

 virtual void getcolor (float* red, float *green, float *blue) const
	{
		*red = r;
		*green = g;
		*blue = b;
	}
 static ColorPoint* MakeRed (float x, float y)
 {
  return new ColorPoint(x,y,1,0,0);
 }
};
//tolua_end
#endif
