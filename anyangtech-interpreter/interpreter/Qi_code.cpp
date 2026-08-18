#include "Qi.h"
#include "Qi_prot.h"

CodeSet code;
int startPc;
int Pc = -1;
int baseReg;
int spReg;
int maxLine;
vector<char*> intercode;
char *code_ptr;
double returnValue;
bool break_Flg, return_Flg, exit_Flg;
Mymemory Dmem;
vector<string> strLITERAL;
vector<double> nbrLITERAL;
bool syntaxChk_mode = false;
extern vector<SymTbl> Gtable;

class Mystack {
private: 
	stack<double> st;
public:
	void push(double n) { st.push(n); }
	int size() { return (int)st.size(); }
	bool empty() { return st.empty(); }
	double pop() {
		if (st.empty()) err_exit("stack underflow");
		double d = st.top();
		st.pop(); return d;
	}
};
Mystack stk;

void syntaxChk()
{
	syntaxChk_mode = true;
	for (Pc=1; Pc<(int)intercode.size(); Pc++) {
		code = firstCode(Pc);
		switch (code.kind) {
		case Func: case Option: case Var:
			break;
		case Else: case End: case Exit:
			code = nextCode(); chk_EofLine();
			break;
		case If: case Elif: case While:
			code = nextCode(); (void)get_expression(0, EofLine);
			break;
		case For:
			code = nextCode();
			(void)get_memAdrs(code);
			(void)get_expression('=', 0);
			(void)get_expression(To, 0);
			if (code.kind == Step) (void)get_expression(Step,0);
			chk_EofLine();
			break;
		case Fcall:
			fncCall_syntax(code.symNbr);
			chk_EofLine();
			(void)stk.pop();
			break;
		case Print: case Println:
			sysFncExec_syntax(code.kind);
			break;
		case Gvar: case Lvar:
			(void)get_memAdrs(code);
			(void)get_expression('=', EofLine);
			break;
		case Return:
			code = nextCode();
			if (code.kind!='?' && code.kind!=EofLine) (void)get_expression();
			if (code.kind == '?') (void)get_expression('?', 0);
			chk_EofLine();
			break;
		case Break:
			code = nextCode();
			if (code.kind == '?') (void)get_expression('?', 0);
			chk_EofLine();
			break;
		case EofLine:
			break;
		default:
			err_exit("잘못된 기술입니다 : ", kind_to_s(code.kind));
		}
	}
	syntaxChk_mode = false;
}

void set_startPc(int n)
{
	startPc = n;
}

void execute()
{
	baseReg = 0;
	spReg = Dmem.size();
	Dmem.resize(spReg+1000);
	break_Flg = return_Flg = exit_Flg = false;

	Pc = startPc;
	maxLine = intercode.size() - 1;
	while (Pc<=maxLine && !exit_Flg) {
		statement();
	}
	Pc = -1;
}

void statement()
{
	CodeSet save;
	int top_line, end_line, varAdrs;
	double wkVal, endDt, stepDt;

	if (Pc>maxLine || exit_Flg) return;
	code = save = firstCode(Pc);
	top_line = Pc; end_line = code.jmpAdrs;
	if (code.kind == If ) end_line = endline_of_If(Pc);

	switch (code.kind) {
	case If:
		if (get_expression(If, 0)) {
			++Pc; block(); Pc = end_line + 1;
			return;
		}
		Pc = save.jmpAdrs;
		while (lookCode(Pc) == Elif) {
			save = firstCode(Pc); code = nextCode();
			if (get_expression()) {
				++Pc; block(); Pc = end_line + 1;
				return;
			}
			Pc = save.jmpAdrs;
		}
		if (lookCode(Pc) == Else) {
			++Pc; block(); Pc = end_line + 1;
			return;
		}
		++Pc;
		break;
	case While:
		for (;;) {
			if (!get_expression(While, 0)) break;
			++Pc; block();
			if (break_Flg || return_Flg || exit_Flg) {
				break_Flg = false; break;
			}
			Pc = top_line; code = firstCode(Pc);
		}
		Pc = end_line + 1;
		break;
	case For:
		save = nextCode();
		varAdrs = get_memAdrs(save);

		expression('=', 0);
		set_dtTyp(save, DBL_T);
		Dmem.set(varAdrs, stk.pop());

		endDt = get_expression(To, 0);
		if (code.kind == Step) stepDt = get_expression(Step, 0); else stepDt = 1.0;
		for (;; Pc=top_line) {
			if (stepDt >= 0) {
				if (Dmem.get(varAdrs) > endDt) break;
			} else {
				if (Dmem.get(varAdrs) < endDt) break;
			}
			++Pc; block();
			if (break_Flg || return_Flg || exit_Flg) {
				break_Flg = false; break;
			}
			Dmem.add(varAdrs, stepDt);
		}
		Pc = end_line + 1;
		break;
	case Fcall:
		fncCall(code.symNbr);
		(void)stk.pop();
		++Pc;
		break;
	case Func:
		Pc = end_line + 1;
		break;
	case Print: case Println:
		sysFncExec(code.kind);
		++Pc;
		break;
	case Gvar: case Lvar:
		varAdrs = get_memAdrs(code);
		expression('=', 0);
		set_dtTyp(save, DBL_T);
		Dmem.set(varAdrs, stk.pop());
		++Pc;
		break;
	case Return:
		wkVal = returnValue;
		code = nextCode();
		if (code.kind!='?' && code.kind!=EofLine)
			wkVal = get_expression();
		post_if_set(return_Flg);
		if (return_Flg) returnValue = wkVal;
		if (!return_Flg) ++Pc;
		break;
	case Break:
		code = nextCode(); post_if_set(break_Flg);
		if (!break_Flg) ++Pc;
		break;
	case Exit:
		code = nextCode(); exit_Flg = true;
		break;
	case Option: case Var: case EofLine:
		++Pc;
		break;
	default:
		err_exit("잘못된 기술입니다 : ", kind_to_s(code.kind));
	}
}

void block()
{
	TknKind k;
	while (!break_Flg && !return_Flg && !exit_Flg) {
		k = lookCode(Pc);
		if (k==Elif || k==Else || k==End) break;
		statement();							        		
	}
}
double get_expression(int kind1, int kind2)
{
	expression(kind1, kind2); return stk.pop();
}

void expression(int kind1, int kind2)
{
	if (kind1 != 0) code = chk_nextCode(code, kind1);
	expression();
	if (kind2 != 0) code = chk_nextCode(code, kind2);
}

void expression()
{
	term(1);
}

void term(int n)
{
	TknKind op;
	if (n == 7) { factor(); return; }
	term(n+1);
	while (n == opOrder(code.kind)) {
		op = code.kind;
		code = nextCode(); term(n+1);
		if (syntaxChk_mode) { stk.pop(); stk.pop(); stk.push(1.0); }
		else binaryExpr(op);
	}
}

void factor()
{
	TknKind kd = code.kind;

	if (syntaxChk_mode) {
		switch (kd) {
		case Not: case Minus: case Plus:
			code = nextCode(); factor(); stk.pop(); stk.push(1.0);
			break;
		case Lparen:
			expression('(', ')');
			break;
		case IntNum: case DblNum:
			stk.push(1.0); code = nextCode();
			break;
		case Gvar: case Lvar:
			(void)get_memAdrs(code); stk.push(1.0);
			break;
		case Toint: case Input:
			sysFncExec_syntax(kd);
			break;
		case Fcall:
			fncCall_syntax(code.symNbr);
			break;
		case EofLine:
			err_exit("식이 바르지 않습니다. ");
		default:
			err_exit("식 오류 : ", kind_to_s(code));
		}
		return;
	}

	switch (kd) {
	case Not: case Minus: case Plus:
		code = nextCode(); factor();
		if (kd == Not) stk.push(!stk.pop());
		if (kd == Minus) stk.push(-stk.pop());
		break;
	case Lparen:
		expression('(', ')');
		break;
	case IntNum: case DblNum:
		stk.push(code.dblVal); code = nextCode();
		break;
	case Gvar: case Lvar:
		chk_dtTyp(code);
		stk.push(Dmem.get(get_memAdrs(code)));
		break;
	case Toint: case Input:
		sysFncExec(kd);
		break;
	case Fcall:
		fncCall(code.symNbr);
		break;
	}
}

int opOrder(TknKind kd)
{
	switch (kd) {
	case Multi: case Divi: case Mod:
	case IntDivi:                    return 6; /* *  /  % \  */
	case Plus:  case Minus:          return 5; /* +  -       */
	case Less:  case LessEq:
	case Great: case GreatEq:        return 4; /* <  <= > >= */
	case Equal: case NotEq:          return 3; /* == !=      */
	case And:                        return 2; /* &&         */
	case Or:                         return 1; /* ||         */
	default:                         return 0; /* 해당 없음    */
	}
}

void binaryExpr(TknKind op)
{
	double d = 0, d2 = stk.pop(), d1 = stk.pop();

	if ((op==Divi || op==Mod || op==IntDivi) && d2==0)
		err_exit("0으로 나누었습니다. ");

	switch (op) {
	case Plus:    d = d1 + d2;  break;
	case Minus:   d = d1 - d2;  break;
	case Multi:   d = d1 * d2;  break;
	case Divi:    d = d1 / d2;  break;
	case Mod:     d = (int)d1 % (int)d2; break;
	case IntDivi: d = (int)d1 / (int)d2; break;
	case Less:    d = d1 <  d2; break;
	case LessEq:  d = d1 <= d2; break;
	case Great:   d = d1 >  d2; break;
	case GreatEq: d = d1 >= d2; break;
	case Equal:   d = d1 == d2; break;
	case NotEq:   d = d1 != d2; break;
	case And:     d = d1 && d2; break;
	case Or:      d = d1 || d2; break;
	}
	stk.push(d);
}

void post_if_set(bool& flg)
{
	if (code.kind == EofLine) { flg = true; return; }
	if (get_expression('?', 0)) flg = true;
}

void fncCall_syntax(int fncNbr)
{
	int argCt = 0;

	code = nextCode(); code = chk_nextCode(code, '(');
	if (code.kind != ')') {
		for (;; code=nextCode()) {
			(void)get_expression(); ++argCt;
			if (code.kind != ',') break;
		}
	}
	code = chk_nextCode(code, ')');
	if (argCt != Gtable[fncNbr].args)
		err_exit(Gtable[fncNbr].name, " 함수의 인수 개수가 잘못되었습니다. ");
	stk.push(1.0);
}

void fncCall(int fncNbr)
{
	int  n, argCt = 0;
	vector<double> vc;

	nextCode(); code = nextCode();
	if (code.kind != ')') {
		for (;; code=nextCode()) {
			expression(); ++argCt;
			if (code.kind != ',') break;
		}
	}
	code = nextCode();

	for (n=0; n<argCt; n++) vc.push_back(stk.pop());
	for (n=0; n<argCt; n++) { stk.push(vc[n]); }

	fncExec(fncNbr);
}

void fncExec(int fncNbr)
{
	int save_Pc = Pc;
	int save_baseReg = baseReg;
	int save_spReg = spReg;
	char *save_code_ptr = code_ptr;
	CodeSet save_code = code;

	Pc = Gtable[fncNbr].adrs;
	baseReg = spReg;
	spReg += Gtable[fncNbr].frame;
	Dmem.auto_resize(spReg);
	returnValue = 1.0;
	code = firstCode(Pc);

	nextCode(); code = nextCode();
	if (code.kind != ')') {
		for (;; code=nextCode()) {
			set_dtTyp(code, DBL_T);
			Dmem.set(get_memAdrs(code), stk.pop());
			if (code.kind != ',') break;
		}
	}
	code = nextCode();

	++Pc; block(); return_Flg = false;

	stk.push(returnValue);
	Pc       = save_Pc;
	baseReg  = save_baseReg;
	spReg    = save_spReg;
	code_ptr = save_code_ptr;
	code     = save_code;
}

void sysFncExec_syntax(TknKind kd)
{
	switch (kd) {
	case Toint:
		code = nextCode(); (void)get_expression('(', ')');
		stk.push(1.0);
		break;
	case Input:
		code = nextCode();
		code = chk_nextCode(code, '('); code = chk_nextCode(code, ')');
		stk.push(1.0);
		break;
	case Print: case Println:
		do{
			code = nextCode();
			if(code.kind == String) code = nextCode();
			else (void)get_expression();		
		} while(code.kind == ',');
		chk_EofLine();
		break;
	}
}


void sysFncExec(TknKind kd)
{
	double d;
	string s;

	switch (kd) {
	case Toint:
		code = nextCode();
		stk.push((int)get_expression('(', ')'));
		break;
	case Input:
		nextCode(); nextCode(); code = nextCode();
		getline(cin, s);
		stk.push(atof(s.c_str()));
		break;
	case Print: case Println:
		do {
			code = nextCode();
			if (code.kind == String) {
				cout << code.text; code = nextCode();
			} else {
				d = get_expression();
				if (!exit_Flg) cout << d;
			}
		} while (code.kind == ',');
		if (kd == Println) cout << endl;
		break;
	}
}

int get_memAdrs(const CodeSet& cd)
{
	int adr=0, index, len;
	double d;

	adr = get_topAdrs(cd);
	len = tableP(cd)->aryLen;
	code = nextCode();

	if (len == 0)
		return adr;
	
	d = get_expression('[', ']');
	if ((int)d != d) 
		err_exit("첨자는 끝수가 없는 수치로 지정해 주세요.");
	if (syntaxChk_mode) 
		return adr;

	index = (int)d;
	if (index <0 || len <= index)
		err_exit(index,  "는 첨자 범위 밖입니다(첨자범위:0-", len-1, ")");
return adr + index;
}

int get_topAdrs(const CodeSet& cd)
{
	switch (cd.kind) {
	case Gvar: return tableP(cd)->adrs;
	case Lvar: return tableP(cd)->adrs + baseReg;
	default: err_exit("변수명이 필요합니다 : ", kind_to_s(cd));
	}
	return 0;
}

int endline_of_If(int line)
{
	CodeSet cd;
	char *save = code_ptr;

	cd = firstCode(line);
	for (;;) {
		line = cd.jmpAdrs;
		cd = firstCode(line);
		if (cd.kind==Elif || cd.kind==Else)
			continue;
		if (cd.kind == End)
			break;
	}
	code_ptr = save;
	return line;
}

void chk_EofLine()
{
	if (code.kind != EofLine) err_exit("잘못된 기술입니다 : ", kind_to_s(code));
}

TknKind lookCode(int line)
{
	return (TknKind)(unsigned char)intercode[line][0];
}

CodeSet chk_nextCode(const CodeSet& cd, int kind2)
{
	if (cd.kind != kind2) {
		if (kind2   == EofLine) err_exit("잘못된 기술입니다 : ", kind_to_s(cd));
		if (cd.kind == EofLine) err_exit(kind_to_s(kind2), " 가 필요합니다. ");
		err_exit(kind_to_s(kind2) + " 가(이) " + kind_to_s(cd) + " 앞에 필요합니다. ");
	}
	return nextCode();
}

CodeSet firstCode(int line)
{
	code_ptr = intercode[line];
	return nextCode();
}

CodeSet nextCode()
{
	TknKind kd;
	short int jmpAdrs, tblNbr;

	if (*code_ptr == '\0') return CodeSet(EofLine);
	kd = (TknKind)*UCHAR_P(code_ptr++);
	switch (kd) {
	case Func:
	case While: case For: case If: case Elif: case Else:
		jmpAdrs = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
		return CodeSet(kd, -1, jmpAdrs);
	case String:
		tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
		return CodeSet(kd, strLITERAL[tblNbr].c_str());   
	case IntNum: case DblNum:
		tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
		return CodeSet(kd, nbrLITERAL[tblNbr]);       
	case Fcall: case Gvar: case Lvar:
		tblNbr = *SHORT_P(code_ptr); code_ptr += SHORT_SIZ;
		return CodeSet(kd, tblNbr, -1);        
	default:                    
		return CodeSet(kd);
	}
}

void chk_dtTyp(const CodeSet& cd)
{
	if (tableP(cd)->dtTyp == NON_T)
		err_exit("초기화되지 않은 변수가 사용되었습니다 : ", kind_to_s(cd));
}

void set_dtTyp(const CodeSet& cd, char typ)
{
	int memAdrs = get_topAdrs(cd);
	vector<SymTbl>::iterator p = tableP(cd);

	if (p->dtTyp != NON_T) return;
	p->dtTyp = typ;
	if (p->aryLen != 0) {
		for (int n=0; n < p->aryLen; n++) { Dmem.set(memAdrs+n, 0); }
	}
}

int set_LITERAL(double d)
{
	for (int n=0; n<(int)nbrLITERAL.size(); n++) {
		if (nbrLITERAL[n] == d) return n;
	}
	nbrLITERAL.push_back(d);
	return nbrLITERAL.size() - 1;
}

int set_LITERAL(const string& s)
{
	for (int n=0; n<(int)strLITERAL.size(); n++) {
		if (strLITERAL[n] == s) return n;
	}
	strLITERAL.push_back(s);
	return strLITERAL.size() - 1;
}

