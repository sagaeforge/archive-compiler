#include "Qi.h"
#include "Qi_prot.h"

#define NO_FIX_ADRS 0
Token token;
SymTbl tmpTb;
int blkNest;
int localAdrs;
int mainTblNbr;
int loopNest;
bool fncDecl_F;
bool explicit_F;
char codebuf[LIN_SIZ+1], *codebuf_p;
extern vector<char*> intercode;

void init()
{
	initChTyp();
	mainTblNbr = -1;
	blkNest = loopNest = 0;
	fncDecl_F = explicit_F = false;
	codebuf_p = codebuf;
}

void convert_to_internalCode(char *fname)
{
	init();

	fileOpen(fname);
	while (token=nextLine_tkn(), token.kind != EofProg) {
		if (token.kind == Func) {
			token = nextTkn(); set_name(); enter(tmpTb, fncId);
		}
	}

	push_intercode();
	fileOpen(fname);
	token = nextLine_tkn();
	while (token.kind != EofProg) {
		convert();
	}

	set_startPc(1);
	if (mainTblNbr != -1) {
		set_startPc(intercode.size());
		setCode(Fcall, mainTblNbr); setCode('('); setCode(')');
		push_intercode();
	}
}

void convert()
{
	switch (token.kind) {
	case Option: optionSet(); break;
	case Var:    varDecl();   break;
	case Func:   fncDecl();   break;
	case While: case For:
		++loopNest;
		convert_block_set(); setCode_End();
		--loopNest;
		break;
	case If:
		convert_block_set();                                // if
		while (token.kind == Elif) { convert_block_set(); } // elif
		if (token.kind == Else)    { convert_block_set(); } // else
		setCode_End();                                      // end
		break;
	case Break:
		if (loopNest <= 0) err_exit("잘못된 break입니다.");
		setCode(token.kind); token = nextTkn(); convert_rest();
		break;
	case Return:
		if (!fncDecl_F) err_exit("잘못된 return입니다.");
		setCode(token.kind); token = nextTkn(); convert_rest();
		break;
	case Exit:
		setCode(token.kind); token = nextTkn(); convert_rest();
		break;
	case Print: case Println:
		setCode(token.kind); token = nextTkn(); convert_rest();
		break;
	case End:
		err_exit("잘못된 end입니다.");
		break;
	default: convert_rest(); break;
	}
}

void convert_block_set()
{
	int patch_line;
	patch_line = setCode(token.kind, NO_FIX_ADRS); token = nextTkn();
	convert_rest();
	convert_block();
	backPatch(patch_line, get_lineNo());
}

void convert_block()
{
	TknKind k;
	++blkNest;
	while(k=token.kind, k!=Elif && k!=Else && k!=End && k!=EofProg) {
		convert();
	}
	--blkNest;
}

void convert_rest()
{
	int tblNbr;

	for (;;) {
		if (token.kind == EofLine) break;
		switch (token.kind) {
		case If: case Elif: case Else: case For: case While: case Break:
		case Func:  case Return:  case Exit:  case Print:  case Println:
		case Option: case Var: case End:
			err_exit("잘못된 기술입니다. : ", token.text);
			break;
		case Ident:
			set_name();
			if ((tblNbr=searchName(tmpTb.name, 'F')) != -1) {
				if (tmpTb.name == "main") err_exit("main함수는 호출할 수 없습니다. ");
				setCode(Fcall, tblNbr); continue;
			}
			if ((tblNbr=searchName(tmpTb.name, 'V')) == -1) {
				if (explicit_F) err_exit("변수 선언이 필요합니다. : ", tmpTb.name);
				tblNbr = enter(tmpTb, varId);
			}
			if (is_localName(tmpTb.name, varId)) setCode(Lvar, tblNbr);
			else                                 setCode(Gvar, tblNbr);
			continue;
		case IntNum: case DblNum:
			setCode(token.kind, set_LITERAL(token.dblVal));
			break;
		case String:
			setCode(token.kind, set_LITERAL(token.text));
			break;
		default:
			setCode(token.kind);
			break;
		}
		token = nextTkn();
	}
	push_intercode();
	token = nextLine_tkn();
}

void optionSet()
{
	setCode(Option);
	setCode_rest();
	token = nextTkn();
	if (token.kind==String && token.text=="var") explicit_F = true;
	else err_exit("option 지정이 올바르지 않습니다.");
	token = nextTkn();
	setCode_EofLine();
}

void varDecl()
{
	setCode(Var);
	setCode_rest();
	for (;;) {
		token = nextTkn();
		var_namechk(token);
		set_name(); set_aryLen();
		enter(tmpTb, varId);
		if (token.kind != ',') break;
	}
	setCode_EofLine();
}

void var_namechk(const Token& tk)
{
	if (tk.kind != Ident) err_exit(err_msg(tk.text, "식별자"));
	if (is_localScope() && tk.text[0] == '$')
		err_exit("함수 내 var선언에서는 $가 붙은 이름을 지정할 수 없습니다 : ", tk.text);
	if (searchName(tk.text, 'V') != -1)
		err_exit("식별자가 중복되었습니다 : ", tk.text);
}

void set_name()
{
	if (token.kind != Ident) err_exit("식별자가 필요합니다. : ", token.text);
	tmpTb.clear(); tmpTb.name = token.text;
	token = nextTkn();
}

void set_aryLen()
{
	tmpTb.aryLen = 0;
	if (token.kind != '[') return;

	token = nextTkn();
	if (token.kind != IntNum)
		err_exit("배열의 길이는 양의 정수로 지정해주세요 : ", token.text);
	tmpTb.aryLen = (int)token.dblVal + 1;
	token = chk_nextTkn(nextTkn(), ']');
	if (token.kind == '[') err_exit("다차원 배열은 지원하지 않습니다. ");
}

void fncDecl()
{
	extern vector<SymTbl> Gtable;
	int tblNbr, patch_line, fncTblNbr;

	if(blkNest > 0) err_exit("함수 정의 위치가 바르지 않습니다. ");
	fncDecl_F = true;
	localAdrs = 0;
	set_startLtable();
	patch_line = setCode(Func, NO_FIX_ADRS);
	token = nextTkn();

	fncTblNbr = searchName(token.text, 'F');
	Gtable[fncTblNbr].dtTyp = DBL_T;

	token = nextTkn();
	token = chk_nextTkn(token, '(');
	setCode('(');
	if (token.kind != ')') {
		for (;; token=nextTkn()) {
			set_name();
			tblNbr = enter(tmpTb, paraId);
			setCode(Lvar, tblNbr);
			++Gtable[fncTblNbr].args;
			if (token.kind != ',') break;
			setCode(',');
		}
	}
	token = chk_nextTkn(token, ')');
	setCode(')'); setCode_EofLine();
	convert_block();

	backPatch(patch_line, get_lineNo());
	setCode_End();
	Gtable[fncTblNbr].frame = localAdrs;

	if (Gtable[fncTblNbr].name == "main") {
		mainTblNbr = fncTblNbr;
		if (Gtable[mainTblNbr].args != 0)
			err_exit("main함수에서는 가인수를 지정할 수 없습니다. ");
	}
	fncDecl_F = false;
}

void backPatch(int line, int n)
{
	*SHORT_P(intercode[line] + 1) = (short)n;
}

void setCode(int cd)
{
	*codebuf_p++ = (char)cd;
}

int setCode(int cd, int nbr)
{
	*codebuf_p++ = (char)cd;
	*SHORT_P(codebuf_p) = (short)nbr; codebuf_p += SHORT_SIZ;
	return get_lineNo();
}

void setCode_rest()
{
	extern char *token_p;
	strcpy(codebuf_p, token_p);
	codebuf_p += strlen(token_p) + 1;
}

void setCode_End()
{
	if (token.kind != End) err_exit(err_msg(token.text, "end"));
	setCode(End); token = nextTkn(); setCode_EofLine();
}

void setCode_EofLine()
{
	if (token.kind != EofLine) err_exit("잘못된 기술입니다. : ", token.text);
	push_intercode();
	token = nextLine_tkn();
}

void push_intercode()
{
	int len;
	char *p;

	*codebuf_p++ = '\0';
	if ((len = codebuf_p-codebuf) >= LIN_SIZ)
		err_exit("변환 후의 내부 코드가 너무 깁니다. 식을 줄여주세요. ");

	try {
		p = new char[len];
		memcpy(p, codebuf, len);
		intercode.push_back(p);
	}
	catch (bad_alloc) { err_exit("메모리를 확보할 수 없습니다. "); }
	codebuf_p = codebuf;
}

bool is_localScope()
{
	return fncDecl_F;
}

