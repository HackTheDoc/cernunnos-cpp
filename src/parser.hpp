#pragma once

#include <cassert>
#include <variant>

#include "tokenization.hpp"
#include "arena.hpp"

enum VarType {
    NONE,
    INT
};

std::string to_string(VarType t)
{
    switch (t)
    {
    case VarType::NONE:
        return "";
    case VarType::INT:
        return "int";
    default:
        return "auto";
    }
}

VarType to_variable_type(TokenType t)
{
    switch (t)
    {
    case TokenType::INTEGER_LITERAL:
        return VarType::INT;
    default:
        return VarType::NONE;
    }
}

namespace Node
{
    struct Expr;

    struct TermIntegerLiteral
    {
        Token int_lit;
    };

    struct TermIdentifier
    {
        Token ident;
    };

    struct TermParen
    {
        Expr *expr;
    };

    struct Term
    {
        std::variant<TermIntegerLiteral *, TermIdentifier *, TermParen *> var;
        VarType type{VarType::NONE};
    };

    struct BinExprAdd
    {
        Expr *lside;
        Expr *rside;
    };

    struct BinExprSub
    {
        Expr *lside;
        Expr *rside;
    };

    struct BinExprMulti
    {
        Expr *lside;
        Expr *rside;
    };

    struct BinExprDiv
    {
        Expr *lside;
        Expr *rside;
    };

    struct BinExpr
    {
        std::variant<BinExprAdd *, BinExprSub *, BinExprMulti *, BinExprDiv *> var;
    };

    struct Expr
    {
        std::variant<Term *, BinExpr *> var;
        VarType type{VarType::NONE};
    };

    struct Stmt;

    struct Scope
    {
        std::vector<Stmt *> stmts;
    };

    struct StmtReturn
    {
        Expr *expr;
    };

    struct StmtVar
    {
        Token identifier;
        Expr *expr;
    };

    struct StmtVarAssign
    {
        Token ident;
        Expr *expr;
    };

    struct IfPred;

    struct IfPredElif
    {
        Expr *expr;
        Scope *scope;
        std::optional<IfPred *> pred;
    };

    struct IfPredElse
    {
        Scope *scope;
    };

    struct IfPred
    {
        std::variant<IfPredElif *, IfPredElse *> var;
    };

    struct StmtIf
    {
        Expr *expr;
        Scope *scope;
        std::optional<IfPred *> pred;
    };

    struct Stmt
    {
        std::variant<StmtReturn *, StmtVar *, StmtVarAssign *, Scope *, StmtIf *> var;
    };

    struct Prog
    {
        std::vector<Stmt *> stmts;
    };
}

class Parser
{
private:
    const std::vector<Token> _tokens;

    size_t _index = 0;

    ArenaAllocator _allocator;

    std::optional<Token> peek(const int offset = 0) const
    {
        if (_index + offset >= _tokens.size())
            return {};
        return _tokens.at(_index + offset);
    }

    bool peek_type(TokenType type, int offset = 0) const
    {
        return peek(offset).has_value() && peek(offset).value().type == type;
    }

    Token consume()
    {
        return _tokens[_index++];
    }

    Token try_consume_err(TokenType type)
    {
        if (peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        else
        {
            exit_with(to_string(type));
            return {}; // unreachable
        }
    }

    std::optional<Token> try_consume(TokenType type)
    {
        if (peek().has_value() && peek().value().type == type)
        {
            return consume();
        }
        else
        {
            return {};
        }
    }

    void exit_with(const std::string &err_msg, std::string template_msg = "missing")
    {
        std::cerr << "[Parse Error] " << template_msg << " " << err_msg << " on line ";

        if (peek().has_value())
            std::cerr << peek().value().line;
        else
            std::cerr << peek(-1).value().line;

        std::cerr << std::endl;

        exit(EXIT_FAILURE);
    }

public:
    Parser(std::vector<Token> tokens) : _tokens(std::move(tokens)), _allocator(1024 * 1024 * 4) {} // 4mb

    std::optional<Node::Term *> parse_term()
    {

        if (auto int_lit = try_consume(TokenType::INTEGER_LITERAL))
        {
            auto term_int_lit = _allocator.emplace<Node::TermIntegerLiteral>(int_lit.value());
            auto term = _allocator.emplace<Node::Term>(term_int_lit, VarType::INT);
            return term;
        }

        if (auto ident = try_consume(TokenType::IDENTIFIER))
        {
            auto expr_ident = _allocator.emplace<Node::TermIdentifier>(ident.value());
            auto term = _allocator.emplace<Node::Term>(expr_ident, to_variable_type(ident.value().type));
            return term;
        }

        if (auto open_paren = try_consume(TokenType::LEFT_PARENTHESIS))
        {
            auto expr = parse_expr();
            if (!expr.has_value())
                exit_with("expression");

            try_consume_err(TokenType::RIGHT_PARENTHESIS);

            auto term_paren = _allocator.emplace<Node::TermParen>(expr.value());
            auto term = _allocator.emplace<Node::Term>(term_paren, expr.value()->type);
            return term;
        }

        return {};
    }

    std::optional<Node::Expr *> parse_expr(int min_prec = 0)
    {
        std::optional<Node::Term *> lterm = parse_term();
        if (!lterm.has_value())
            return {};

        auto expr = _allocator.emplace<Node::Expr>(lterm.value(), lterm.value()->type);

        while (true)
        {
            std::optional<Token> curr_tok = peek();
            std::optional<int> prec;

            if (curr_tok.has_value())
            {
                prec = bin_prec(curr_tok.value().type);
                if (!prec.has_value() || prec.value() < min_prec)
                {
                    break;
                }
            }
            else
                break;

            Token op = consume();

            int next_min_prec = prec.value() + 1;
            auto expr_rside = parse_expr(next_min_prec);
            if (!expr_rside.has_value())
                exit_with("expression");
            
            if (expr->type != VarType::NONE &&
                expr_rside.value()->type != VarType::NONE &&
                expr->type != expr_rside.value()->type)
            {
                exit_with(to_string(expr->type)+to_string(op.type)+to_string(expr_rside.value()->type),
                            "wrong operation :");
            }

            auto bin_expr = _allocator.emplace<Node::BinExpr>();
            auto expr_lside = _allocator.emplace<Node::Expr>(expr->var);

            if (op.type == TokenType::PLUS)
            {
                auto add = _allocator.emplace<Node::BinExprAdd>(expr_lside, expr_rside.value());
                bin_expr->var = add;
            }
            else if (op.type == TokenType::MINUS)
            {
                /// TODO: make sure you cannot sub strings
                auto sub = _allocator.emplace<Node::BinExprSub>(expr_lside, expr_rside.value());
                bin_expr->var = sub;
            }
            else if (op.type == TokenType::STAR)
            {
                auto multi = _allocator.emplace<Node::BinExprMulti>(expr_lside, expr_rside.value());
                bin_expr->var = multi;
            }
            else if (op.type == TokenType::SLASH)
            {
                /// TODO: make sure you cannot divide strings
                auto div = _allocator.emplace<Node::BinExprDiv>(expr_lside, expr_rside.value());
                bin_expr->var = div;
            }
            else
                assert(false); // unreachable

            expr->var = bin_expr;
        }

        return expr;
    }

    std::optional<Node::Scope *> parse_scope()
    {
        if (!try_consume(TokenType::LEFT_CURLY_BACKET).has_value())
            return {};

        auto scope = _allocator.emplace<Node::Scope>();
        int l = -1;
        while (auto stmt = parse_stmt())
        {
            scope->stmts.push_back(stmt.value());
        }

        try_consume_err(TokenType::RIGHT_CURLY_BRACKET);

        return scope;
    }

    std::optional<Node::IfPred *> parse_if_pred()
    {
        if (auto t = try_consume(TokenType::ELIF))
        {
            try_consume_err(TokenType::LEFT_PARENTHESIS);
            auto elif_pred = _allocator.alloc<Node::IfPredElif>();
            if (const auto expr = parse_expr())
                elif_pred->expr = expr.value();
            else
                exit_with("expression");

            try_consume_err(TokenType::RIGHT_PARENTHESIS);

            if (const auto scope = parse_scope())
                elif_pred->scope = scope.value();
            else
                exit_with("scope");

            elif_pred->pred = parse_if_pred();

            return _allocator.emplace<Node::IfPred>(elif_pred);
        }

        if (try_consume(TokenType::ELSE))
        {
            auto else_pred = _allocator.alloc<Node::IfPredElse>();
            if (const auto scope = parse_scope())
                else_pred->scope = scope.value();
            else
                exit_with("scope");

            return _allocator.emplace<Node::IfPred>(else_pred);
        }

        return {};
    }

    std::optional<Node::Stmt *> parse_stmt()
    {
        if (!peek().has_value())
            return {};

        if (peek_type(TokenType::RETURN))
        {
            consume();
            Node::StmtReturn *ret = _allocator.emplace<Node::StmtReturn>();

            if (auto ne = parse_expr())
                ret->expr = ne.value();
            else
                exit_with("return value");

            Node::Stmt *stmt = _allocator.emplace<Node::Stmt>(ret);

            return stmt;
        }

        if (peek_type(TokenType::VAR) && peek_type(TokenType::IDENTIFIER, 1) && peek_type(TokenType::EQUAL, 2))
        {
            consume();
            Node::StmtVar *var = _allocator.emplace<Node::StmtVar>();
            var->identifier = consume();
            consume();
            if (auto e = parse_expr())
            {
                var->expr = e.value();
            }
            else
            {
                exit_with("expression");
            }

            Node::Stmt *stmt = _allocator.emplace<Node::Stmt>(var);
            return stmt;
        }

        if (peek_type(TokenType::IDENTIFIER) && peek_type(TokenType::EQUAL, 1))
        {
            auto var_assign = _allocator.alloc<Node::StmtVarAssign>();
            var_assign->ident = consume();

            consume(); // = token

            if (const auto expr = parse_expr())
            {
                var_assign->expr = expr.value();
            }
            else
                exit_with("expression");

            return _allocator.emplace<Node::Stmt>(var_assign);
        }

        if (peek_type(TokenType::LEFT_CURLY_BACKET))
        {
            if (auto scope = parse_scope())
            {
                auto stmt = _allocator.emplace<Node::Stmt>(scope.value());
                return stmt;
            }
            else
                exit_with("scope");
        }

        if (auto tif = try_consume(TokenType::IF))
        {
            try_consume_err(TokenType::LEFT_PARENTHESIS);

            auto stmt_if = _allocator.emplace<Node::StmtIf>();

            if (auto expr = parse_expr())
            {
                stmt_if->expr = expr.value();
            }
            else
                exit_with("expression");

            try_consume_err(TokenType::RIGHT_PARENTHESIS);

            if (auto scope = parse_scope())
            {
                stmt_if->scope = scope.value();
            }
            else
                exit_with("scope");

            stmt_if->pred = parse_if_pred();

            auto stmt = _allocator.emplace<Node::Stmt>(stmt_if);
            return stmt;
        }

        return {};
    }

    std::optional<Node::Prog> parse_prog()
    {
        Node::Prog prog;

        while (peek().has_value())
        {
            if (std::optional<Node::Stmt *> stmt = parse_stmt())
            {
                prog.stmts.push_back(stmt.value());
            }
            else
            {
                exit_with("statement");
            }
        }

        return prog;
    };
};
