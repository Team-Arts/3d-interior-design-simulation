# 📋 Code Convention의 목표

- 코드의 **가독성** 향상
  - 다른 개발자가 작성한 소스 코드를 빠르게 파악하여 생산성 향상
  - 향후 소프트웨어 유지보수 비용 최소화
- 코드의 **일관성** 유지
  - 팀 내의 개발자간의 혼란을 최소화
  - 코드 서식을 통일하여 소스 코드의 형식이 아니라 내용에 집중
  - 코드 형식의 불연속성을 최소화하여 코드를 읽는 리듬 유지

<br>

---

# ✏️ 표기법

<br>

## 파스칼 케이스(Pascal Case)

```
PascalCase
AdminAccount
```

- 모든 단어의 첫 글자를 대문자로 표기한다.
- 단어와 단어를 대소문자로 구분한다.

<br>

## 카멜 케이스(camel Case)

```
camelCase
playGame
```

- 첫 번째 단어만 첫 글자를 소문자로 표기하고 두 번째 이후의 단어는 첫 글자를 대문자로 표기한다.
- 단어와 단어를 대소문자로 구분한다.

<br>

## 스네이크 케이스(snake_case)

```
snake_case
SNAKE_CASE
```

- 모든 문자를 소문자로 표기하거나 대문자로 표기하여 대소문자가 섞이지 않고 소문자, 대문자 중 하나로 통일한다.
- 단어와 단어 사이를 언더스코어(_)로 연결한다.

<br>

---

# 헤더 파일

<br>

## #pragma once

모든 헤더 파일 첫 번째 줄에 `#pragma once`를 기재한다.

<br>

## #include의 이름과 순서

### #include 이름

모든 프로젝트의 헤더 파일은 상대 경로를 사용하지 않고 프로젝트의 src 디렉터리의 하위 요소로 나열되어야 한다. 예를 들어 awesome-project/src/base/logging.h는 이와 같이 #include 되어야 한다.

```cpp
#include "base/logging.h"
```

**※ 참고**

프로젝트에서 헤더 파일을 `#include "base/logging.h"`처럼 깔끔하게 경로를 설정하려면, Visual Studio를 사용하는 경우, 프로젝트 속성에서 `C/C++ -> General -> Additional Include Directories`에 `src` 디렉터리를 추가한다.

<br>

### #include 순서

주된 목적이 dir2/ExampleClass.h에 있는 것들을 구현하는 것이라면

1. dir2/ExampleClass.h
2. C 라이브러리
3. C++ 라이브러리
4. 외부 라이브러리들의 .h 파일
5. 현재 프로젝트의 .h 파일

순서로 #include를 작성한다.

<br>

각각의 부분(C 라이브러리, C++ 라이브러리 등) 안에서 #include는 알파벳 순서로 나열되어야 한다.

예를 들어 awesome-project/src/foo/fooserver.cpp의 #include들은 이렇게 보일 수 있다.

```cpp
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <hash_map>
#include <vector>

#include "base/basictypes.h"
#include "foo/public/bar.h"
```

<br>

---

# 범위

<br>

## namespace

- `using`지시자 사용 금지
- 인라인 namespace 사용 금지
- namespace가 필요할 경우 주로 경로에 기반한 이름으로 명명한다.

<br>

## 지역변수

- 최대한 첫 번째 사용처에 가까운 곳에 선언한다.
- 변수 생성 시 선언 + 할당 대신 초기화를 한다.

```cpp
// Bad: 선언 + 할당
int i;
i = f();
```

```cpp
// Good: 초기화
int j = g();
for (int i = 0; i < 10; i++)
{
    doSomething();
}
```

<br>

## 정적 변수와 전역 번수

클래스 타입의 정적 변수와 전역 변수는 가급적 사용을 자제한다.

- 정적 변수의 클래스 생성자와 초기화 함수가 호출되는 순서는 C++에서 부분적으로만 특정되어 있고, 심지어 각각의 빌드마다 호출 순서가 변경될 수 있어 찾기 힘든 버그를 만들 수 있다.

<br>

---

# 클래스

<br>

## 생성자

생성자에서 가상 함수 호출같은 복잡한 초기화 작업은 자제한다.

- 생성자에서 가상 함수를 호출할 경우, 이 호출은 하위 클래스 구현체에 전달되지 않을 것이다. 당장은 하위 클래스가 없더라도 언젠가 클래스가 수정되면 이런 문제가 조용히 생겨날 수 있고 이는 많은 혼동을 줄 것이다.

<br>

## 초기화

- 간단한 초기화는 클래스 내 멤버 변수 초기화를 통해 초기화한다.
- 클래스의 멤버 변수가 클래스 내에서 초기화되지 않고 다른 생성자도 없는 경우 반드시 디폴트 생성자(인자 없는 생성자)를 정의해야 한다. 디폴트 생성자는 되도록 내부 상태가 일관성 있고 유효하도록 객체를 초기화해야 한다.

<br>

## 구조체(struct) VS 클래스(class)

- 데이터를 나르는 수동적인 객체의 경우에만 `struct`를 사용하고 그 외의 모든 경우에는 `class`를 사용한다.
- 구조체는 데이터를 나르는 수동적 객체로서 사용되고 연관된 상수들을 포함할 수 있으나, **데이터 멤버의 값을 읽거나 쓰는 것 이외의 어떤 기능도 가져서는 안 된다.**
  - 구조체 필드 값 읽기/쓰기는 메서드 호출이 아닌 직접 필드에 접근하는 방식으로 이루어진다.
  - 구조체 메서드는 데이터 멤버를 setup하는 것 외의 동작을 제공해서는 안 된다.
- 필드 값 읽기/쓰기 외에 더 많은 기능이 필요하다면 `class`가 적절하다. 불확실한 경우에는 `class`로 만든다.
- 구조체와 클래스의 멤버 변수들은 서로 다른 네이밍 규칙을 가진다.

<br>

## 상속

- 모든 상속은 `public`상속으로 한다.
- 데이터 멤버(멤버 변수)는 항상 `private`이어야 한다.
- 하위 클래스로부터 접근이 필요할 수 있는 메서드(멤버 함수)들만 `protected`로 선언한다. 

<br>

## 다중 상속

다중 구현 상속이 실제로 유용한 경우는 매우 드물기에 상위 클래스들 중 최대 하나가 구현을 가진 경우에만 다중 상속을 허용한다. 다른 모든 상위 클래스는 `Interface`접미어가 달린 순수한 인터페이스 클래스이어야 한다.

<br>

## 선언 순서

<br>

### 접근 지정자 순서

1. `public`
2. `protected`
3. `private`

이 중 비어있는 것이 있다면, 그 부분을 생략한다.

<br>

### 각각의 접근 지정자 안에서의 순서

1. typedef
2. 열거형
3. 상수(`static const` 멤버 변수)
4. 생성자
5. 소멸자
6. 메서드(정적 메서드 포함)
7. 멤버 변수(`static const`멤버 변수 제외)

`friend` 선언은 항상 private 부분 안에 있어야 한다.

<br>

---

# 그 외의 C++ 기능

<br>

## 레퍼런스(&) 인자

레퍼런스로 전달되는 모든 인자는 `const`이어야 한다.

- 레퍼런스는 포인터의 의미를 가지지만 값(value)과 같은 문법을 사용하므로 혼란스러울 수 있다.

<br>

## 함수 오버로드

읽는 사람이 호출부를 살펴볼 때, 굳이 어느 오버로드가 정확히 호출되었는지 먼저 알아내지 않고도 무슨 일이 벌어졌는지 잘 파악할 수 있는 경우에만 (생성자를 포함하여) 오버로드된 함수를 사용한다.

- 함수를 오버로드 하기를 원하면 인자의 타입 이름을 접미어로 하여 함수 이름을 짓는다.
  - 예를 들면 단순한 `append()`보다 `appendString()`이나 `appendInt()`.

<br>

## 형 변환(Type Casting)

- int y = (int)x; 또는 int y = int(x); 같은 C 스타일의 형 변환을 사용하지 않는다.
- `static_cast<>()`같은 C++ 스타일 형 변환을 사용한다.
  - C 스타일 형 변환처럼 연산이 모호할 수 있는 문제를 방지하고 C 스타일 형 변환보다 검색하기도 좋다.

<br>

## I/O(입출력)

I/O를 할 때 모든 곳에서 같은 형태의 코드를 보는 일관성을 유지하는 것이 좋다.

- C 스타일인 `scanf()`, `printf()`는 사용을 자제한다.
- C++ 스타일인 `cin`, `cout`을 사용한다.

<br>

## 0과 nullptr/NULL

정수는 `0`을, 실수는 `0.0`을, 포인터는 `nullptr`를, char는 `'\0'`을 사용한다.

<br>

## sizeof

`sizeof(type)`보다는 `sizeof(varname)`사용을 권장한다.

- 특정 변수의 사이즈가 필요할 때 `sizeof(varname)`을 사용하라. `sizeof(varname)`은 누군가가 그 변수의 타입을 변경했을 때에도 문제가 없다. `sizeof(type)`은 특정 변수의 사이즈와 관련 없는 곳에서 사용될 수 있다.

<br>

## typedef

- 구조체, 열거형을 typedef로 정의하지 않는다.
  - typedef로 정의하면 구조체, 열거형과 클래스간에 구별이 힘들어지므로 구조체를 사용할 땐 `struct LaptopInfo laptop_info` 형식으로 선언하여 사용한다.

```cpp
// Good
struct LaptopInfo
{
}

enum eColor
{
};
...
    struct LaptopInfo laptop_info;
    eColor color;
```

```cpp
// Bad
typedef struct LaptopInfo
{
}LaptopInfo;
```

<br>

## 열거형

열거형 변수를 선언할 때 `enum`을 생략하여 작성한다.

```cpp
enum eColor
{
    kRed,
    kGreen,
    kBlue
};

eColor color;
```



<br>

---

# 네이밍 규칙

<br>

## 일반 이름 규칙

가능하면 약어를 피하고 서술적으로 이름을 짓는다.

```cpp
// Good
int price_count_reader; // 축약 없음
int num_errors; // "num"은 누구나 이해 가능
```

<br>

```cpp
// Bad
int n; // 의미없음
int nerr; // 모호한 축약
int pc_reader; // "pc"는 다양한 의미가 있다.
```

<br>

## 파일

- 클래스 헤더(.h), 클래스 구현부(.cpp) 파일 이름은 클래스 이름과 동일하게 한다.

  ```
  Character.h
  Character.cpp
  Stair.h
  Stair.cpp
  ```

- 그 외의 모든 소스 코드 파일은 모두 소문자로 작성하고 언더스코어(_)를 포함할 수 있다.

<br>

## 타입

<br>

### 클래스, 구조체, typedef

클래스, 구조체, typedef 타입 이름은 파스칼 케이스로 작성한다.

```cpp
class AwesomeLaptop
{
}

struct LaptopInfo
{
}

...
```



<br>

### 열거형

열거형을 선언할 때는 첫 글자를 `e`로 작성하고 두 번째 문자 이후로는 파스칼 케이스로 표기한다.

```cpp
enum eDirection
{
    kNorth,
    kSouth
};
```

<br>

## 변수

<br>

### 지역 변수, 구조체 변수, 열거형 변수

지역 변수, 전역 변수, 구조체 변수, 열거형 변수는 스네이크 케이스로 작성한다.

```cpp
string table_name;
struct LaptopInfo laptop_info;
eTableColor table_color;
```

<br>

### 클래스 멤버 변수(데이터 멤버)

클래스 멤버 변수는 스네이크 케이스로 작성한 다음, 항상 끝에 언더스코어(_)를 붙인다.

```cpp
string table_name_;
```

<br>

### 전역 변수

전역 변수는 가급적 사용하지 않는 것이 좋지만 사용할 때에는 맨 앞에 `g_`로 작성하고 그 다음엔 스네이크 케이스로 작성한다.

```cpp
int g_num_chair;
```

<br>

## 상수

첫 글자를 `k`로 시작하고 그 다음엔 파스칼 케이스로 작성한다.

```cpp
const int kDaysInAWeek = 7;
```

<br>

## 함수

<br>

- 일반 함수, 메서드(멤버 함수)의 경우 카멜 케이스로 이름 짓는다.
- getter, setter는 `get` 또는 `set`을 맨 앞에 작성하고 그 다음 문자부터 해당하는 변수의 이름을 파스칼 케이스로 작성한다.

```cpp
void playGame()
{
}

...
    
class AwesomeWeapon
{
public:
    int getRange();
    int setRange();
    void moveForward();
private:
    int range_;
}
```

<br>

## namespace

네임스페이스 이름은 모두 소문자로 하며, 경로 구조에 기반하여 작성한다.

<br>

## 열거형 멤버 변수

열거형 멤버 변수는 상수 방식(첫 글자로 `k` 작성, 다음 글자부터 파스칼 케이스)으로 작성한다.

 ```cpp
 enum eColor
 {
     kRed,
     kGreen,
     kBlue
 };
 ```

<br>

---

# 주석

<br>

## 주석 스타일

- /* */ 형식 사용을 자제하고 // 형식을 활용한다
- 설명하려는 코드의 바로 윗 줄에 작성한다.
- 코드가 짧고 주석도 짧을 경우 코드와 같은 줄의 오른쪽에 작성해도 좋다.
- 주석 내용을 작성할 땐 //를 쓰고 스페이스를 한 칸 넣고 작성한다.
- 명령문과 같은 줄에 주석을 작성할 땐 스페이스를 한 칸 넣고 작성한다.

```cpp
// 캐릭터를 앞으로 이동시킬 때 사용한다.
void moveForward();

// 캐릭터를 앞으로 이동시킨다.
void moveForward()
{
    
}

int speed = 100; // 명령문과 같은 줄에 주석 작성
```



<br>

## 파일 주석

- 일반적으로 .h파일은 그 파일 안에 정의된 클래스에 대한 설명과 그 클래스가 어떻게 사용되는지에 대한 설명을 포함해야 한다.
- .cpp파일은 구현에 대한 세부사항이나 다루기 힘든 알고리즘에 대한 논의를 담아야 한다.

<br>

## 함수 주석

- 함수 선언 주석은 함수의 사용을 설명한다.
- 함수 정의의 주석은 함수의 동작을 설명한다.

```cpp
// 캐릭터를 앞으로 이동시킬 때 사용한다.
void moveForward();

// 캐릭터를 앞으로 이동시킨다.
void moveForward()
{
    
}
```

<br>

## 변수 주석

- 클래스의 멤버 변수들은 그것이 무엇이고 어디에 사용되는지 설명하는 주석을 가져야 한다.

```cpp
private:
	// 테이블에 있는 엔트리의 총 개수를 기록한다.
 	// 한계를 넘지 않도록 보장하는 데 사용된다. -1은 테이블에 얼마나 많은
 	// 엔트리가 들어있는지 아직 알지 못하는 경우를 의미한다.
	int num_total_entries_;
```

<br>

- 모든 전역 변수는 클래스의 멤버 변수와 마찬가지로 그것이 무엇이고 어디에 사용되는지 설명하는 주석을 가져야 한다.

```cpp
// 이 회귀 테스트에서 통과한 테스트 케이스의 총 개수
const int g_num_test_cases = 6;
```

<br>

## 구현 주석

구현 로직에서 까다롭거나, 명백하지 않거나, 중요한 부분은 주석을 가져야 한다.

<br>

## 구두점, 철자, 문법

- 구두점, 철자, 문법에 주의하라. 구두점, 철자, 문법이 제대로 사용된 주석이 이해하기 쉽다.
- 주석은 일반적인 문장과 마찬가지로 읽기가 쉬워야 하고, 적절한 대소문자와 구두점이 사용되어야 한다.

<br>

## TODO 주석

- 아직 완벽하지 않은 코드 혹은 임시적이고 단기적인 해결책에 `TODO` 주석을 사용해야 한다.
- `TODO` 주석의 내용으로는 구현해야 할 로직이 무엇인지 작성하거나 해당 부분에서 다른 해야 할 일을 작성한다.
- `TODO` 주석에서 `TODO`는 모두 대문자로 해야 하며, 이름, 이메일 등으로 작성자를 적어야 한다.
- `TODO` 다음에는 () 안에 작성자를 적고 () 다음엔 `:`를 작성하여 `TODO(작성자):`형태가 되도록 작성한다.
- `:`다음엔 한 칸 띄어쓰고 내용을 작성한다.

일관성 있는 `TODO` 포맷을 사용하여 필요할 때 올바른 상황 설명을 해 줄 사람을 찾을 수 있게 하는 것이 주 목적이다. `TODO` 주석은 작성자가 그 문제를 고친다는 약속은 아니다. 그러므로 `TODO` 주석에 언제나 이름을 포함하자.

```cpp
// TODO(이준혁): 연결 연산자로 "*"를 사용하라.
// TODO(visualnnz): 연관 관계를 사용하도록 이 부분을 수정.
```

<br>

---

# 포매팅(Formatting)

코딩 스타일과 포매팅은 제멋대로인 경우가 많지만, 모두가 통일된 스타일을 쓴다면 프로젝트를 파악하기가 훨씬 쉬워진다. 개개인이 모든 포매팅 규칙에 다 동의하기는 어렵고, 어떤 규칙은 익숙해지는데 시간이 걸리지만, 프로젝트의 구성원들이 규칙을 따라서 다른 사람의 코드를 쉽게 이해하도록 하는 것은 중요하다.

<br>

## 공통 사항

- 콤마(,) 뒤에는 스페이스를 한 칸 넣고 내용을 작성한다.
- 들여쓰기는 스페이스 4 칸을 기본으로 한다.
- 괄호와 문자, 숫자 사이에 스페이스를 넣지 않는다.

```cpp
int *ptr = new int [10];

int doSomething(char *str, int num1)
{
    ...
}
```

<br>

## 줄 길이

코드의 각 줄은 가급적 80 문자를 넘지 않게 작성한다. 

- 길이 80자는 기존의 많은 코드들이 이미 따르고 있는 전통적인 표준이다.

<br>

## 스페이스 VS Tab

Tab 사용을 자제하고 스페이스만 사용하고 4개의 스페이스(4칸)로 들여쓰기 한다.

- Tab을 눌렀을 때 스페이스가 입력되도록 편집기를 설정할 수 있다.

<br>

## 함수 선언과 정의

- 여는 소괄호는 항상 함수 이름과 같은 줄에 작성한다.
- 함수 이름과 여는 소괄호 사이에는 스페이스를 넣지 않고 붙여서 작성한다.
- 가능하면 리턴 타입과 함수 이름, 파라미터를 같은 줄에 작성한다.
- 한 줄에 넣기에 글자가 너무 많다면 파라미터 작성 시 줄 바꿈을 할 때 첫 번째 파라미터와 같은 열로 맞춘다.

```cpp
ReturnType ClassName::FunctionName(Type par_name1, Type par_name2)
{
    doSomething();
    ...
}
```

한 줄에 넣기에 글자가 너무 많다면

```cpp
ReturnType ClassName::FunctionName(Type par_name1, Type par_name2, 
                                   Type par_name3)
{
    doSomething();
    ...
}
```

혹은 첫 번째 파라미터도 맞추기 힘들다면

```cpp
ReturnType LongClassName::ReallyReallyLongFunctionName(
    Type par_name1,
    Type par_name2,
    Type par_name3)
{
    doSomething();
    ...
}
```

<br>

## 중괄호

함수 정의, 조건문, 반복문 등 모든 범위 지정 중괄호 작성 스타일은 여는 중괄호와 닫는 중괄호가 같은 열에 위치하도록 하는 **BSD 스타일**로 작성한다.

**※ 참고**

K&R

```cpp
int doSomething(){
...
}
```

<br>

**BSD**

```cpp
int doSomething()
{
    ...
}
```

<br>

## 함수 호출

- 자리가 충분한 경우 한 줄에 쓰고, 그렇지 않은 경우 괄호 안의 인자들을 줄바꿈한다.

```cpp
bool retval = doSomething(argument1, argument2, argument3);
```

<br>

- 인자들이 모두 한 줄에 들어갈 자리가 없다면 여러 줄로 나누어 쓰되 이어지는 줄은 첫 번째 인자와 같은 열에 오도록 한다. 괄호와 인자 사이에는 스페이스를 두지 않고 붙여서 작성한다.

```cpp
bool retval = doSomething(argument1,
                          argument2,
                          argument3,
                          argument4);
```

<br>

## 조건문

<br>

### if-else

- `else`키워드는 줄바꿈해서 사용한다.
- `if`와 `else if` 키워드와 괄호 사이에는 스페이스를 한 칸 넣는다.
- 명령문이 한 줄이라도 중괄호로 감싸서 작성한다.

```cpp
if (condition)
{
    return something;
}
else if (condition)
{
    ...
}
else
{
    ...
}
```

<br>

### switch

- `switch` 키워드와 괄호 사이에는 스페이스를 한 칸 넣는다.
- `case` 블록은 중괄호를 사용하지 않는다.
- 열거형 값에 대한 조건이 아닌 경우, switch 문은 항상 `default`케이스를 포함한다.
- `default` 케이스가 실행되지 말아야 할 경우 `assert(false);` 구문을 넣어둔다.

```cpp
switch (var)
{
    case 0:
        ...
        break;
    case 1:
        ...
        break;
    default:
        assert(false);
}
```

<br>

## 반복문

## for

- `for` 키워드와 괄호 사이에는 스페이스를 한 칸 넣는다.
- 초기식을 작성할 땐 `int i = 0;`형식으로 변수 뿐만 아니라 타입도 포함한다.
- 제어식에서는 기본적으로 후치`i++` 형식으로 작성하되 필요시 전치`++i` 형식으로 작성할 수 있다.
- 명령문이 한 줄이라도 중괄호로 감싸서 작성한다.
- 명령문이 없는 비어있는 루프는 `for` 키워드와 같은 줄에 `{}`를 작성한다. 

```cpp
for (int i = 0; i < kSomeNumber; i++) {} // 비어 있는 루프

for (int i = 0; i < kSomeNumber; i++)
{
    doSomething;
}
```

<br>

### while

- `while` 키워드와 괄호 사이에는 스페이스를 한 칸 넣는다.
- 명령문이 한 줄이라도 중괄호로 감싸서 작성한다.
- 명령문이 없는 비어있는 루프는 `while` 키워드와 같은 줄에 `{}`를 작성한다. 

```cpp
while (condition) {} // 비어 있는 루프

while (condition)
{
    doSomething;
}
```

<br>

## 포인터(*), 레퍼런스(&)

- 포인터 변수나 파라미터 선언 시 '*'는 변수 명에 붙여서 작성한다.
- 마침표(.) 좌우나 화살표(->) 좌우, 포인터 연산자 '*'이나 '&' 뒤에는 스페이스를 넣지 않고 붙여서 작성한다.
- 레퍼런스(&) 변수나 파라미터 선언 또한 '&'는 변수 명에 붙여서 작성한다.

```cpp
int *p = &x;
const int &ref_a = a;
*p = 5;
x = r.y;
x = r->y;
```

<br>

## bool 표현식

- 정해진 줄 길이보다 긴 bool 표현식을 사용해야 할 때에는 중간에 있는 `&&` 또는 `||` 연산자가 줄의 마지막에 오도록 작성한다.
- 가독성을 높이는 것에 도움이 될 경우 추가적인 괄호를 적절히 사용하여도 좋다.
- `and`나 `compl`과 같은 단어로 된 연산자는 사용하지 않고 `&&`, `||`, `~`와 같은 기호 연산자를 사용한다.

```cpp
if (this_one_thing > this_other_thing &&
   a_third_thing == a_fourth_thing &&
   yet_another && last_one)
{
    ...
}
```

<br>

## return

- `return` 표현식을 불필요하게 괄호로 묶지 않는다.
- 복잡한 표현식의 가독성을 높일 수 있다면 괄호 사용을 허용한다.

```cpp
return result;
return (some_long_condition && 
       another_condition);
```

<br>

## 클래스 포맷

- 접근 지정자는 `public`, `protected`, `private` 순서로 스페이스 없이 들여쓰기하고 접근 지정자 이후부터는 4 칸씩 들여쓰기 하여 클래스 본문을 구성한다.
- `public`, `protected`, `private`는 첫 번째로 사용되는 경우를 제외하고는 이들 키워드의 이전에 빈 줄을 삽입한다. 이들 키워드 이후에는 빈 줄을 삽입하지 않는다.

```cpp
class ChildClass : public ParentClass
{
public:
    ChildClass();
    ChildClass(int var);
    ~ChildClass() {}
    
    void doSomething();
    void setSomeVar(int var)
    {
        some_var_ = var;
    }
    
private:
    bool start_ = false;
    int some_var_ = 0;
};
```

<br>

## 세로 공백

- 함수의 시작이나 끝에 빈 줄을 사용하지 않는다.
- 지나치게 조밀한 코드는 가독성을 해치므로 가독성을 위해 명령문 사이에 빈 줄을 의미 단위로 적절히 추가한다.

<br>

## 기타

- 명령문과 문장 마침 기호인 세미콜론 `;`은 붙여쓰고 `;` 다음에 코드를 작성할 경우 스페이스를 한 칸 넣고 작성한다.
- 하나의 변수를 연산할 때 `x += 5;`형식 보단  `x = x + 5;`, `x = x * 3;`형식으로 작성한다.

<br>
