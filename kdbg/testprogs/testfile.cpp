#include <iostream.h>
#include <qstring.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>

enum E { red, green, blue, yellow };
struct S { int x, y; S* s; };

struct emptyBase { };
struct emptyDerived : S { };
struct emptyNested : emptyBase { };
struct emptyVBase { virtual ~emptyVBase(){} };
struct emptyVDerived : S { virtual ~emptyVDerived(){} };
struct emptyVNested : emptyVBase { };

int globalvar = 1234;

class Cl
{
	int k;
	double l;
public:
	Cl(int r);
	virtual ~Cl();
	virtual int f(int x);
};

typedef void (*PtrFunc)(E*, char);

class Dl : public Cl
{
public:
	Dl(int r);
	virtual int f(int x);
	int operator()(const QString& x, int& y) const;
	operator const char*() { return 0; }
	operator PtrFunc*();
};

void g()
{
	S s1, s2;
	s1.x = 85;
	s2.y = 17;
	s1.s = &s2;
	s2.s = &s1;
}

void f(E e[3], char c)
{
	E x = e[2];
	S y[2];
	E* pe = e;
        E* pea[3] = { pe, };
	{
		int x = 17;
		x;
	}
	g();
	char buffer[300];
	memset(buffer, 1, 300);
	for (int i = 0; i < sizeof(buffer); i +=15) buffer[i] = '\02';
	QDir dir;
	QFile file;
	QFileInfo fi;
	x = red;
	emptyBase eb;
	emptyDerived ed;
	emptyNested en;
	int ea[0];
	emptyVBase evb;
	emptyVDerived evd;
	emptyVNested evn;
}

void strtest(const char* t)
{
    const char* s = t;
    cout << s << endl;
}


void segFault()
{
	*(char*)0 = 's';
}

int main(int argc, char* argv[])
{
	if (argc > 1) {
		if (*argv[1] == 's') {
			segFault();
		} else if (*argv[1] == 'a') {
			// let debugger attach
			int junk;
			cin >> junk;
		}
	}

	char a[6] = { 'a', 'B', '\'', '\"' };
	char a1[1] = { '1' };
	E e[3] = { red, green, blue };
	E e1[1] = { yellow };

	a[0] = '5';
       	void (*pf[2])(E*, char) = {f};
	{
		double d[1] = { -1.234e123 };
		int i = 10;
		sin(i);
	}
	(*pf[0])(e, '\n');

	QString s;

	s = "Hi, there!\r\n\t\"\'\\";

	const QString& strref = s;

	strtest(s);
	s = "asbcxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	strtest(s);
	s += "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiii";
	strtest(s);
	s += "rst";
	strtest(s);
	s = "xxxxxxxxxxxxxxxxxxxxxxxxxxx";
	strtest(s);
	s += "rst";
	strtest(s);
	s = "";

	Cl c1(13);
	Dl d1(3214);
	d1.f(17);
	int n = 83;
	d1(strref, n);
	PtrFunc* ppf = d1;
}

Cl::Cl(int r) :
	k(r),
	l(sin(r))
{
	cout << l << endl;
}

Cl::~Cl()
{
}

int Cl::f(int x)
{
	int y = 2*x;
	return y;
}

Dl::Dl(int r) :
	Cl(r)
{
}

int Dl::f(int x)
{
	int y = Cl::f(x);
	return y+3;
}

int Dl::operator()(const QString& x, int& y) const
{
	cerr << "ha! I know!" << endl;
	return 1;
}

Dl::operator PtrFunc*()
{
    return 0;
}