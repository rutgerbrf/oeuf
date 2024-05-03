// vim:set sw=2 ts=2 sts et:
//
// Copyright 2024 Rutger Broekhoff. Licensed under the EUPL.

#include <tmi8/kv1_lexer.hpp>

Kv1Lexer::Kv1Lexer(std::string_view input)
  : input(input), slice(input)
{}

// Does not eat newline character.
void Kv1Lexer::eatRestOfLine() {
  size_t end = slice.size();
  for (size_t i = 0; i < slice.size(); i++) {
    if (slice[i] == '\r' || slice[i] == '\n') {
      end = i;
      break;
    }
  }
  slice = slice.substr(end);
}

void Kv1Lexer::lexOptionalHeader() {
  if (slice.starts_with('[')) eatRestOfLine();
}

void Kv1Lexer::lexOptionalComment() {
  if (slice.starts_with(';')) eatRestOfLine();
}

inline bool Kv1Lexer::isWhitespace(int c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

void Kv1Lexer::readQuotedColumn() {
  Kv1Token token{ .type = KV1_TOKEN_CELL };

  if (slice.size() == 0 || slice[0] != '"') {
    errors.push_back("(internal error) readQuotedColumn: slice[0] != '\"'");
    return;
  }
  slice = slice.substr(1);
  while (true) {
    size_t quote = slice.find('"');
    if (quote == std::string_view::npos) {
      errors.push_back("readQuotedColumn: no matching closing quote found");
      return;
    }
    if (quote+1 == slice.size() || slice[quote + 1] != '"') {
      token.data.append(slice.substr(0, quote));
      break;
    }
    token.data.append(slice.substr(0, quote + 1));
    slice = slice.substr(quote + 2);
  }

  size_t end = slice.size();
  for (size_t i = 0; i < slice.size(); i++) {
    if (slice[i] == '|' || slice[i] == '\r' || slice[i] == '\n') {
      end = i;
      break;
    }
    if (!isWhitespace(slice[i])) {
      errors.push_back("readQuotedColumn: encountered non-whitespace character after closing quote");
      return;
    }
  }
  if (end != std::string_view::npos) slice = slice.substr(end);
  else slice = slice.substr(slice.size());

  tokens.push_back(std::move(token));
}

void Kv1Lexer::readUnquotedColumn() {
  size_t end = slice.size();
  size_t content_end = 0;
  for (size_t i = 0; i < slice.size(); i++) {
    if (slice[i] == '|' || slice[i] == '\r' || slice[i] == '\n') {
      end = i;
      break;
    } else if (!isWhitespace(slice[i])) {
      content_end = i + 1;
    }
  }
  tokens.emplace_back(KV1_TOKEN_CELL, std::string(slice.substr(0, content_end)));
  if (end != std::string_view::npos) slice = slice.substr(end);
  else slice = slice.substr(slice.size());
}

void Kv1Lexer::lexRow() {
  size_t cols = 0;
  while (slice.size() > 0 && slice[0] != '\r' && slice[0] != '\n') {
    if (slice[0] == '"') readQuotedColumn();
    else readUnquotedColumn();
    if (!errors.empty()) return;
    cols++;
    if (slice.size() != 0) {
      if (slice[0] == '|') {
        slice = slice.substr(1);
        // A newline/eof right after pipe? That means an empty field at the end
        // of the record, we also want to emit that as a token.
        if (slice.size() == 0 || slice[0] == '\r' || slice[0] == '\n') {
          tokens.push_back({ .type = KV1_TOKEN_CELL });
        }
      } else if (slice[0] == '\r') {
        if (slice.size() > 1 && slice[1] == '\n') slice = slice.substr(2);
        else slice = slice.substr(1);
        break;
      } else if (slice[0] == '\n') {
        slice = slice.substr(1);
        break;
      } else {
        errors.push_back("lexRow: expected CR, LF or |");
        return;
      }
    }
  }
  tokens.push_back({ .type = KV1_TOKEN_ROW_END });
}

// Returns true when a line ending was consumed.
bool Kv1Lexer::eatWhitespace() {
  for (size_t i = 0; i < slice.size(); i++) {
    if (slice[i] == '\r') {
      slice = slice.substr(i + 1);
      if (slice.size() > 1 && slice[i + 1] == '\n')
        slice = slice.substr(i + 2);
      return true;
    }
    if (slice[i] == '\n') {
      slice = slice.substr(i + 1);
      return true;
    }
    
    if (slice[i] != ' ' && slice[i] != '\f' && slice[i] != '\t' && slice[i] != '\v') {
      slice = slice.substr(i);
      return false;
    }
  }
  return false;
}

void Kv1Lexer::lex() {
  lexOptionalHeader();
  eatWhitespace();

  while (errors.empty() && !slice.empty()) {
    lexOptionalComment();
    bool newline = eatWhitespace();
    if (newline) continue;
    // We are now either (1) at the end of the file or (2) at the start of some column data
    if (errors.empty()) lexRow();
  }
}
