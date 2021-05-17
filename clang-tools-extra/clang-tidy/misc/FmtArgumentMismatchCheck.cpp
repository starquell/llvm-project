//===--- FmtArgumentMismatchCheck.cpp - clang-tidy ------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "FmtArgumentMismatchCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallSet.h"

#include <algorithm>

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {
namespace misc {

namespace fmt {

    struct ParseError : public llvm::ErrorInfo<ParseError> {
        // Required field for all ErrorInfo derivatives.
        static char ID;

        ParseError(std::string ErrorMsg, std::size_t Pos = 0)
            : ErrorMsg(std::move(ErrorMsg)),
              Pos(Pos) {}

        void log(llvm::raw_ostream &OS) const override {
            OS << "parse error: " << ErrorMsg;
        }

        std::error_code convertToErrorCode() const override {
            return llvm::inconvertibleErrorCode();
        }

        std::string ErrorMsg;
        std::size_t Pos;
    };

    char ParseError::ID;

    /// @brief Represents positional, index or named argument in format string
    struct ArgId {

        enum class IdType {
            Index,
            Named,
            None
        };
        union Id {
            std::size_t Index;
            StringRef Named;
        };
        
        Id Id{};
        IdType Type = IdType::None;

        static Expected<ArgId> parse(StringRef Str) {
            if (Str.empty()) {
                return ArgId{};
            }
            if (std::isdigit(Str.front())) {
                ArgId Id;
                Str.getAsInteger(10, Id.Id.Index);
                Id.Type = IdType::Index;
                return Id;
            }
            else if (std::isalpha(Str.front())) {
                ArgId Id{};
                Id.Id.Named = Str;
                Id.Type = IdType::Named;
                return Id;
            }
            else return llvm::make_error<ParseError>("invalid format string: expected argument id or ':'");
        }
    };

    /// We omit format specifiers for simplification now
    struct FormatSpecifiers {

        static Expected<FormatSpecifiers> parse(StringRef str) {
            return FormatSpecifiers{};
        }
    };

    struct ReplacementField {
        ArgId Id;
        FormatSpecifiers Specs;

        static Expected<ReplacementField> parse(StringRef Str) {
            if (Str.empty()) {
                return ReplacementField{};
            }
            ReplacementField Field;
            const auto Splitted = Str.split(':');
            if (auto Parsed = ArgId::parse(Splitted.first)) {
                Field.Id = *Parsed;
            }
            else {
                return Parsed.takeError();
            }
            if (auto Parsed = FormatSpecifiers::parse(Splitted.second)) {
                Field.Specs = *Parsed;
            }
            else {
                return llvm::handleErrors(Parsed.takeError(), [&] (const ParseError& err) {
                    return llvm::make_error<ParseError>(err.ErrorMsg, Str.find(':') + 1 + err.Pos);
                });
            }
            return Field;
        }
    };

    Expected<std::vector<ReplacementField>> extractFields(StringRef Str)  {
        std::vector<ReplacementField> Fields;
        auto StrBegin = Str.begin();
        bool InStr = false;

        auto It = Str.begin();
        while (It != Str.end()) {
            if (!InStr && *It == '{') {
                if (It + 1 != Str.end() && *(It + 1) == '{') {
                    It += 2;
                }
                else {
                    InStr = true;
                    StrBegin = It;
                }
            }
            else if (InStr && *It == '}') {
                if (It + 1 != Str.end() && *(It + 1) == '}') {
                    It += 2;
                }
                else {
                    InStr = false;
                    if (auto Parsed = ReplacementField::parse(StringRef(StrBegin + 1, It - (StrBegin + 1)))) {
                        Fields.emplace_back(*Parsed);
                    }
                    else {
                        return llvm::handleErrors(Parsed.takeError(), [&] (const ParseError& err) {
                            return llvm::make_error<ParseError>(err.ErrorMsg, StrBegin - Str.begin() + 1 + err.Pos);
                        });
                    }
                }
            }
            ++It;
        }
        return Fields;
    }
}

void FmtArgumentMismatchCheck::registerMatchers(MatchFinder *Finder) {

    /// Array of pairs that contain function to check and position of format string argument
    constexpr std::array<std::pair<StringRef, std::size_t>, 6> FuncsAndFormatPositions = {
        std::make_pair("::fmt::format", 0),
        std::make_pair("::fmt::format_to", 1),
        std::make_pair("::fmt::format_to_n", 2),
        std::make_pair("::fmt::formatted_size", 0),
        std::make_pair("::fmt::print", 0),
        std::make_pair("::fmt::print", 1)
    };

    for (const auto& Elem: FuncsAndFormatPositions) {
        Finder->addMatcher(callExpr(callee(functionDecl(hasName(Elem.first))),
                                    hasArgument(Elem.second, stringLiteral().bind("format string"))
                                    ).bind("format"), this);
    }
    
}

void FmtArgumentMismatchCheck::check(const MatchFinder::MatchResult &Result) {

  if (auto* MatchedCall = Result.Nodes.getNodeAs<CallExpr>("format")) {
      auto FormatString = Result.Nodes.getNodeAs<StringLiteral>("format string");

      if (auto Parsed = fmt::extractFields(FormatString->getString())) {

          auto PosOfFormatStr = [&] () -> int {
              /// Finding position of format string argument by comparison every arg with format string
              const Expr* const* Args = MatchedCall->getArgs();
              for (std::size_t i = 0; i < MatchedCall->getNumArgs(); ++i) {
                  const auto AsStrLiteral = dyn_cast<const StringLiteral>(*(Args + i));
                  if (AsStrLiteral && (FormatString->getString() == AsStrLiteral->getString())) {
                      return i;
                  }
              }
              return -1;
          }();
          
          if (PosOfFormatStr != -1) {
              matchReplacements(*Parsed, MatchedCall, PosOfFormatStr);
          }
      } 
      else {
          if (auto Err = llvm::handleErrors(Parsed.takeError(),
              [&] (const fmt::ParseError& Err) {
                  diag(FormatString->getBeginLoc(), "%0 at character %1") << Err.ErrorMsg << static_cast<unsigned int>(Err.Pos);
              })) {
                  diag(MatchedCall->getBeginLoc(), "incorrect format string");
              }
      }
  }
}

void FmtArgumentMismatchCheck::matchReplacements(ArrayRef<fmt::ReplacementField> Fields, const CallExpr* FmtCall, std::size_t PosOfFormatStr) {
    if (Fields.empty()) {
        return;
    }
    const bool UsesPositionalArgs = std::any_of(Fields.begin(), Fields.end(), [&] (const auto& field) {
                                                return field.Id.Type == fmt::ArgId::IdType::Index;
                                    });
    const bool UsesDefaultArgs = std::any_of(Fields.begin(), Fields.end(), [&] (const auto& field) {
                                             return field.Id.Type == fmt::ArgId::IdType::None;
                                 });
    if (UsesPositionalArgs && UsesDefaultArgs) {
        diag(FmtCall->getBeginLoc(), "combining of manual and automatic argument indexing in format string restricted");
        return;
    }
    const std::size_t ArgsNum = FmtCall->getNumArgs() - 1 - PosOfFormatStr;
    
    if (UsesPositionalArgs) {

        SmallVector<bool> UsedPositions(ArgsNum, false);
        
        for (const auto& Field: Fields) {
            if (Field.Id.Id.Index >= ArgsNum) {
                diag(FmtCall->getBeginLoc(), "no argument for position %0 in format string") << static_cast<unsigned int>(Field.Id.Id.Index);
                continue;
            }
            UsedPositions[Field.Id.Id.Index] = true;
        }
        for (std::size_t I = 0; I < UsedPositions.size(); ++I) {
            if (!UsedPositions[I]) {
                diag(FmtCall->getArg(PosOfFormatStr + 1 + I)->getBeginLoc(), "unused argument");
            }
        }
        return;
    }
    if (UsesDefaultArgs) {
        if (Fields.size() != ArgsNum) {
            diag(FmtCall->getBeginLoc(), "argument count mismatch (expected %0, got %1)")
                << static_cast<unsigned>(Fields.size())
                << static_cast<unsigned>(ArgsNum);
        }
    }
}


} // namespace misc
} // namespace tidy
} // namespace clang
