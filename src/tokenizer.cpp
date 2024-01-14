#include "tokenizer.h"

std::string to_string(const TokenType type)
{
    switch (type)
    {
    case TokenType::RETURN:
        return "return value";
    case TokenType::VAR:
        return "var";
    case TokenType::FUNC:
        return "func";
    case TokenType::IDENTIFIER:
        return "identifier";
    case TokenType::TYPE_INT:
        return "int";
    case TokenType::TYPE_CHAR:
        return "char";
    case TokenType::INTEGER_LITERAL:
        return "integer literal";
    case TokenType::CHAR_LITERAL:
        return "char literal";
    case TokenType::IF:
        return "if";
    case TokenType::ELIF:
        return "elif";
    case TokenType::ELSE:
        return "else";
    case TokenType::EQUAL:
        return "=";
    case TokenType::COLON:
        return ":";
    case TokenType::COMMA:
        return ",";
    case TokenType::LEFT_PARENTHESIS:
        return "(";
    case TokenType::RIGHT_PARENTHESIS:
        return ")";
    case TokenType::LEFT_CURLY_BACKET:
        return "{";
    case TokenType::RIGHT_CURLY_BRACKET:
        return "}";
    case TokenType::PLUS:
        return "+";
    case TokenType::MINUS:
        return "-";
    case TokenType::STAR:
        return "*";
    case TokenType::SLASH:
        return "/";
    default:
        return "";
    }
}

std::optional<int> bin_prec(TokenType type)
{
    switch (type)
    {
    case TokenType::PLUS:
    case TokenType::MINUS:
        return 0;
    case TokenType::STAR:
    case TokenType::SLASH:
        return 1;
    default:
        return {};
    }
}

Tokenizer::Tokenizer(const std::string &src)
    : _src(std::move(src))
{
}

std::optional<char> Tokenizer::peek(const size_t offset) const
{
    if (_index + offset >= _src.length())
        return {};
    return _src.at(_index + offset);
}

char Tokenizer::consume()
{
    return _src[_index++];
}

std::vector<Token> Tokenizer::tokenize()
{
    std::vector<Token> tokens;
    std::string buf;
    int line_count = 1;

    while (peek().has_value())
    {
        if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '/')
        {
            while (peek().has_value() && peek().value() != '\n')
                consume();
        }
        else if (peek().value() == '/' && peek(1).has_value() && peek(1).value() == '*')
        {
            consume();
            consume();

            while (peek().has_value())
            {
                if (peek().value() == '*' && peek(1).has_value() && peek(1).value() == '/')
                    break;
                consume();
            }

            if (peek().has_value())
                consume();
            if (peek().has_value())
                consume();
        }
        else if (std::isalpha(peek().value()))
        {
            buf.push_back(consume());
            while (peek().has_value() &&
                   (std::isalnum(peek().value()) || peek().value() == '_'))
            {
                buf.push_back(consume());
            }

            // TYPES
            if (buf == "int")
                tokens.push_back({.type = TokenType::TYPE_INT, .line = line_count});
            else if (buf == "char")
                tokens.push_back({.type = TokenType::TYPE_CHAR, .line = line_count});

            // KEYWORDS
            else if (buf == "var")
                tokens.push_back({.type = TokenType::VAR, .line = line_count});
            else if (buf == "func")
                tokens.push_back({.type = TokenType::FUNC, .line = line_count});
            else if (buf == "return")
                tokens.push_back({.type = TokenType::RETURN, .line = line_count});
            else if (buf == "if")
                tokens.push_back({.type = TokenType::IF, .line = line_count});
            else if (buf == "elif")
                tokens.push_back({.type = TokenType::ELIF, .line = line_count});
            else if (buf == "else")
                tokens.push_back({.type = TokenType::ELSE, .line = line_count});
            else
                tokens.push_back({.type = TokenType::IDENTIFIER, .line = line_count, .val = buf});

            buf.clear();
        }
        else if (std::isdigit(peek().value()))
        {
            buf.push_back(consume());
            while (peek().has_value() && std::isdigit(peek().value()))
            {
                buf.push_back(consume());
            }
            tokens.push_back({.type = TokenType::INTEGER_LITERAL,
                              .line = line_count,
                              .val = buf});
            buf.clear();
        }
        else if (peek().value() == '=')
        {
            consume();
            tokens.push_back({.type = TokenType::EQUAL, .line = line_count});
        }
        else if (peek().value() == ':')
        {
            consume();
            tokens.push_back({.type = TokenType::COLON, .line = line_count});
        }
        else if (peek().value() == ',')
        {
            consume();
            tokens.push_back({.type = TokenType::COMMA, .line = line_count});
        }
        else if (peek().value() == '(')
        {
            consume();
            tokens.push_back({.type = TokenType::LEFT_PARENTHESIS, .line = line_count});
        }
        else if (peek().value() == ')')
        {
            consume();
            tokens.push_back({.type = TokenType::RIGHT_PARENTHESIS, .line = line_count});
        }
        else if (peek().value() == '{')
        {
            consume();
            tokens.push_back({.type = TokenType::LEFT_CURLY_BACKET, .line = line_count});
        }
        else if (peek().value() == '}')
        {
            consume();
            tokens.push_back({.type = TokenType::RIGHT_CURLY_BRACKET, .line = line_count});
        }
        else if (peek().value() == '+')
        {
            consume();
            tokens.push_back({.type = TokenType::PLUS, .line = line_count});
        }
        else if (peek().value() == '-')
        {
            consume();
            tokens.push_back({.type = TokenType::MINUS, .line = line_count});
        }
        else if (peek().value() == '*')
        {
            consume();
            tokens.push_back({.type = TokenType::STAR, .line = line_count});
        }
        else if (peek().value() == '/')
        {
            consume();
            tokens.push_back({.type = TokenType::SLASH, .line = line_count});
        }
        else if (peek().value() == '\'')
        {
            consume(); // '

            if (peek().has_value() && isalnum(peek().value()))
            {
                std::string c;
                c += consume();
                tokens.push_back({.type = TokenType::CHAR_LITERAL, .line = line_count, .val = c});

                if (!peek().has_value() || peek().value() != '\'')
                {
                    std::cerr << "[Error] expected `'` on line " << line_count << std::endl;
                    exit(EXIT_FAILURE);
                }

                consume(); // '
            }
            else
            {
                std::cerr << "[Error] expected a valid char on line " << line_count << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else if (peek().value() == '\n')
        {
            line_count++;
            consume();
        }
        else if (std::isspace(peek().value()))
        {
            consume();
        }
        else
        {
            std::cerr << "[Error] invalid token `" << peek().value() << "` on line " << line_count << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    _index = 0;

    return tokens;
}
