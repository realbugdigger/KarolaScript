# KarolaScript

Introducing KarolaScript, a toy programming language built to delve deeper into the intricate mechanics of parsers, interpreters, and compilers. 
KarolaScript, inspired by the fusion of Python and C, provides a procedurally-oriented, dynamically-typed programming environment. 
It distinguishes itself by recognizing classes and functions as first-class citizens; this allows for versatile usage scenarios such as assignment, passing as function parameters, and inclusion within complex data structures.

The present method of code execution in KarolaScript involves recursive evaluation of the AST. 
An upcoming plan includes developing a virtual machine and dedicated bytecode to enhance efficiency. 
Please note, the current feature list includes class and function operations, control flows, code blocks, and advanced parser capabilities. 
Updates will be made as progress continues.

### Building

```
git clone git@github.com:realbugdigger/KarolaScript.git
cd KarolaScript
mkdir build
cd build
cmake ..
make
```

After all that you can `./KarolaScript` for REPL or `./KarolaScript example.ks` for specific file

### Grammar

```
program           → declaration* EOF ;

declaration       → classDecl | funDecl | varDecl | statement ;


classDecl         → "clazz" IDENTIFIER ( "<" IDENTIFIER )?
                    "{" member* "}" ;

member            → classMethodDecl | methodDecl ;
classMethodDecl   → "class" IDENTIFIER "(" parameters? ")" block ;
methodDecl        → function ;

funDecl           → "funct" function ;

function          → IDENTIFIER "(" parameters? ")" block ;
parameters        → IDENTIFIER ( "," IDENTIFIER )* ;

anonFunDecl       → "funct" anonFunction ;
anonFunction      → "(" parameters? ")" block ;


varDecl           → "let" IDENTIFIER ( "=" expression )? ";" ;



statement         → exprStmt | forStmt | ifStmt | printStmt | returnStmt | whileStmt | block | breakStmt;

returnStmt        → "return" expression? ";" ;

forStmt           → "for" "(" ( varDecl | exprStmt | ";" )
                    expression? ";"
                    expression? ")" statement ;

whileStmt         → "while" "(" expression ")" statement ;

exprStmt          → expression ";" ;

ifStmt            → "if" "(" expression ")" statement
                  ( "else" statement )? ;

printStmt         → "console" expression ";" ;
block             → "{" declaration* "}" ;
breakStmt         → "break" ";" ;



expression        → anonFunDecl | assignment ;

assignment        → ( call "." )? IDENTIFIER "=" logic_or | commaExpr ;

commaExpr         → logic_or ( "," logic_or )* ;

logic_or          → logic_and ( "or" logic_and )* ;
logic_and         → ternaryExpr ( "and" ternaryExpr )* ;

ternaryExpr       → equality ( "?" equality ":" ternaryExpr )? ;



equality          → comparison ( ( "!=" | "==" ) comparison )* ;
comparison        → term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
term              → factor ( ( "-" | "+" ) factor )* ;
factor            → unary ( ( "/" | "*" ) unary )* ;
unary             → unaryOperator* primary | call;

call              → primary ( "(" arguments? ")" | "." IDENTIFIER )* ;
arguments         → expression ( "," expression )* ;

primary           → errorPrimary | NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" | IDENTIFIER | "super" "." IDENTIFIER ;
errorPrimary      → binaryOperator expression ;
unaryOperator     → ( "!" | "-" )
binaryOperator    → ( "!=" | "==" | ">" | ">=" | "<" | "<=" | "-" | "+" | "/" | "*" )
```

### Standard library

KarolaScript stdlib consists of following functions:
- `console` for standard output
- `input` for standard input
- `clock` for getting epoch time
- `sleep` for thread sleep for *n* miliseconds
- `toUpper` and `toLower`

There is also builtin `Math` clazz with static functions:
- `pwr` takes first argument to the power of two
- `sqrr00t` returns square root of a argument

### Metaclasses

Metaclasses in KarolaScript are a form of [metaprogramming](https://en.wikipedia.org/wiki/Metaprogramming) as they enable the creation, modification, or management of classes in runtime.
So, a metaclass is essentially a class for a class -- it defines how a class behaves. So, just as an ordinary object is an instance of a class, a class is an instance of a metaclass.
Metaclasses allow you to change how classes are created, typically for API development or building a framework. However, they are generally considered complex to use in daily tasks. 
Therefore, they are not commonly used despite being a powerful tool. Most programmers prefer simpler, more comprehensible programming methods. 
Metaclasses can introduce unexpected behavior and complexity, which is why they are often avoided in typical programming tasks.

Currently exists only one metaclass called exactly `MetaClazz` and there is no way for user to create their own metaclass. That will change as project grows.

The motto "explicit is better than implicit" from the Zen of Python applies here. 
If metaprogramming tools like metaclasses are used, they should be applied with care and for a good reason.

### Ideas for future enhancements
- Number literals written in binary, hex, and octal
- Pratt Parser for expressions (currently it works with recursive descent for both expressions and statements)
- Arrays
- Add more functions to stdlib and more stdlib classes
- Improve error handling; Currently errors are only displayed on `stderr`, logging to a file will be a nice addition.
    - There is `ErrorReporter.h` file in utils folder. Some [strategy design pattern](https://refactoring.guru/design-patterns/strategy) can be used to select where to redirect errors and warnings.
- Add VM with garbage collection and bytecode generation
- Add lambda but make it work like in Python
- Add possiblity for user to create his own metaclass; For example user-defined metaclasses can start with `$clazz`
- Test cases
- Allow multiple code files

### Licence

[MIT licence](https://github.com/realbugdigger/KarolaScript/blob/master/LICENSE)

### LLVM version

![KarolaScript on LLVM](https://github.com/realbugdigger/KarolaScriptLLVM)
