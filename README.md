# Lox Interpreter 구현


Crafting interpreters 책을 참고해 인터프리터를 직접 작성해보는 개인 공부 프로젝트입니다.



총 두 가지의 인터프리터를 작성했고 두 개의 인터프리터는 다른 구조로 작성되었습니다.
* JLOX | Parser -> AST -> Resolver(정적분석 용도) -> Interpreter (Visitor Pattern)
* CLOX | Compiler -> ByteCode -> Virtual Machine (VM 방식)

## Project Status (진행도)
* **JLOX(java)** : 'com/craftinginterpreters' | 구현 완료 (Class, 상속, Closure 지원)
* **CLOX(c)** : 'clox' | 진행 중

## JLOX (with Java)
* 파서(Parser)는 Lox의 문법 형식을 참고해 AST(추상구문트리)를 생성합니다.
* JLox에서 파서는 재귀 하강 방식을 통해 구현되었습니다.
* interpreter 클래스로 AST의 노드를 방문해 문장 또는 표현식을 실행합니다. Visitor 패턴을 적용했습니다.
* 구현이 비교적 직관적이고 쉽지만 재귀 호출을 통해 노드를 순회하기 때문에 매우 느리다는 단점이 있습니다.

## CLOX (with C)
* Compiler가 Scanner를 호출해 토큰 하나씩 읽어옵니다.
* Pratt Parser 알고리즘을 통해 연산자 우선순위를 분석하고, 이를 스택 머신이 실행 가능한 순서(Postfix)로 정렬합니다.
* 바이트 코드로 변환하여 Chunk에 기록합니다.
* VM은 생성된 Chunk를 읽어들여, 스택을 이용해 명령어를 하나씩 디스패치하고 실행합니다.
* (문장, 가비지컬렉션 아직 미구현; 표현식 평가 구현 완료)
