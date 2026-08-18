# Quantum Language 

이 프로젝트는 만들면서 배우는 인터프리터를 참고하여 제작되었습니다.

자세한 인터프리터 파일 사양은 https://www.notion.so/Quantum-Language-037fe8f8907b43f98ee0078eaa8dbf6e 에서 확인이 가능합니다.


# 언어 컴파일 사양

Compiler : GNU Compiler C 9.3.0

Compile OS : Ubuntu 20.04.1 LTS



*Test

Windows 10

Ubuntu 20.04.1 LTS



# 사용법
  실행 파일은 Excute 폴더에, 예제 파일은 Exsample 폴더에 있습니다.


 1. cmd (유저모드)
 2. 프로그램이 있는 위치까지 이동 (exsample cd c:\\~interpreter\\Excute)
 3. 소스 파일 실행 (.\qi.exe target.source)
 4. 결과 확인

# 문제 발생하는 경우
 1. 글자가 깨집니다.

    글자가 깨질경우 cmd가 기본으로 cp-949나 한글 인코딩(enc-kor)을 사용하고 있을 텐데
    chcp를 통해 유니코드로 변경합니다.
    
    chcp 65001  # 유니코드로 현재 인코딩을 변경
 
 2. end가 없다고 뜨는 경우

    파일에서 줄을 인식할 때 약간의 차이가 존재하는 데
    소스코드에 엔터 한줄을 치면됩니다.
    
 3. libgcc 또는 libg++ 없다고 나오면서 실행이 안되는 경우
    
    GCC 나 G++에 관련된 라이브러리가 없어서 발생한 문제입니다.
    
    윈도우의 경우 mingw,
    리눅스와 맥등 유닉스 환경에서 build-essential을 다운 받으시면 오류가 해결됩니다.
    
 4. 파일이 실행이 안됨
    
    파일 주소 중에 한글이 있을 경우 문제가 발생할 가능성이 높습니다.
    
    한글이 없는 위치에 프로그램을 옮겨주세요 
