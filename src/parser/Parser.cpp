#include "Parser.h"

Token Parser::previous() {
    return tokens.at(current - 1);
}

Token Parser::peek() {
    return tokens.at(current);
}

bool Parser::isAtEnd() {
    return peek().type == TOKEN_EOF;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return peek().type == type;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();

    throw error(peek(), message);
}


// Call error() to just report the error and keep parsing, but throw it when parser need's to synchronize
Parser::ParseError Parser::error(const Token& token, const std::string& message) {
    ErrorReporter::error(token, message.c_str());
    return ParseError();
}

void Parser::synchronize() { //synchronizes the parser to the next statement when it finds an error
    advance();

    while (!isAtEnd()) {
        if (previous().type == TOKEN_SEMICOLON) return;

        switch (peek().type) {
            case TOKEN_CLAZZ:
            case TOKEN_FUNCT:
            case TOKEN_LET:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_KONSOLE:
            case TOKEN_RETURN:
                return; //found new statement;
        }

        advance(); //keep advancing until finding a new statement
    }
}

bool Parser::match(const std::initializer_list<TokenType>& types) {
    for (auto type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

UniqueExprPtr Parser::expression() {
    return assignment();
}

UniqueExprPtr Parser::assignment() {
    UniqueExprPtr expr = commaExpression();

    if (match({ TOKEN_EQUAL })) {
        Token equals = previous();
        UniqueExprPtr value = assignment();

        //Checks if the parsed expression to the left of the '=' is a variable expression that we can assign to
        auto variable = dynamic_cast<Variable*>(expr.get());
        if (variable != nullptr) {
            Token name = variable->m_VariableName;
            return std::make_unique<Assign>(name, std::move(value));
        }

        //Checks if the parsed expression to the left of the '=' is a get expression such as obj.field that we can assign to
        auto get = dynamic_cast<Get*>(expr.get());
        if (get != nullptr) {
            return std::make_unique<Set>(std::move(get->m_Object), get->m_Name, std::move(value));
        }

//            throw error(equals, "Invalid assignment target.");
        error(equals, "Invalid assignment target.");
    }

    // if you don’t hit an `=`, report an error if the left-hand side isn’t a valid expression.
//        error(previous(), "Invalid expression. Did you try to perform assignment?"); // previous() ????

    // consume(SEMICOLON, "Expect ';' after expression."); ??????????

    return expr;
}

UniqueExprPtr Parser::commaExpression() {
    UniqueExprPtr expr = orSmt();

    while (match({TOKEN_COMMA})) {
        expr = orSmt();
    }

    return expr;
}

UniqueExprPtr Parser::orSmt() {
    UniqueExprPtr expr = andSmt();

    while (match({ TOKEN_OR })) {
        Token operation = previous();
        UniqueExprPtr right = andSmt();
        expr = std::make_unique<Logical>(std::move(expr), operation, std::move(right));
    }

    return expr;
}

UniqueExprPtr Parser::andSmt() {
    UniqueExprPtr expr = ternaryExpression();

    while (match({ TOKEN_AND })) {
        Token operation = previous();
        UniqueExprPtr right = ternaryExpression();
        expr = std::make_unique<Logical>(std::move(expr), operation, std::move(right));
    }

    return expr;
}

UniqueExprPtr Parser::ternaryExpression() {
    UniqueExprPtr expr = equality();

    if (match({TOKEN_QUESTION_MARK})) {
        UniqueExprPtr trueExpr = equality();
        consume(TOKEN_COLON, "Expected ':' after ? in ternary operator.");
        UniqueExprPtr falseExpr = ternaryExpression();
        expr = std::make_unique<Ternary>(std::move(expr), std::move(trueExpr), std::move(falseExpr));
    }

    return expr;
}

UniqueExprPtr Parser::equality() {
    UniqueExprPtr expr = comparison();

    while (match({ TOKEN_BANG_EQUAL, TOKEN_EQUAL_EQUAL })) {
        Token operation = previous();
        UniqueExprPtr right = comparison();
        expr = std::make_unique<Binary>(std::move(expr), operation, std::move(right));
    }

    return expr;
}

UniqueExprPtr Parser::comparison() {
    UniqueExprPtr expr = term();

    while (match({ TOKEN_GREATER, TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL })) {
        Token operation = previous();
        UniqueExprPtr right = term();
        expr = std::make_unique<Binary>(std::move(expr), operation, std::move(right));
    }
    return expr;
}

UniqueExprPtr Parser::term() {
    UniqueExprPtr expr = factor();

    while (match({TOKEN_MINUS, TOKEN_PLUS})) {
        Token operator_ = previous();
        UniqueExprPtr right = factor();
        expr = std::make_unique<Binary>(std::move(expr), operator_, std::move(right));
    }

    return expr;
}

UniqueExprPtr Parser::factor() {
    UniqueExprPtr expr = unary();

    while (match({TOKEN_SLASH, TOKEN_STAR})) {
        Token operator_ = previous();
        UniqueExprPtr right = unary();
        expr = std::make_unique<Binary>(std::move(expr), operator_, std::move(right));
    }

    return expr;
}

UniqueExprPtr Parser::unary() {
    if (match({TOKEN_BANG, TOKEN_MINUS})) {       //unaryOperator
        Token operator_ = previous();
        UniqueExprPtr right = unary();
        return std::make_unique<Unary>(operator_, std::move(right));
    }

    // this if block can probably go to primary() but checked first
    if (match({TOKEN_BANG_EQUAL, TOKEN_EQUAL_EQUAL, TOKEN_GREATER,
               TOKEN_GREATER_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL,
               TOKEN_MINUS, TOKEN_PLUS, TOKEN_SLASH, TOKEN_STAR})) {         //binaryOperator
        error(previous(), "Binary operator without left-hand operand.");
        expression();                   // Discard a right-hand operand
    }

    return call();
}

UniqueExprPtr Parser::finishCall(UniqueExprPtr callee) {
    std::vector<UniqueExprPtr> arguments;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            if (arguments.size() >= 255) {
                // Report the error but don't throw it bc throwing it will cause the parser to synchronize and we want to keep parsing
                error(peek(), "Cannot have more than 255 arguments.");
            }

            if (match({TOKEN_FUNCT}))
                arguments.push_back(anonymousFunction());
            else
                arguments.push_back(ternaryExpression());
        } while (match({ TOKEN_COMMA }));
    }

    Token closingParen = consume(TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");

    return std::make_unique<Call>(std::move(callee), closingParen, std::move(arguments));
}

UniqueExprPtr Parser::call() {
    UniqueExprPtr expr = primary();

    while (true) {
        if (match({ TOKEN_LEFT_PAREN })) {
            expr = finishCall(std::move(expr));
        } else if (match({ TOKEN_DOT })) {
            Token name = consume(TOKEN_IDENTIFIER, "Expected property name after '.'.");
            expr = std::make_unique<Get>(name, std::move(expr));
        } else {
            break;
        }
    }
    return expr;
}

UniqueExprPtr Parser::anonymousFunction() {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'funct'.");
    std::vector<Token> parameters;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            if (parameters.size() >= 255) {
                error(peek(), "Can't have more than 255 parameters.");
            }

            parameters.push_back(consume(TOKEN_IDENTIFIER, "Expected parameter name."));
        } while (match({TOKEN_COMMA}));
    }
    Token errorToken = consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before fun body.");
    std::vector<UniqueStmtPtr> body = block();

    if (body.empty())
        throw error(errorToken, "Anonymous function is declared without being used.");

    return std::make_unique<AnonFunction>(parameters, std::move(body));
}

UniqueExprPtr Parser::primary() {
    // replace all with previous() inside Object constructor

    if (match({ TOKEN_FALSE })) {
        return std::make_unique<Literal>(Object(false));
    }
    if (match({ TOKEN_TRUE })) {
        return std::make_unique<Literal>(Object(true));
    }
    if (match({ TOKEN_NULL })) {
        return std::make_unique<Literal>(Object::Null());
    }

    if (match({ TOKEN_NUMBER })) {
        return std::make_unique<Literal>(Object(std::strtod(previous().lexeme.c_str(), nullptr)));
    }
    if (match({ TOKEN_STRING })) {
        return std::make_unique<Literal>(Object(previous().lexeme));
    }

    if (match({ TOKEN_SUPER })) {
        Token keyword = previous();
        consume(TOKEN_DOT, "Expected '.' after 'super'.");
        Token method = consume(TOKEN_IDENTIFIER,"Expected identifier method super.");
        return std::make_unique<Super>(keyword, method);
    }

    if (match({ TOKEN_THIS })) {
        return std::make_unique<This>(previous());
    }

    if (match({ TOKEN_IDENTIFIER })) {
        return std::make_unique<Variable>(previous());
    }

    if (match({ TOKEN_LEFT_PAREN })) {
        UniqueExprPtr expr = expression();
        consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
        return std::make_unique<Grouping>(std::move(expr));
    }

    throw error(peek(), "Expected expression.");
}

UniqueStmtPtr Parser::forStatement() {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");

    // declaring or initializing variable
    UniqueStmtPtr initializer;
    if (match({ TOKEN_SEMICOLON })) {
        initializer = nullptr;
    } else if (match({ TOKEN_LET })) {
        initializer = letDeclaration();
    } else {
        initializer = expressionStatement();
    }

    // the condition
    UniqueExprPtr condition = nullptr;
    if (!check(TOKEN_SEMICOLON)) {
        condition = expression();
    }
    consume(TOKEN_SEMICOLON, "Expected ';' after loop condition.");

    // the incrementation
    UniqueExprPtr increment = nullptr;
    if (!check(TOKEN_RIGHT_PAREN)) {
        increment = expression();
    }
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after for clauses.");

    UniqueStmtPtr body = statement();

    if (increment != nullptr) {
        std::vector<UniqueStmtPtr> stmts;
        stmts.push_back(std::move(body));
        stmts.push_back(std::make_unique<Expression>(std::move(increment)));
        body = std::make_unique<Block>(std::move(stmts));
    }

    if (condition == nullptr) {
        condition = std::make_unique<Literal>(Object(true));
    }
    body = std::make_unique<While>(std::move(condition), std::move(body));

    if (initializer != nullptr) {
        std::vector<UniqueStmtPtr> stmts;
        stmts.push_back(std::move(initializer));
        stmts.push_back(std::move(body));
        body = std::make_unique<Block>(std::move(stmts));
    }

    return body;
}

UniqueStmtPtr Parser::ifStatement() {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'.");
    UniqueExprPtr condition = expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after if condition.");

    UniqueStmtPtr thenBranch = statement();
    UniqueStmtPtr elseBranch = nullptr;
    if (match({ TOKEN_ELSE })) {
        elseBranch = statement();
    }

    return std::make_unique<If>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

UniqueStmtPtr Parser::konsoleStatement() {
    if (match({TokenType::TOKEN_SEMICOLON})){
        return std::make_unique<Print>(std::nullopt);
    }

    UniqueExprPtr value = expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after value.");
    UniqueStmtPtr print = std::make_unique<Print>(std::move(value));
    return print;
}

UniqueStmtPtr Parser::returnStatement() {
    Token keyword = previous();
    std::optional<UniqueExprPtr> value = std::nullopt;
    if (!check(TOKEN_SEMICOLON)) {
        value = expression();
    }

    consume(TOKEN_SEMICOLON, "Expected ';' after return value.");
    return std::make_unique<Return>(keyword, std::move(value));
}

UniqueStmtPtr Parser::whileStatement() {
    consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
    UniqueExprPtr condition = expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");
    UniqueStmtPtr body = statement();

    return std::make_unique<While>(std::move(condition), std::move(body));
}

UniqueStmtPtr Parser::breakStatement() {
    Token keyword = previous();

    consume(TOKEN_SEMICOLON, "Expected ';' after 'break'.");
    return std::make_unique<Break>(keyword);
}

std::vector<UniqueStmtPtr> Parser::block() {
    std::vector<UniqueStmtPtr> statements;

    while (!check(TOKEN_RIGHT_BRACE) && !isAtEnd()) {
        statements.push_back(declaration());
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
    return statements;
}

UniqueStmtPtr Parser::expressionStatement() {
    UniqueExprPtr expr = expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
    UniqueStmtPtr expression = std::make_unique<Expression>(std::move(expr));
    return expression;
}

UniqueStmtPtr Parser::statement() {
    if (match({ TOKEN_FOR })) {
        return forStatement();
    }
    if (match({ TOKEN_IF })) {
        return ifStatement();
    }
    if (match({ TOKEN_KONSOLE })) {
        return konsoleStatement();
    }
    if (match({ TOKEN_RETURN })) {
        return returnStatement();
    }
    if (match({ TOKEN_WHILE })) {
        return whileStatement();
    }
    if (match({ TOKEN_BREAK })) {
        return breakStatement();
    }
    if (match({ TOKEN_LEFT_BRACE })) {
        return std::make_unique<Block>(block());
    }
    return expressionStatement();
}

UniqueStmtPtr Parser::letDeclaration() {
    Token name = consume(TOKEN_IDENTIFIER, "Expected variable identifier.");
    std::optional<UniqueExprPtr> initializer = std::nullopt;
    if (match( {TOKEN_EQUAL} )) {
        initializer = expression();
    }

    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    UniqueStmtPtr var = std::make_unique<Let>(name, std::move(initializer));
    return var;
}

std::unique_ptr<Function> Parser::function(const std::string& kind) {
    Token name = consume(TOKEN_IDENTIFIER, "Expected " + kind + " name.");
    consume(TOKEN_LEFT_PAREN, "Expected '(' after " + kind + " name.");

    std::vector<Token> parameters;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            if (parameters.size() >= 255) {
                error(peek(), "Cannot have more than 255 parameters.");
            }
            parameters.push_back(consume(TOKEN_IDENTIFIER, "Expected parameter name."));
        } while (match({ TOKEN_COMMA }));
    }

    consume(TOKEN_RIGHT_PAREN, "Expected ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expected '{' before " + kind + " body.");

    std::vector<UniqueStmtPtr> body = block();
    return std::make_unique<Function>(name, parameters, std::move(body));
}

UniqueStmtPtr Parser::clazzDeclaration() {
    Token name = consume(TOKEN_IDENTIFIER, "Expected class name.");

    std::optional<std::unique_ptr<Variable>> superclass = std::nullopt;
    if (match( {TOKEN_LESS} )) {
        consume(TOKEN_IDENTIFIER, "Expected superclass name.");
        superclass = std::make_unique<Variable>(previous());
    }

    consume(TOKEN_LEFT_BRACE, "Expected '{' before class body.");

    std::vector<std::unique_ptr<Function>> methods;
    std::vector<std::unique_ptr<Function>> staticMethods;
    while (!check(TOKEN_RIGHT_BRACE) && !isAtEnd()) {
        if (match({TOKEN_STATIC})) {
            staticMethods.push_back(function("static method"));
        } else {
            methods.push_back(function("method"));
        }
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' after class body.");

    return std::make_unique<Class>(name, std::move(superclass), std::move(methods), std::move(staticMethods));
}

UniqueStmtPtr Parser::declaration() {
    try
    {
        if (match({TokenType::TOKEN_CLAZZ}))
            return clazzDeclaration();
        if (match({TokenType::TOKEN_LET}))
            return letDeclaration();
        if (match({TokenType::TOKEN_FUNCT}))
            return function("function");

        return statement();
    }
    catch (ParseError&)
    {
        //Report the exception but don't let it bubble up and stop the program. Instead, synchronize the parser and keep parsing.
        hadParseError = true;
        synchronize();
        return nullptr;
    }
}