// mcxx.backend.clang implementation unit: the definitions of :unit (a parsed unit and its queries).
module;

#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
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

module mcxx.backend.clang;

import mcxx.msa;
import mcxx.graph;
import mcxx.base;
import mcxx.plugin;
import :support;
import :unit;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;

// ============================================================================================
// 3. units
// ============================================================================================
std::string path_of(const cl::SourceManager& sm, cl::FileID fid) {
    if (const auto ref = sm.getFileEntryRefForID(fid)) return normalize_path(ref->getName());
    return {};
}

Position position_of(const cl::SourceManager& sm, cl::SourceLocation loc) {
    const auto [fid, offset] = sm.getDecomposedLoc(loc);
    bool invalid { false };
    const unsigned line { sm.getLineNumber(fid, offset, &invalid) };
    const unsigned column { sm.getColumnNumber(fid, offset, &invalid) };
    if (invalid || line == 0 || column == 0) return {};
    return { line - 1, column - 1 };
}

// The name token at `loc`, as a location in the file it is written in.
std::optional<Location> token_location(const cl::SourceManager& sm, const cl::LangOptions& lo, cl::SourceLocation loc) {
    if (loc.isInvalid()) return std::nullopt;
    const cl::SourceLocation file { sm.getFileLoc(loc) };
    const std::string path { path_of(sm, sm.getFileID(file)) };
    if (path.empty()) return std::nullopt;
    const Position begin { position_of(sm, file) };
    const unsigned length { cl::Lexer::MeasureTokenLength(sm.getSpellingLoc(loc), sm, lo) };
    return Location { path, { begin, { begin.line, begin.column + std::max(1u, length) } } };
}

std::optional<Range> source_range(const cl::SourceManager& sm, const cl::LangOptions& lo, cl::SourceRange range, cl::FileID main) {
    const cl::SourceLocation b { sm.getFileLoc(range.getBegin()) };
    const cl::SourceLocation e { sm.getFileLoc(range.getEnd()) };
    if (b.isInvalid() || e.isInvalid() || sm.getFileID(b) != main || sm.getFileID(e) != main) return std::nullopt;
    const Position end { position_of(sm, e) };
    return Range { position_of(sm, b), { end.line, end.column + cl::Lexer::MeasureTokenLength(e, sm, lo) } };
}

std::string usr_of(const cl::Decl* d) {
    llvm::SmallString<128> buffer;
    if (!d || cl::index::generateUSRForDecl(d, buffer)) return {};
    return std::string { buffer.str() };
}

msa::Kind kind_of(const cl::Decl* d) {
    if (!d) return msa::Kind::unknown;
    if (llvm::isa<cl::ConceptDecl>(d)) return msa::Kind::concept_;
    if (llvm::isa<cl::TypeAliasTemplateDecl>(d)) return msa::Kind::type_alias;
    if (const auto* t = llvm::dyn_cast<cl::TemplateDecl>(d)) {
        if (const auto* templated = t->getTemplatedDecl()) d = templated;
    }
    if (llvm::isa<cl::NamespaceDecl>(d)) return msa::Kind::namespace_;
    if (llvm::isa<cl::NamespaceAliasDecl>(d)) return msa::Kind::namespace_alias;
    if (llvm::isa<cl::UsingDecl>(d)) return msa::Kind::using_declaration;
    if (const auto* r = llvm::dyn_cast<cl::RecordDecl>(d)) return r->isUnion() ? msa::Kind::union_ : r->isStruct() ? msa::Kind::struct_ : msa::Kind::class_;
    if (llvm::isa<cl::EnumDecl>(d)) return msa::Kind::enum_;
    if (llvm::isa<cl::EnumConstantDecl>(d)) return msa::Kind::enumerator;
    if (llvm::isa<cl::TypedefNameDecl>(d)) return msa::Kind::type_alias;
    if (llvm::isa<cl::CXXConstructorDecl>(d)) return msa::Kind::constructor;
    if (llvm::isa<cl::CXXDestructorDecl>(d)) return msa::Kind::destructor;
    if (llvm::isa<cl::CXXConversionDecl>(d)) return msa::Kind::conversion;
    if (llvm::isa<cl::CXXMethodDecl>(d)) return msa::Kind::method;
    if (llvm::isa<cl::FunctionDecl>(d)) return msa::Kind::function;
    if (llvm::isa<cl::FieldDecl>(d)) return msa::Kind::field;
    if (llvm::isa<cl::ParmVarDecl>(d)) return msa::Kind::parameter;
    if (llvm::isa<cl::VarDecl>(d) || llvm::isa<cl::BindingDecl>(d)) return msa::Kind::variable;
    if (llvm::isa<cl::TemplateTypeParmDecl>(d) || llvm::isa<cl::NonTypeTemplateParmDecl>(d) || llvm::isa<cl::TemplateTemplateParmDecl>(d))
        return msa::Kind::template_parameter;
    if (llvm::isa<cl::LabelDecl>(d)) return msa::Kind::label;
    return msa::Kind::unknown;
}

std::uint32_t roles_of(cl::index::SymbolRoleSet roles, llvm::ArrayRef<cl::index::SymbolRelation> relations) {
    using R = cl::index::SymbolRole;
    std::uint32_t out { 0 };
    if (roles & static_cast<unsigned>(R::Declaration)) out |= msa::role::declaration;
    if (roles & static_cast<unsigned>(R::Definition)) out |= msa::role::definition;
    if (roles & static_cast<unsigned>(R::Reference)) out |= msa::role::reference;
    if (roles & static_cast<unsigned>(R::Read)) out |= msa::role::read;
    if (roles & static_cast<unsigned>(R::Write)) out |= msa::role::write;
    if (roles & static_cast<unsigned>(R::Call)) out |= msa::role::call;
    if (roles & static_cast<unsigned>(R::Implicit)) out |= msa::role::implicit;
    for (const auto& relation : relations)
        if (relation.Roles & static_cast<unsigned>(R::RelationOverrideOf)) out |= msa::role::overrides;
    return out;
}

cl::PrintingPolicy printing_policy(const cl::ASTContext& ctx) {
    cl::PrintingPolicy p { ctx.getLangOpts() };
    p.TerseOutput = true;
    p.PolishForDeclaration = true;
    p.SuppressUnwrittenScope = true;
    p.SuppressTemplateArgsInCXXConstructors = true;
    return p;
}


cl::index::IndexingOptions indexing_options(bool locals) {
    cl::index::IndexingOptions o;
    o.SystemSymbolFilter = cl::index::IndexingOptions::SystemSymbolFilterKind::All;
    o.IndexFunctionLocals = locals;
    o.IndexParametersInDeclarations = locals;
    o.IndexTemplateParameters = locals;
    o.IndexImplicitInstantiation = false;
    o.IndexMacros = false;
    o.IndexMacrosInPreprocessor = false;
    return o;
}

// A diagnostic's stable name, as clangd gives it: Clang's own name less its kind ("err_", "warn_",
// "ext_"), e.g. "expected_semi_after_module_or_import".
std::string diagnostic_code(unsigned id) {
    std::string_view name;
    switch (id) {
#define DIAG(ENUM, CLASS, DEFAULT_SEVERITY, DESC, GROUP, SFINAE, NOWERROR, SHOWINSYSHEADER, SHOWINSYSMACRO, DEFERRABLE, CATEGORY, STABLE_ID, \
             LEGACY_STABLE_IDS)                                                                                                     \
    case ::clang::diag::ENUM: name = #ENUM; break;
#include <clang/Basic/AllDiagnosticKinds.inc>
#undef DIAG
    default: break;
    }
    for (const std::string_view kind : { "err_", "warn_", "ext_" }) {
        if (name.starts_with(kind)) {
            name.remove_prefix(kind.size());
            break;
        }
    }
    return std::string { name };
}

namespace {

std::string one_line(std::string text) {
    std::string out;
    bool space { false };
    for (char c : text) {
        if (c == '\n' || c == '\r' || c == '\t' || c == ' ') {
            space = !out.empty();
            continue;
        }
        if (space) out += ' ';
        space = false;
        out += c;
    }
    if (const auto brace = out.find(" {"); brace != std::string::npos && out.ends_with("}")) out.erase(brace);
    return out;
}

const cl::Decl* definition_of(const cl::Decl* d) {
    if (const auto* t = llvm::dyn_cast<cl::TemplateDecl>(d)) {
        if (const auto* templated = t->getTemplatedDecl()) d = templated;
    }
    if (const auto* f = llvm::dyn_cast<cl::FunctionDecl>(d)) return f->getDefinition();
    if (const auto* t = llvm::dyn_cast<cl::TagDecl>(d)) return t->getDefinition();
    if (const auto* v = llvm::dyn_cast<cl::VarDecl>(d)) return v->getDefinition();
    return nullptr;
}

std::optional<Location> name_location(const cl::Decl* d) {
    if (!d) return std::nullopt;
    const auto& ctx = d->getASTContext();
    return token_location(ctx.getSourceManager(), ctx.getLangOpts(), d->getLocation());
}

const cl::NamedDecl* type_decl_of(cl::QualType type) {
    if (type.isNull()) return nullptr;
    type = type.getNonReferenceType();
    while (true) {
        if (const auto* p = type->getAs<cl::PointerType>()) type = p->getPointeeType();
        else if (const auto* a = type->getAsArrayTypeUnsafe()) type = a->getElementType();
        else break;
    }
    if (const auto* typedefType = type->getAs<cl::TypedefType>()) return typedefType->getDecl();
    if (const auto* tag = type->getAsTagDecl()) return tag;
    if (const auto* specialization = type->getAs<cl::TemplateSpecializationType>()) {
        if (const auto* decl = specialization->getTemplateName().getAsTemplateDecl()) return decl;
    }
    return nullptr;
}

msa::Entity build_entity(const cl::Decl* d) {
    msa::Entity e;
    const auto& ctx = d->getASTContext();
    const auto& sm = ctx.getSourceManager();
    const cl::PrintingPolicy policy { printing_policy(ctx) };
    e.id = usr_of(d);
    e.kind = kind_of(d);
    if (const auto* nd = llvm::dyn_cast<cl::NamedDecl>(d)) {
        e.name = nd->getNameAsString();
        e.qualified_name = plain_name(nd);   // as MC3 names it: no inline namespace (nlohmann's ABI one either)
    }
    {
        std::string text;
        llvm::raw_string_ostream os { text };
        d->print(os, policy);
        e.signature = one_line(std::move(text));
    }
    const cl::Decl* subject { d };
    if (const auto* t = llvm::dyn_cast<cl::TemplateDecl>(d)) {
        if (const auto* templated = t->getTemplatedDecl()) subject = templated;
    }
    cl::QualType valueType;
    if (const auto* v = llvm::dyn_cast<cl::ValueDecl>(subject)) {
        valueType = v->getType();
        e.type = valueType.getAsString(policy);
    }
    if (const auto* t = llvm::dyn_cast<cl::TypedefNameDecl>(subject)) {
        valueType = t->getUnderlyingType();
        e.type = valueType.getAsString(policy);
    }
    if (const auto* f = subject->getAsFunction()) {
        e.return_type = f->getReturnType().getAsString(policy);
        valueType = f->getReturnType();
        for (const auto* p : f->parameters()) {
            msa::Parameter parameter { p->getNameAsString(), p->getType().getAsString(policy), {} };
            if (p->hasDefaultArg() && !p->hasUninstantiatedDefaultArg() && !p->hasUnparsedDefaultArg()) {
                if (const auto* init = p->getDefaultArg()) {
                    std::string text;
                    llvm::raw_string_ostream os { text };
                    init->printPretty(os, nullptr, policy);
                    parameter.default_value = text;
                }
            }
            e.parameters.push_back(std::move(parameter));
        }
    }
    if (const auto* c = llvm::dyn_cast<cl::EnumConstantDecl>(subject)) {
        llvm::SmallString<32> digits;
        c->getInitVal().toString(digits, 10);
        e.value = std::string { digits.str() };
    }
    // A constant's value, as the unit evaluates it (a -D the command gives shows here).
    const auto* declared = llvm::dyn_cast<cl::VarDecl>(subject);
    if (const auto* v = declared ? declared->getInitializingDeclaration() : nullptr;
        v && !v->getType()->isDependentType() && !v->isTemplated() && (v->isConstexpr() || v->getType().isConstQualified())) {
        if (const auto* value = v->evaluateValue(); value && (value->isInt() || value->isFloat())) {
            e.value = value->getAsString(ctx, v->getType());
        }
    }
    if (const auto* comment = ctx.getRawCommentForAnyRedecl(d)) e.documentation = comment->getFormattedText(sm, ctx.getDiagnostics());
    if (const auto* m = d->getOwningModule(); m && m->isNamedModule()) e.module = m->getPrimaryModuleInterfaceName().str();
    for (const cl::DeclContext* dc { d->getDeclContext() }; dc; dc = dc->getParent()) {
        if (const auto* named = llvm::dyn_cast<cl::NamedDecl>(dc)) {
            e.container = named->getQualifiedNameAsString();
            break;
        }
    }
    switch (d->getAccess()) {
    case cl::AS_public: e.access = "public"; break;
    case cl::AS_protected: e.access = "protected"; break;
    case cl::AS_private: e.access = "private"; break;
    default: break;
    }
    e.declaration = name_location(d->getCanonicalDecl());
    if (const auto* def = definition_of(d)) e.definition = name_location(def);
    else if (const auto* nd = llvm::dyn_cast<cl::NamespaceDecl>(d)) e.definition = name_location(nd);
    if (const auto* typeDecl = type_decl_of(valueType); typeDecl && typeDecl != d) {
        e.type_entity = usr_of(typeDecl);
        e.type_location = name_location(typeDecl);
    }
    return e;
}

msa::Severity severity_of(cl::DiagnosticsEngine::Level level) {
    switch (level) {
    case cl::DiagnosticsEngine::Error:
    case cl::DiagnosticsEngine::Fatal: return msa::Severity::error;
    case cl::DiagnosticsEngine::Warning: return msa::Severity::warning;
    case cl::DiagnosticsEngine::Remark: return msa::Severity::information;
    default: return msa::Severity::hint;
    }
}

std::vector<msa::Diagnostic> diagnostics_of(cl::ASTUnit& ast) {
    std::vector<msa::Diagnostic> out;
    const auto& sm = ast.getSourceManager();
    const auto& lo = ast.getLangOpts();
    const cl::FileID main { sm.getMainFileID() };
    for (auto it = ast.stored_diag_begin(); it != ast.stored_diag_end(); ++it) {
        const cl::StoredDiagnostic& d { *it };
        if (d.getLevel() == cl::DiagnosticsEngine::Ignored) continue;
        const cl::FullSourceLoc loc { d.getLocation() };
        std::optional<Location> where;
        if (loc.isValid()) {
            if (auto token = token_location(sm, lo, loc)) {
                where = std::move(token);
                for (const auto& range : d.getRanges()) {
                    const cl::SourceRange r { range.getAsRange() };
                    if (auto full = source_range(sm, lo, r, main); full && where->path == path_of(sm, main) && full->contains(where->range.begin)) {
                        where->range = *full;
                        break;
                    }
                }
            }
        }
        if (d.getLevel() == cl::DiagnosticsEngine::Note) {
            if (!out.empty() && where) out.back().notes.push_back({ *where, std::string { d.getMessage() } });
            continue;
        }
        msa::Diagnostic diagnostic;
        diagnostic.severity = severity_of(d.getLevel());
        diagnostic.message = std::string { d.getMessage() };
        diagnostic.code = diagnostic_code(d.getID());
        // MC++'s own diagnostics (feature gates) name their feature in brackets: that is their code.
        if (diagnostic.code.empty()) {
            for (std::size_t open { diagnostic.message.find('[') }; open != std::string::npos; open = diagnostic.message.find('[', open + 1)) {
                const std::size_t close { diagnostic.message.find(']', open) };
                if (close == std::string::npos) break;
                const std::string_view id { std::string_view { diagnostic.message }.substr(open + 1, close - open - 1) };
                if (plugin::find_feature(id) != nullptr || id == "mcxx-config" || id == "mcxx-plugin" || id == "mcxx-filter") {
                    diagnostic.code = std::string { id };
                    break;
                }
            }
        }
        diagnostic.category = cl::DiagnosticIDs::getCategoryNameFromID(cl::DiagnosticIDs::getCategoryNumberForDiag(d.getID())).str();
        if (where && where->path == path_of(sm, main)) {
            diagnostic.range = where->range;
        } else if (where) {
            // In a file the unit includes: report it at the top, naming the place.
            diagnostic.message = std::format("In included file {}:{}: {}", where->path, where->range.begin.line + 1, diagnostic.message);
            diagnostic.notes.push_back({ *where, "the error is here" });
        }
        out.push_back(std::move(diagnostic));
    }
    return out;
}

void collect_symbols(const cl::DeclContext* dc, const cl::SourceManager& sm, const cl::LangOptions& lo, cl::FileID main,
                     const cl::PrintingPolicy& policy, std::vector<msa::Symbol>& out) {
    for (const cl::Decl* d : dc->decls()) {
        if (const auto* x = llvm::dyn_cast<cl::ExportDecl>(d)) {
            collect_symbols(x, sm, lo, main, policy, out);
            continue;
        }
        if (const auto* l = llvm::dyn_cast<cl::LinkageSpecDecl>(d)) {
            collect_symbols(l, sm, lo, main, policy, out);
            continue;
        }
        const auto* nd = llvm::dyn_cast<cl::NamedDecl>(d);
        if (!nd || nd->isImplicit() || nd->getDeclName().isEmpty()) continue;
        const cl::SourceLocation nameLoc { sm.getFileLoc(nd->getLocation()) };
        if (nameLoc.isInvalid() || sm.getFileID(nameLoc) != main) continue;
        const msa::Kind kind { kind_of(nd) };
        if (kind == msa::Kind::unknown || kind == msa::Kind::parameter || kind == msa::Kind::template_parameter) continue;
        msa::Symbol symbol;
        symbol.name = nd->getNameAsString();
        symbol.kind = kind;
        const auto name = token_location(sm, lo, nd->getLocation());
        if (!name) continue;
        symbol.selection = name->range;
        symbol.range = source_range(sm, lo, nd->getSourceRange(), main).value_or(name->range);
        const cl::Decl* subject { nd };
        if (const auto* t = llvm::dyn_cast<cl::TemplateDecl>(nd)) {
            if (const auto* templated = t->getTemplatedDecl()) subject = templated;
        }
        if (const auto* f = subject->getAsFunction()) symbol.detail = cl::QualType { f->getType() }.getAsString(policy);
        else if (const auto* v = llvm::dyn_cast<cl::ValueDecl>(subject)) symbol.detail = v->getType().getAsString(policy);
        if (const auto* inner = llvm::dyn_cast<cl::DeclContext>(subject);
            inner && (llvm::isa<cl::NamespaceDecl>(subject) || llvm::isa<cl::TagDecl>(subject)))
            collect_symbols(inner, sm, lo, main, policy, symbol.children);
        out.push_back(std::move(symbol));
    }
}

// Why the driver rejects a command line, in its own words; empty when it accepts it.
std::string command_rejection(const std::vector<std::string>& args) {
    std::vector<const char*> argv;
    for (const auto& a : args) argv.push_back(a.c_str());
    auto vfs = llvm::vfs::getRealFileSystem();
    cl::DiagnosticOptions diagOptions;
    CollectingConsumer consumer;
    auto diags = cl::CompilerInstance::createDiagnostics(*vfs, diagOptions, &consumer, false);
    cl::CreateInvocationOptions options;
    options.Diags = diags;
    options.VFS = vfs;
    if (cl::createInvocation(argv, options) && consumer.errors.empty()) return {};
    return consumer.errors.empty() ? std::string { "the driver gave no reason" } : consumer.errors.front();
}

} // namespace

UnitImpl::UnitImpl(std::unique_ptr<cl::ASTUnit> ast, std::string path, std::string text, std::int64_t version, std::string module,
         std::vector<msa::Diagnostic> extra, ClangPool* pool)
    : pool_ { pool }, ast_ { std::move(ast) }, path_ { std::move(path) }, text_ { std::move(text) }, version_ { version },
      module_ { std::move(module) } {
    diagnostics_ = std::move(extra);
    if (!ast_) return;
    const std::size_t failed { diagnostics_.size() };
    for (auto& d : diagnostics_of(*ast_)) {
        // "module 'm' not found" where m failed to build says less than the failure already told.
        const bool told { d.code == "module_not_found" &&
                          std::ranges::any_of(std::span { diagnostics_ }.first(failed), [&](const msa::Diagnostic& f) { return f.range.begin == d.range.begin; }) };
        if (!told) diagnostics_.push_back(std::move(d));
    }
    auto& sm = ast_->getSourceManager();
    const auto& lo = ast_->getLangOpts();
    const cl::FileID main { sm.getMainFileID() };
    OccurrenceConsumer consumer { [&](const cl::Decl* d, cl::index::SymbolRoleSet roles, llvm::ArrayRef<cl::index::SymbolRelation> relations,
                                      cl::SourceLocation loc) {
        const cl::SourceLocation file { sm.getFileLoc(loc) };
        if (file.isInvalid() || sm.getFileID(file) != main) return;
        const auto token = token_location(sm, lo, loc);
        if (!token) return;
        const cl::Decl* canonical { d->getCanonicalDecl() };
        std::string id { usr_of(canonical) };
        if (id.empty()) return;
        decls_.try_emplace(id, canonical);
        const auto* nd = llvm::dyn_cast<cl::NamedDecl>(d);
        occurrences_.push_back({ token->range, id, nd ? nd->getNameAsString() : std::string {}, kind_of(d), roles_of(roles, relations) });
    } };
    cl::index::indexASTUnit(*ast_, consumer, indexing_options(true));
    std::ranges::sort(occurrences_, [](const msa::Occurrence& a, const msa::Occurrence& b) {
        return std::tie(a.range.begin, a.range.end) < std::tie(b.range.begin, b.range.end);
    });
}

std::vector<msa::Symbol> UnitImpl::symbols() const {
    return pool_->run([&] { return symbols_(); });
}

std::vector<msa::Symbol> UnitImpl::symbols_() const {
    std::lock_guard lock { mutex_ };
    std::vector<msa::Symbol> out;
    if (!ast_) return out;
    auto& ctx = ast_->getASTContext();
    collect_symbols(ctx.getTranslationUnitDecl(), ast_->getSourceManager(), ast_->getLangOpts(), ast_->getSourceManager().getMainFileID(),
                    printing_policy(ctx), out);
    return out;
}

std::optional<msa::Entity> UnitImpl::entity_at(Position at) const {
    const msa::Occurrence* best { nullptr };
    for (const auto& o : occurrences_) {
        if (at < o.range.begin) break;
        if (o.range.contains(at) && (!best || best->range.begin < o.range.begin)) best = &o;
    }
    if (!best) return std::nullopt;
    return entity(best->entity);
}

std::optional<msa::Entity> UnitImpl::entity(std::string_view id) const {
    return pool_->run([&] { return entity_(id); });
}

std::optional<msa::Entity> UnitImpl::entity_(std::string_view id) const {
    std::lock_guard lock { mutex_ };
    const auto it = decls_.find(std::string { id });
    if (it == decls_.end()) return std::nullopt;
    return build_entity(it->second);
}

std::vector<Location> UnitImpl::overriders(std::string_view id) const {
    return pool_->run([&] { return overriders_(id); });
}

std::vector<Location> UnitImpl::overriders_(std::string_view id) const {
    std::lock_guard lock { mutex_ };
    std::vector<Location> out;
    const auto it = decls_.find(std::string { id });
    if (it == decls_.end()) return out;
    const auto* method = llvm::dyn_cast<cl::CXXMethodDecl>(it->second);
    if (!method) return out;
    for (const auto& [otherId, decl] : decls_) {
        const auto* other = llvm::dyn_cast<cl::CXXMethodDecl>(decl);
        if (!other) continue;
        for (const auto* overridden : other->overridden_methods()) {
            if (overridden->getCanonicalDecl() == method->getCanonicalDecl()) {
                if (auto loc = name_location(other)) out.push_back(*loc);
            }
        }
    }
    return out;
}

std::unique_ptr<cl::ASTUnit> parse_ast(const ParseRequest& request, std::string* rejected) {
    std::vector<std::string> args { normalize(request.command) };
    for (const auto& extra : backend_arguments(request.resource_directory)) args.push_back(extra);
    for (const auto& [name, pcm] : request.modules) args.push_back("-fmodule-file=" + name + "=" + pcm);
    if (request.module_unit) args.insert(args.end(), { "-x", "c++-module" });
    args.insert(args.end(), { "-fsyntax-only", request.path });
    std::vector<const char*> argv;
    for (const auto& a : args) argv.push_back(a.c_str());
    auto options = std::make_shared<cl::DiagnosticOptions>();
    auto vfs = llvm::vfs::getRealFileSystem();
    auto diags = cl::CompilerInstance::createDiagnostics(*vfs, *options);
    std::vector<cl::ASTUnit::RemappedFile> remapped;
    if (request.remap) remapped.emplace_back(request.path, llvm::MemoryBuffer::getMemBufferCopy(request.text, request.path).release());
    auto ast = cl::CreateASTUnitFromCommandLine(argv.data(), argv.data() + argv.size(), std::make_shared<cl::PCHContainerOperations>(), options, diags,
                                                request.resource_directory, false, {}, false, cl::CaptureDiagsKind::All, remapped, true, 0,
                                                cl::TU_Complete, false, true, false, cl::SkipFunctionBodiesScope::None, false, true);
    if (!ast && rejected != nullptr) *rejected = command_rejection(args);
    return ast;
}

} // namespace mcxx::clang_backend
