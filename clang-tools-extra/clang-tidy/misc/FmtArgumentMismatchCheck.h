//===--- FmtArgumentMismatchCheck.h - clang-tidy ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_FMTARGUMENTMISMATCHCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_FMTARGUMENTMISMATCHCHECK_H

#include "../ClangTidyCheck.h"
#include "llvm/ADT/ArrayRef.h"

#include <unordered_map>

namespace clang {
namespace tidy {
namespace misc {

namespace fmt {
  class ReplacementField;
}

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc-fmt-argument-mismatch.html
class FmtArgumentMismatchCheck : public ClangTidyCheck {
public:
  FmtArgumentMismatchCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override; 

private:
  void matchReplacements(ArrayRef<fmt::ReplacementField> Fields, const CallExpr* FmtCall, std::size_t PosOfFormatStr);
};

} // namespace misc
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_FMTARGUMENTMISMATCHCHECK_H
