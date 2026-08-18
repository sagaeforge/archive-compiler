#include "Qi.h"
#include "Qi_prot.h"

vector<SymTbl> Gtable;
vector<SymTbl> Ltable;
int startLtable;

int enter(SymTbl& tb, SymKind kind)
{
	int n, mem_size;
	bool isLocal = is_localName(tb.name, kind);
	extern int localAdrs;
	extern Mymemory Dmem;

	mem_size = tb.aryLen;
	if (mem_size == 0) mem_size = 1;
	if (kind!=varId && tb.name[0]=='$')
		err_exit("변수명 이외에서 $를 사용할 수 없습니다. : ", tb.name);
	tb.nmKind = kind;
	n = -1;
	if (kind == fncId)  n = searchName(tb.name, 'G');
	if (kind == paraId) n = searchName(tb.name, 'L');
	if (n != -1) err_exit("이름이 중복되었습니다. : ", tb.name);

	if (kind == fncId) tb.adrs = get_lineNo();
	else {
		if (isLocal) { tb.adrs = localAdrs; localAdrs += mem_size; }
		else {
			tb.adrs = Dmem.size();
			Dmem.resize(Dmem.size() + mem_size);
		}
	}

	if (isLocal) { n = Ltable.size(); Ltable.push_back(tb); }
	else         { n = Gtable.size(); Gtable.push_back(tb); }
	return n;
}

void set_startLtable()
{
	startLtable = Ltable.size();
}

bool is_localName(const string& name, SymKind kind)
{
	if (kind == paraId) return true;
	if (kind == varId) {
		if (is_localScope() && name[0]!='$') return true; else return false;
	}
	return false;
}

int searchName(const string& s, int mode)
{
	int n;
	switch (mode) {
	case 'G':
		for (n=0; n<(int)Gtable.size(); n++) {
			if (Gtable[n].name == s) return n;
		}
		break;
	case 'L':
		for (n=startLtable; n<(int)Ltable.size(); n++) {
			if (Ltable[n].name == s) return n;
		}
		break;
	case 'F':
		n = searchName(s, 'G');
		if (n != -1 && Gtable[n].nmKind==fncId) return n;
		break;
	case 'V':
		if (searchName(s, 'F') != -1) err_exit("함수명과 중복되었습니다. : ", s);
		if (s[0] == '$')     return searchName(s, 'G');
		if (is_localScope()) return searchName(s, 'L');
		else                 return searchName(s, 'G');
	}
	return -1;
}

vector<SymTbl>::iterator tableP(const CodeSet& cd)
{
	if (cd.kind == Lvar) return Ltable.begin() + cd.symNbr;
	return Gtable.begin() + cd.symNbr;
}

