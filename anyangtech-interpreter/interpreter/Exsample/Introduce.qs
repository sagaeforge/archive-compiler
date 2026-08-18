
// 출력문
print "Hello World"                                     // 공백을 사용하지 않고 출력
println ""                                              // 공백 출력
println "Hello Programming", "My name is Quantum"       // ,를 이용해 무한대로 가능

// 입력문
// double <- input()                                       // 입력

// 정수형 변환
// int <- toint(double)                                    // 정수형으로 표현

// 변수
a <- 10                                                 // a에 10을 넣어라
// a = 10                                               // 오류> =는 같다와 같음
b <- a = 10                                             // a와 10이 같다면 b에 참을 아니라면 거짓을
println a, " : ", b

a <- b                                                  // b의 값을 a에 넣어라
var ary[5]                                              // 배열 선언 가능 (단 1차원까지, 한번에 설정 불가)
var d, e                                                // 암묵적 변수 선언 가능

// 연산자
a <- 10
b <- 20

// 산술 연산자
c <- a + b                                              // 더하기
c <- a - b                                              // 빼기
c <- a * b                                              // 곱하기
c <- a \ b                                              // 더하기(정수까지)
c <- a / b                                              // 나누기(소수까지)
c <- a % b                                              // 나머지

// 참과 거짓
c <- true                                               // 참
c <- false                                              // 거짓

// 비교 연산자
c <- a < b                                              // 작다
c <- a <= b                                             // 작거나 같다
c <- a > b                                              // 크다
c <- a >= b                                             // 크거나 작다
c <- a == b or a = b                                    // 같다
c <- a != b                                             // 같지 않다

// 논리 연산자
c <- a == b and a = b                                   // 두 식이 참이라면
c <- a == b or a = b                                    // 두 식중 하나라도 참이라면
c <- not a == b                                         // 반대로 참이라면 거짓, 거짓이라면 참

// if문
if a = 1                                                // 만약에 a가 1이라면
    a <- 10                                             // a에 10을 넣어라
elif a == 2                                             // 만약에 a가 1이 아니면서 a가 2라면
    a <- 20                                             // a에 20을 넣어라
else                                                    // 만약에 a가 1과 2가 아니라면
    a <- 30                                             // a에 30을 넣어라
end                                                     // 조건문이 끝났음을 알림

// 반복문
// while문
a <- 0
sum <- 0
while a <= 10                                           // a가 10보다 작다면
    sum <- sum + a                                      // sum에 sum + a를 넣어라
    a <- a + 1                                          // n에 1을 더해라
end                                                     // 반복문이 끝났음을 알림
println sum

// for문
sum <- 0
for a <- 0 to 10                                        // a를 10까지
    sum <- sum + a                                      // sum에 sum + a를 넣어라
end                                                     // 반복문이 끝났음을 알림

sum <- 0
for a <- 0 to 20 step 2                                 // a를 20까지 2씩 증가시켜라, step은 음수도 가능
    sum <- sum + a                                      // sum에 sum + a를 넣어라
end

// 반복문 종료 구문
sum <- 0
for a <- 0 to 20                                        // a를 20까지 2씩 증가시켜라, step은 음수도 가능
    if a == 10                                          // a가 짝수라면
        break                                           // 종료
    else                                                // 아니라면
        sum <- sum + a                                  // sum에 sum + a를 넣어라
    end
end

sum <- 0
for a <- 0 to 20                                        // a를 20까지 2씩 증가시켜라, step은 음수도 가능
    break ? a == 10                                     // a가 짝수라면 종료
    sum <- sum + a                                      // 아니라면 sum에 sum + a를 넣어라
end

// 함수
def f()                                                 // 함수 정의
    println "Welcome function c"                        // 출력
end                                                     // 함수 종료

// 메인함수
// def main() ~ end                                     // 메인함수
                                                        // Quantum은 main함수가 있다면 메인함수에서부터 실행
                                                        // 아니라면 위에서 아래로 실행

// 매개변수
def f1(n)                                               // 함수 정의
    if(n == 2)                                          // n이 2이라면
        return 2                                        // 2 반환
    else                                                // 아니라면
        return n * f1(n - 1)                            // n * n - 1을 호출 (재귀함수)
        end
end

println f1(5)                                           // 출력

def f2(n)                                               // 함수 정의
    return 2 ? n == 2                                   // n이 2라면 2반환
    return n * f1(n - 1)                                // 아니라면 n * n - 1을 호출 (재귀함수)
end

println f2(6)                                           // 출력

// 최적화를 더 하면 이딴 것도 가능
def f3(n)                                               // 함수 정의
    return 2 ? n == 2
    return n * f3(n - 1)                                // n * n - 1을 호출 (재귀함수)
end

println f3(6)                                           // 출력

// 결론 f1 = f2 = f3

// 프로그램의 종료
exit                                                    // 정상적으로 종료
