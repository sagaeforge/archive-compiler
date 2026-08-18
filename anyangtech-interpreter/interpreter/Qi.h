#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cctype>

using namespace std;

#define SHORT_SIZ  sizeof(short int)
#define SHORT_P(p) (short int *)(p)
#define UCHAR_P(p) (unsigned char *)(p)
#define LIN_SIZ 255
#define _C_and &&
#define _C_or ||
#define _C_not !
#define __CompileOS__ Ubuntu


enum TknKind {
	False=0, True=1,
	Lparen='(', Rparen=')', Lbracket='[', Rbracket=']', Plus='+', Minus='-',
	Multi='*',  Divi='/',   Mod='%',      Not='!',      Ifsub='?',  Assign='=',
	IntDivi='\\', Comma=',',     DblQ='"',
	Func=150, Var,   If,     Elif,   Else,   For, To, Step,  While,
	End,      Break, Return, Option, Print,  Println, Input, Toint,
	Exit,     Equal, NotEq,  Less,   LessEq, Great,   GreatEq, And, Or,
	END_KeyList, Assign_Char,
	Ident,      IntNum, DblNum, String,   Letter, Doll, Digit,
	Gvar, Lvar, Fcall,  EofProg, EofLine, Others
};

struct Token {
	TknKind kind;
	string  text;
	double  dblVal;
	Token() {  kind=Others; text=""; dblVal=0.0; }
	Token (TknKind k)           { kind=k; text=""; dblVal=0.0; }
	Token (TknKind k, double d) { kind=k; text=""; dblVal=d; }
	Token (TknKind k, const string& s) { kind=k; text=s; dblVal=0.0; }
	Token (TknKind k, const string& s, double d) { kind=k; text=s; dblVal=d; }
};

enum SymKind { noId, varId, fncId, paraId };
enum DtType  { NON_T, DBL_T };

struct SymTbl {
	string  name;
	SymKind nmKind;
	char    dtTyp;
	int     aryLen;
	short   args;
	int     adrs;
	int     frame;
	SymTbl() { clear(); }
	void clear() {
		name=""; nmKind=noId; dtTyp=NON_T;
		aryLen=0; args=0; adrs=0; frame=0;
	}
};

struct CodeSet {
	TknKind kind;
	const char *text;
	double dblVal;
	int    symNbr;
	int    jmpAdrs;
	CodeSet() { clear(); }
	CodeSet(TknKind k)           { clear(); kind=k; }
	CodeSet(TknKind k, double d) { clear(); kind=k; dblVal=d; }
	CodeSet(TknKind k, const char *s) { clear(); kind=k; text=s; }
	CodeSet(TknKind k, int sym, int jmp) {
		clear(); kind=k; symNbr=sym; jmpAdrs=jmp;
	}
	void clear() { kind=Others; text=""; dblVal=0.0; jmpAdrs=0; symNbr=-1; }
};

struct Tobj {
	char type;
	double d;
	string s;
	Tobj()                 { type = '-'; d = 0.0; s = ""; }
	Tobj(double dt)        { type = 'd'; d = dt;  s = ""; }
	Tobj(const string& st) { type = 's'; d = 0.0; s = st; }
	Tobj(const char *st)   { type = 's'; d = 0.0; s = st; }
};

class Mymemory {
private:
	vector<double> mem;
public:
	void auto_resize(int n) {
		if (n >= (int)mem.size()) { n = (n/256 + 1) * 256; mem.resize(n); }
	}
	void set(int adrs, double dt) { mem[adrs] =  dt; }
	void add(int adrs, double dt) { mem[adrs] += dt; }
	double get(int adrs)     { return mem[adrs]; }
	int size()               { return (int)mem.size(); }
	void resize(unsigned int n) { mem.resize(n); }
};

