#include "Qi.h"
#include "Qi_prot.h"

struct KeyWord {
	const char *keyName;
	TknKind keyKind;
};

KeyWord KeyWdTbl[] = {
	{"def"   , Func   }, {"var"    , Var    },
	{"if"     , If    }, {"elif"   , Elif   },
	{"else"   , Else  }, {"for"    , For    },
	{"to"     , To    }, {"step"   , Step   },
	{"while"  , While }, {"end"    , End    },
	{"break"  , Break }, {"return" , Return },
	{"print"  , Print }, {"println", Println},
	{"option" , Option}, {"input"  , Input  },
	{"toint"  , Toint }, {"exit"   , Exit   },
	{"("  , Lparen    }, {")"  , Rparen     },
	{"["  , Lbracket  }, {"]"  , Rbracket   },
	{"+"  , Plus      }, {"-"  , Minus      },
	{"*"  , Multi     }, {"/"  , Divi       },
	{"=" , Equal      }, {"!=" , NotEq      },
	{"<"  , Less      }, {"<=" , LessEq     },
	{">"  , Great     }, {">=" , GreatEq    },
	{"%"  , Mod		  }, {"or",  Or		    },
	{"and", And		  }, {"not", Not	    },
	{"?"  , Ifsub     }, {"=="  , Equal     },
	{"\\" , IntDivi   }, {","  , Comma      },
	{"\"" , DblQ      }, {"!", Not          },
	{"true", True     }, {"false", False    },
	{"<-", Assign     },
	{"@dummy", END_KeyList}
};

int srcLineno;
TknKind ctyp[256];
char *token_p;
bool endOfFile_F;
char buf[LIN_SIZ+5];
ifstream fin;
#define MAX_LINE 2000

void initChTyp()
{
	int i;
	for (i=0; i<256; i++)    { ctyp[i] = Others; }
	for (i='0'; i<='9'; i++) { ctyp[i] = Digit;  }
	for (i='A'; i<='Z'; i++) { ctyp[i] = Letter; }
	for (i='a'; i<='z'; i++) { ctyp[i] = Letter; }
	ctyp['_']  = Letter;    ctyp['$']  = Doll;
	ctyp['(']  = Lparen;    ctyp[')']  = Rparen;
	ctyp['[']  = Lbracket;  ctyp[']']  = Rbracket;
	ctyp['<']  = Less;      ctyp['>']  = Great;
	ctyp['+']  = Plus;      ctyp['-']  = Minus;
	ctyp['*']  = Multi;     ctyp['/']  = Divi;
	ctyp['!']  = Not;       ctyp['%']  = Mod;
	ctyp['?']  = Ifsub;     ctyp['=']  = Equal;
	ctyp['\\'] = IntDivi;   ctyp[',']  = Comma;
	ctyp['\"'] = DblQ;
}

void fileOpen(char *fname)
{
	srcLineno = 0;
	endOfFile_F = false;
	fin.open(fname);
	if (!fin) { cout << fname << " 을(를) 열 수 없습니다.\n"; exit(1); }
}

void nextLine()
{
	string s;

	if (endOfFile_F) return;
	fin.getline(buf, LIN_SIZ+5);
	if (fin.eof()) {
		fin.clear(); fin.close();
		endOfFile_F = true; return;
	}

	if (strlen(buf) > LIN_SIZ)
		err_exit("프로그램은 1행 ", LIN_SIZ, " 문자 이내로 기술해 주세요");
	if (++srcLineno > MAX_LINE)
		err_exit("프로그램이 ", MAX_LINE, " 을(를). 넘겼습니다. ");
	token_p = buf;
}

Token nextLine_tkn()
{
	nextLine();
	return nextTkn();
}

#define CH (*token_p)
#define C2 (*(token_p+1))
#define NEXT_CH()  ++token_p
Token nextTkn()
{
	TknKind kd;
	string txt = "";

	if (endOfFile_F) return Token(EofProg);
	while (isspace(CH)) NEXT_CH();
	if (CH == '\0')  return Token(EofLine);

	switch (ctyp[CH]) {
	case Doll: case Letter:
		txt += CH; NEXT_CH();
		while (ctyp[CH]==Letter || ctyp[CH]==Digit) { txt += CH; NEXT_CH(); }
		break;
	case Digit:
		kd = IntNum;
		while (ctyp[CH] == Digit)   { txt += CH; NEXT_CH(); }
		if (CH == '.') { kd = DblNum; txt += CH; NEXT_CH(); }
		while (ctyp[CH] == Digit)   { txt += CH; NEXT_CH(); }
		return Token(kd, txt, atof(txt.c_str()));
	case DblQ:
		NEXT_CH();
		while (CH!='\0' && CH!='"') { txt += CH; NEXT_CH(); }
		if (CH == '"') NEXT_CH(); else err_exit("문자열 리터럴이 닫혀있지 않습니다. ");
		return Token(String, txt);
	default:
		if (CH=='/' && C2=='/') return Token(EofLine);
		if (is_ope2(CH, C2)) { txt += CH; txt += C2; NEXT_CH(); NEXT_CH(); }
		else                 { txt += CH; NEXT_CH(); }
	}

	kd = get_kind(txt);
	switch (kd)
	{
		case True: return Token(IntNum, 1.0f); break;
		case False: return Token(IntNum, 0.0f); break;
		case Others: err_exit("잘못된 토큰입니다. : ", txt); break;
	}

	return Token(kd, txt);
}

bool is_ope2(char c1, char c2)
{
	char s[] = "    ";
	if (c1=='\0' || c2=='\0') return false;
	s[1] = c1; s[2] = c2;
	return strstr(" ++ -- <= >= == != && || <- ", s) != NULL;
}

TknKind get_kind(const string& s)
{
	for (int i=0; KeyWdTbl[i].keyKind != END_KeyList; i++) {
		if (s == KeyWdTbl[i].keyName) return KeyWdTbl[i].keyKind;
	}
	if (ctyp[s[0]]==Letter || ctyp[s[0]]==Doll) return Ident;
	if (ctyp[s[0]]==Digit)  return DblNum;
	return Others;
}

Token chk_nextTkn(const Token& tk, int kind2)
{
	if (tk.kind != kind2) err_exit(err_msg(tk.text, kind_to_s(kind2)));
	return nextTkn();
}

void set_token_p(char *p)
{
	token_p = p;
}

string kind_to_s(int kd)
{
	if(kd == Equal)	return "=, ==";
	for (int i=0; ; i++) {
		if (KeyWdTbl[i].keyKind == END_KeyList) break;
		if (KeyWdTbl[i].keyKind == kd) return KeyWdTbl[i].keyName;
	}
	return "";
}

string kind_to_s(const CodeSet& cd)
{
	switch (cd.kind) {
	case Lvar: case Gvar: case Fcall: return tableP(cd)->name;
	case IntNum: case DblNum: return dbl_to_s(cd.dblVal);
	case String: return string("\"") + cd.text + "\"";
	case EofLine: return "";
	}
	return kind_to_s(cd.kind);
}

int get_lineNo()
{
	extern int Pc;
	return (Pc == -1) ? srcLineno : Pc;
}

