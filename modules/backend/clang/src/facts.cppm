// mcxx.backend.clang partition :facts: MC3 v0 facts of a unit -- what its own code declares and does.
module;

#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/AST/DynamicRecursiveASTVisitor.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/StmtCXX.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/RawCommentList.h>
#include <clang/AST/Type.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/AllDiagnostics.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/Module.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/Stack.h>
#include <clang/Driver/CreateASTUnitFromArgs.h>
#include <clang/Driver/CreateInvocationFromArgs.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/Utils.h>
#include <clang/Index/IndexDataConsumer.h>
#include <clang/Index/IndexSymbol.h>
#include <clang/Index/IndexingAction.h>
#include <clang/Index/IndexingOptions.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Sema/CodeCompleteConsumer.h>
#include <clang/Sema/Sema.h>
#include <clang/UnifiedSymbolResolution/USRGeneration.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/thread.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

module mcxx.backend.clang:facts;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import :support;
import :unit;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;
namespace fact = msa::fact;

namespace {

// Qualified names without inline namespaces ("std::vector", "nlohmann::basic_json"): what a rule
// and a configuration write, whatever ABI namespace a library versions itself with.
std::string plain_name(const cl::NamedDecl* d) {
    cl::PrintingPolicy policy { d->getASTContext().getLangOpts() };
    policy.SuppressInlineNamespace = llvm::to_underlying(cl::PrintingPolicy::SuppressInlineNamespaceMode::All);
    std::string out;
    llvm::raw_string_ostream os { out };
    d->printQualifiedName(os, policy);
    return out;
}

std::string type_text(const cl::ASTContext& ctx, cl::QualType type) {
    if (type.isNull()) return {};
    return type.getAsString(printing_policy(ctx));
}

// The namespace code in `dc` belongs to ("" = global).
std::string namespace_of(const cl::DeclContext* dc) {
    for (; dc != nullptr; dc = dc->getParent()) {
        if (const auto* ns = llvm::dyn_cast<cl::NamespaceDecl>(dc)) return plain_name(ns);
    }
    return {};
}

const cl::ClassTemplateSpecializationDecl* specialization_of(cl::QualType type) {
    if (type.isNull()) return nullptr;
    const auto* record = type.getNonReferenceType().getCanonicalType()->getAsCXXRecordDecl();
    return llvm::dyn_cast_or_null<cl::ClassTemplateSpecializationDecl>(record);
}

void collect_templates(const cl::ASTContext& ctx, cl::QualType type, std::vector<std::string>& out, int depth = 0) {
    if (type.isNull() || depth > 8) return;
    type = type.getNonReferenceType();
    if (const auto* pointer = type->getAs<cl::PointerType>()) return collect_templates(ctx, pointer->getPointeeType(), out, depth + 1);
    if (const auto* array = ctx.getAsArrayType(type)) return collect_templates(ctx, array->getElementType(), out, depth + 1);
    auto add = [&](std::string name) {
        if (std::ranges::find(out, name) == out.end()) out.push_back(std::move(name));
    };
    if (const auto* spec = specialization_of(type)) {
        add(plain_name(spec->getSpecializedTemplate()));
        for (const auto& arg : spec->getTemplateArgs().asArray())
            if (arg.getKind() == cl::TemplateArgument::Type) collect_templates(ctx, arg.getAsType(), out, depth + 1);
        return;
    }
    // In a template: the specialization is still dependent, its name is what it is.
    if (const auto* tst = type->getAs<cl::TemplateSpecializationType>()) {
        if (const auto* td = tst->getTemplateName().getAsTemplateDecl()) add(plain_name(td));
        for (const auto& arg : tst->template_arguments())
            if (arg.getKind() == cl::TemplateArgument::Type) collect_templates(ctx, arg.getAsType(), out, depth + 1);
    }
}

bool is_initializer_list(cl::QualType type) {
    const auto* record = type.getNonReferenceType().getCanonicalType()->getAsCXXRecordDecl();
    return record != nullptr && record->getName() == "initializer_list" && record->isInStdNamespace();
}

class Collector final : public cl::DynamicRecursiveASTVisitor {
public:
    Collector(cl::ASTContext& ctx, fact::Facts& facts) : ctx_ { ctx }, sm_ { ctx.getSourceManager() }, lo_ { ctx.getLangOpts() }, facts_ { facts } {
        main_ = sm_.getMainFileID();
    }

    bool TraverseDecl(cl::Decl* d) override {
        if (d == nullptr) return true;
        if (!llvm::isa<cl::TranslationUnitDecl>(d)) {
            // Only the file's own code: what it imports or includes is its own file's business.
            if (d->isImplicit() || !in_main(d->getLocation())) return true;
        }
        const cl::Decl* outer { current_ };
        current_ = d;
        const bool go_on { cl::DynamicRecursiveASTVisitor::TraverseDecl(d) };
        current_ = outer;
        return go_on;
    }

    bool VisitDecl(cl::Decl* d) override {
        for (const auto* attr : d->specific_attrs<cl::AnnotateAttr>()) {
            const llvm::StringRef text { attr->getAnnotation() };
            if (!text.starts_with("mcpp::allow|")) continue;
            fact::Suppression s;
            s.range = range_of(d->getSourceRange()).value_or(Range {});
            s.container = namespace_of(d->getDeclContext());
            s.entity = usr_of(d);
            if (const auto* named = llvm::dyn_cast<cl::NamedDecl>(d)) s.declaration = plain_name(named);
            const auto [ids, reason] = text.drop_front(std::string_view { "mcpp::allow|" }.size()).split('|');
            llvm::SmallVector<llvm::StringRef, 4> parts;
            ids.split(parts, ',', -1, false);
            for (auto p : parts) s.ids.emplace_back(p.trim().str());
            s.reason = reason.str();
            facts_.suppressions.push_back(std::move(s));
        }
        return true;
    }

    bool VisitNamedDecl(cl::NamedDecl* d) override {
        if (!llvm::isa<cl::VarDecl, cl::FieldDecl, cl::FunctionDecl, cl::TypedefNameDecl, cl::RecordDecl, cl::EnumDecl, cl::NamespaceDecl>(d)) return true;
        if (const auto* record = llvm::dyn_cast<cl::RecordDecl>(d); record && !record->isThisDeclarationADefinition()) return true;
        fact::Declaration decl;
        decl.range = range_of(d->getSourceRange()).value_or(Range {});
        decl.name = range_of(d->getLocation()).value_or(decl.range);
        decl.container = namespace_of(d->getDeclContext());
        decl.entity = usr_of(d);
        decl.qualified_name = plain_name(d);
        decl.kind = kind_of(d);
        decl.exported = d->isInExportDeclContext();
        cl::QualType type;
        if (const auto* parm = llvm::dyn_cast<cl::ParmVarDecl>(d)) type = parm->getOriginalType();
        else if (const auto* value = llvm::dyn_cast<cl::ValueDecl>(d)) type = value->getType();
        else if (const auto* alias = llvm::dyn_cast<cl::TypedefNameDecl>(d)) type = alias->getUnderlyingType();
        if (const auto* fn = llvm::dyn_cast<cl::FunctionDecl>(d)) collect_templates(ctx_, fn->getReturnType(), decl.templates);
        else collect_templates(ctx_, type, decl.templates);
        if (!type.isNull() && !llvm::isa<cl::FunctionDecl>(d)) {
            decl.type = type_text(ctx_, type);
            decl.c_array = type.getNonReferenceType()->isArrayType();
        }
        if (const auto* record = llvm::dyn_cast<cl::RecordDecl>(d)) decl.is_union = record->isUnion();
        facts_.declarations.push_back(std::move(decl));
        return true;
    }

    bool VisitVarDecl(cl::VarDecl* v) override {
        if (llvm::isa<cl::ParmVarDecl>(v) || !v->hasInit()) return true;
        fact::InitForm form { fact::InitForm::copy };
        switch (v->getInitStyle()) {
        case cl::VarDecl::CInit: form = fact::InitForm::copy; break;
        case cl::VarDecl::CallInit: form = fact::InitForm::direct; break;
        case cl::VarDecl::ListInit: form = fact::InitForm::direct_list; break;
        case cl::VarDecl::ParenListInit: form = fact::InitForm::direct; break;
        }
        add_initialization(v, v->getType(), v->getInit(), form, false);
        return true;
    }

    bool VisitFieldDecl(cl::FieldDecl* f) override {
        if (!f->hasInClassInitializer() || f->getInClassInitializer() == nullptr) return true;
        const fact::InitForm form { f->getInClassInitStyle() == cl::ICIS_ListInit ? fact::InitForm::direct_list : fact::InitForm::copy };
        add_initialization(f, f->getType(), f->getInClassInitializer(), form, true);
        return true;
    }

    bool VisitExplicitCastExpr(cl::ExplicitCastExpr* e) override {
        if (!in_main(e->getBeginLoc())) return true;
        fact::Cast cast;
        cast.range = range_of(e->getSourceRange()).value_or(Range {});
        cast.container = container();
        cast.from = type_text(ctx_, e->getSubExpr()->getType());
        cast.to = type_text(ctx_, e->getType());
        if (llvm::isa<cl::CXXStaticCastExpr>(e)) cast.kind = fact::CastKind::static_cast_;
        else if (llvm::isa<cl::CXXDynamicCastExpr>(e)) cast.kind = fact::CastKind::dynamic_cast_;
        else if (llvm::isa<cl::CXXConstCastExpr>(e)) cast.kind = fact::CastKind::const_cast_;
        else if (llvm::isa<cl::CXXReinterpretCastExpr>(e)) cast.kind = fact::CastKind::reinterpret_cast_;
        else if (llvm::isa<cl::CStyleCastExpr>(e)) cast.kind = fact::CastKind::c_style;
        else if (llvm::isa<cl::CXXFunctionalCastExpr>(e)) cast.kind = fact::CastKind::functional;
        else return true;   // __builtin_bit_cast and friends
        cast.reinterprets = cast.kind == fact::CastKind::reinterpret_cast_ || reinterprets(e);
        facts_.casts.push_back(std::move(cast));
        return true;
    }

    bool VisitCXXNewExpr(cl::CXXNewExpr* e) override {
        if (!in_main(e->getBeginLoc())) return true;
        facts_.allocations.push_back({ { range_of(e->getSourceRange()).value_or(Range {}), container() }, false, e->isArray(),
                                       type_text(ctx_, e->getAllocatedType()) });
        return true;
    }

    bool VisitCXXDeleteExpr(cl::CXXDeleteExpr* e) override {
        if (!in_main(e->getBeginLoc())) return true;
        facts_.allocations.push_back({ { range_of(e->getSourceRange()).value_or(Range {}), container() }, true, e->isArrayForm(),
                                       type_text(ctx_, e->getDestroyedType()) });
        return true;
    }

    bool VisitBinaryOperator(cl::BinaryOperator* e) override {
        if (!in_main(e->getOperatorLoc())) return true;
        const auto op = e->getOpcode();
        if (op != cl::BO_Add && op != cl::BO_Sub && op != cl::BO_AddAssign && op != cl::BO_SubAssign) return true;
        const cl::Expr* pointer { e->getLHS()->getType()->isPointerType() ? e->getLHS() : e->getRHS()->getType()->isPointerType() ? e->getRHS() : nullptr };
        if (pointer == nullptr) return true;
        facts_.pointer_arithmetic.push_back({ { range_of(e->getSourceRange()).value_or(Range {}), container() }, e->getOpcodeStr().str(),
                                              type_text(ctx_, pointer->getType()) });
        return true;
    }

    bool VisitUnaryOperator(cl::UnaryOperator* e) override {
        if (!in_main(e->getOperatorLoc()) || !e->isIncrementDecrementOp() || !e->getSubExpr()->getType()->isPointerType()) return true;
        facts_.pointer_arithmetic.push_back({ { range_of(e->getSourceRange()).value_or(Range {}), container() },
                                              e->isIncrementOp() ? "++" : "--", type_text(ctx_, e->getSubExpr()->getType()) });
        return true;
    }

    bool VisitArraySubscriptExpr(cl::ArraySubscriptExpr* e) override {
        if (!in_main(e->getBeginLoc())) return true;
        const cl::Expr* base { e->getBase()->IgnoreParenImpCasts() };
        if (!base->getType()->isPointerType()) return true;   // a C array's own subscript is the c-array feature's
        facts_.pointer_arithmetic.push_back({ { range_of(e->getSourceRange()).value_or(Range {}), container() }, "[]", type_text(ctx_, base->getType()) });
        return true;
    }

    bool VisitGotoStmt(cl::GotoStmt* s) override {
        if (!in_main(s->getGotoLoc())) return true;
        facts_.gotos.push_back({ { range_of(s->getSourceRange()).value_or(Range {}), container() }, s->getLabel()->getName().str() });
        return true;
    }

    bool VisitIndirectGotoStmt(cl::IndirectGotoStmt* s) override {
        if (!in_main(s->getGotoLoc())) return true;
        facts_.gotos.push_back({ { range_of(s->getSourceRange()).value_or(Range {}), container() }, "*" });
        return true;
    }

private:
    cl::ASTContext& ctx_;
    const cl::SourceManager& sm_;
    const cl::LangOptions& lo_;
    fact::Facts& facts_;
    cl::FileID main_;
    const cl::Decl* current_ { nullptr };

    bool in_main(cl::SourceLocation loc) const { return loc.isValid() && sm_.getFileID(sm_.getFileLoc(loc)) == main_; }

    std::optional<Range> range_of(cl::SourceRange range) const { return source_range(sm_, lo_, range, main_); }
    std::optional<Range> range_of(cl::SourceLocation loc) const {
        if (auto token = token_location(sm_, lo_, loc); token && token->path == path_of(sm_, main_)) return token->range;
        return std::nullopt;
    }

    std::string container() const {
        return current_ == nullptr ? std::string {}
                                   : namespace_of(llvm::isa<cl::DeclContext>(current_) ? llvm::cast<cl::DeclContext>(current_) : current_->getDeclContext());
    }

    // A C-style or functional cast that does what only reinterpret_cast may.
    static bool reinterprets(const cl::ExplicitCastExpr* e) {
        switch (e->getCastKind()) {
        case cl::CK_IntegralToPointer:
        case cl::CK_PointerToIntegral:
        case cl::CK_LValueBitCast:
        case cl::CK_ReinterpretMemberPointer:
            return true;
        case cl::CK_BitCast: {
            // Between object pointers of unrelated types; through void* is a static_cast's.
            const auto from = e->getSubExpr()->getType();
            const auto to = e->getType();
            if (!from->isPointerType() || !to->isPointerType()) return true;
            return !from->getPointeeType()->isVoidType() && !to->getPointeeType()->isVoidType();
        }
        default:
            return false;
        }
    }

    void add_initialization(const cl::DeclaratorDecl* d, cl::QualType type, const cl::Expr* init, fact::InitForm form, bool member) {
        fact::Initialization fact;
        fact.range = range_of(d->getSourceRange()).value_or(Range {});
        fact.name = range_of(d->getLocation()).value_or(fact.range);
        fact.container = namespace_of(d->getDeclContext());
        fact.entity = usr_of(d);
        fact.variable = plain_name(d);
        fact.type = type_text(ctx_, type);
        if (const auto* spec = specialization_of(type)) fact.type_template = plain_name(spec->getSpecializedTemplate());
        fact.member_default = member;
        const cl::Expr* value { init->IgnoreImplicit() };
        // `T x = { ... }`: copy-list-initialization.
        if (form == fact::InitForm::copy) {
            if (llvm::isa<cl::InitListExpr>(value)) form = fact::InitForm::copy_list;
            else if (const auto* construct = llvm::dyn_cast<cl::CXXConstructExpr>(value); construct && construct->isListInitialization())
                form = fact::InitForm::copy_list;
        }
        fact.form = form;
        const cl::InitListExpr* list { nullptr };
        if (const auto* construct = llvm::dyn_cast<cl::CXXConstructExpr>(value)) {
            const cl::CXXConstructorDecl* ctor { construct->getConstructor() };
            std::string params;
            for (const auto* p : ctor->parameters()) params += (params.empty() ? "" : ", ") + type_text(ctx_, p->getType());
            fact.constructor = std::format("{}({})", plain_name(ctor), params);
            fact.initializer_list_constructor = ctor->getNumParams() > 0 && is_initializer_list(ctor->getParamDecl(0)->getType());
            if (construct->getNumArgs() > 0) {
                const cl::Expr* first { construct->getArg(0)->IgnoreImplicit() };
                if (const auto* std_list = llvm::dyn_cast<cl::CXXStdInitializerListExpr>(first)) first = std_list->getSubExpr()->IgnoreImplicit();
                list = llvm::dyn_cast<cl::InitListExpr>(first);
            }
            if (list == nullptr && construct->isListInitialization()) fact.elements = construct->getNumArgs();
        } else {
            list = llvm::dyn_cast<cl::InitListExpr>(value);
        }
        if (list != nullptr) {
            fact.elements = list->getNumInits();
            if (fact.elements == 1) fact.element_braced = starts_with_brace(list->getInit(0));
        }
        facts_.initializations.push_back(std::move(fact));
    }

    // Whether an element was written as a braced list: its first token is `{`.
    bool starts_with_brace(const cl::Expr* e) const {
        const cl::SourceLocation loc { sm_.getSpellingLoc(e->getBeginLoc()) };
        if (loc.isInvalid()) return false;
        bool invalid { false };
        const char* data { sm_.getCharacterData(loc, &invalid) };
        return !invalid && data != nullptr && *data == '{';
    }
};

} // namespace

fact::Facts facts_of(cl::ASTContext& ctx, const cl::Preprocessor* pp) {
    fact::Facts facts;
    Collector collector { ctx, facts };
    collector.TraverseDecl(ctx.getTranslationUnitDecl());
    if (pp != nullptr) {
        const auto& sm = ctx.getSourceManager();
        const cl::FileID main { sm.getMainFileID() };
        for (const auto& [ii, state] : pp->macros(false)) {
            for (const cl::MacroDirective* md { pp->getLocalMacroDirectiveHistory(ii) }; md != nullptr; md = md->getPrevious()) {
                const auto* def = llvm::dyn_cast<cl::DefMacroDirective>(md);
                if (def == nullptr) continue;
                const cl::SourceLocation loc { def->getLocation() };
                if (loc.isInvalid() || sm.getFileID(sm.getFileLoc(loc)) != main) continue;
                if (auto token = token_location(sm, ctx.getLangOpts(), loc))
                    facts.macros.push_back({ { token->range, {} }, ii->getName().str() });
            }
        }
    }
    base::trace::count("facts.units");
    return facts;
}

// UnitImpl::facts: computed once, on the Workspace's Clang stack.
const msa::fact::Facts& UnitImpl::facts() const {
    return *pool_->run([&]() -> const msa::fact::Facts* {
        std::lock_guard lock { mutex_ };
        if (!facts_) {
            facts_.emplace();
            if (ast_) *facts_ = facts_of(ast_->getASTContext(), &ast_->getPreprocessor());
        }
        return &*facts_;
    });
}

} // namespace mcxx::clang_backend
