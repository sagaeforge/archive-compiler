/*
	만들면서 배우는 인터프리터를 보고 참고하여 제작하게 된 프로그램입니다.
	그래서 대부분의 사양을 이 책의 사양으로 잡아서 개발되었습니다.
	
	push 테스트 123
*/
#include "Qi.h"
#include "Qi_prot.h"

int main(int argc, char *argv[])
{
	if (argc == 1) { cout << "프로그램 사용법 : Source filename\n"; exit(1); }
	int argvsize = strlen(argv[1]);
	if(_C_not(argv[1][argvsize - 1] == 's' _C_and argv[1][argvsize - 2] == 'q')) { cout << "Quantum 소스가 아님, 확장자를 확인하세요\n"; exit(1); }
	
	// 파일 밑에 한줄을 추가하는 소스
#if __CompilerOS__ == Ubuntu
	char interpreter[300] = "sudo ./Qi_Interpreter.out ";
	strcat(interpreter, argv[1]);
	system(interpreter);
#endif

	convert_to_internalCode(argv[1]);
	syntaxChk();
	execute();
	return 0;
}

