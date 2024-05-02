// vim:set sw=2 ts=2 sts et:

#ifndef OEUF_LIBTMI8_KV1_LEXER_HPP
#define OEUF_LIBTMI8_KV1_LEXER_HPP

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <variant>

enum Kv1TokenType {
  KV1_TOKEN_CELL,
  KV1_TOKEN_ROW_END,
};
struct Kv1Token { Kv1TokenType type; std::string data; };

struct Kv1Lexer {
  std::vector<std::string> errors;
  std::vector<Kv1Token> tokens;
  
  explicit Kv1Lexer(std::string_view input);

  void lex();

 private:
  // Does not eat newline character.
  void eatRestOfLine();
  void lexOptionalHeader();
  void lexOptionalComment();
  
  static bool isWhitespace(int c);

  void readQuotedColumn();
  void readUnquotedColumn();
  void lexRow();
  // Returns true when a line ending was consumed.
  bool eatWhitespace();
  
  std::string_view input;
  std::string_view slice;
  std::string colbuf;
};

#endif // OEUF_LIBTMI8_KV1_LEXER_HPP
