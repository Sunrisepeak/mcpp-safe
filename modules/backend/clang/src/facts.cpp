// mcxx.backend.clang implementation unit: the definitions of :facts (MC3 v0 facts of a unit).
module;

#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/AST/DynamicRecursiveASTVisitor.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/StmtCXX.h>
#include <clang/Lex/MacroInfo.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/QualTypeNames.h>
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
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/ModuleMap.h>
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
import mcxx.ifc;
import :support;
import :unit;
import :facts;
import :flow;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fs = std::filesystem;
using msa::Position;
using msa::Range;
using msa::Location;
namespace fact = msa::fact;

// Shared with :flow (declared in :facts).
// The namespace code in `dc` belongs to ("" = global).
std::string namespace_of(const cl::DeclContext* dc) {
    for (; dc != nullptr; dc = dc->getParent()) {
        if (const auto* ns = llvm::dyn_cast<cl::NamespaceDecl>(dc)) return plain_name(ns);
    }
    return {};
}

// Whether default-initialization leaves an object of this type indeterminate ([dcl.init.general],
// [basic.indet]): a scalar, or an array of them.
bool indeterminate_type(const cl::ASTContext& ctx, cl::QualType type) {
    if (type.isNull() || type->isDependentType()) return false;
    if (const auto* array = ctx.getAsArrayType(type)) return indeterminate_type(ctx, array->getElementType());
    return type->isScalarType();
}

// A class (or an array of one) default-initialized by a trivial default constructor: its members
// are left indeterminate. An empty class has none.
bool trivially_default_constructed(const cl::CXXConstructExpr* construct) {
    if (construct == nullptr || construct->getNumArgs() != 0 || construct->isListInitialization() || construct->getParenOrBraceRange().isValid()) return false;
    const cl::CXXConstructorDecl* ctor { construct->getConstructor() };
    if (ctor == nullptr || !ctor->isTrivial()) return false;
    const cl::CXXRecordDecl* record { ctor->getParent() };
    return record != nullptr && !record->isEmpty();
}

namespace {

std::string type_text(const cl::ASTContext& ctx, cl::QualType type) {
    if (type.isNull()) return {};
    cl::PrintingPolicy policy { printing_policy(ctx) };
    policy.AnonymousTagNameStyle = llvm::to_underlying(cl::PrintingPolicy::AnonymousTagMode::Plain);
    // Names in it as MC3 names them: without inline namespaces (libc++'s std::__1).
    policy.SuppressInlineNamespace = llvm::to_underlying(cl::PrintingPolicy::SuppressInlineNamespaceMode::All);
    return type.getAsString(policy);
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

// Whether a declared type holds a raw pointer anywhere: T*, an array of them, a template argument.
bool holds_pointer(const cl::ASTContext& ctx, cl::QualType type, int depth = 0) {
    if (type.isNull() || depth > 8) return false;
    type = type.getNonReferenceType();
    if (type->isPointerType()) return true;
    if (const auto* array = ctx.getAsArrayType(type)) return holds_pointer(ctx, array->getElementType(), depth + 1);
    if (const auto* spec = specialization_of(type)) {
        for (const auto& arg : spec->getTemplateArgs().asArray())
            if (arg.getKind() == cl::TemplateArgument::Type && holds_pointer(ctx, arg.getAsType(), depth + 1)) return true;
        return false;
    }
    if (const auto* tst = type->getAs<cl::TemplateSpecializationType>())
        for (const auto& arg : tst->template_arguments())
            if (arg.getKind() == cl::TemplateArgument::Type && holds_pointer(ctx, arg.getAsType(), depth + 1)) return true;
    return false;
}

bool is_initializer_list(cl::QualType type) {
    const auto* record = type.getNonReferenceType().getCanonicalType()->getAsCXXRecordDecl();
    return record != nullptr && record->getName() == "initializer_list" && record->isInStdNamespace();
}

// The class a base specifier names: a class, or a specialization by its template's name (MC3 names
// classes so); nothing for a template parameter or a dependent name.
const cl::NamedDecl* base_class(cl::QualType type) {
    if (type.isNull()) return nullptr;
    if (const auto* spec = specialization_of(type)) return spec->getSpecializedTemplate();
    if (const auto* tst = type->getAs<cl::TemplateSpecializationType>())
        if (const auto* td = tst->getTemplateName().getAsTemplateDecl()) return td;
    return type->getAsCXXRecordDecl();
}

// A type with every name in it fully qualified (inline namespaces left out): what a template
// parameter's default names, for a reader that is not where the template is.
std::string qualified_type_text(const cl::ASTContext& ctx, cl::QualType type) {
    if (type.isNull()) return {};
    cl::PrintingPolicy policy { printing_policy(ctx) };
    policy.AnonymousTagNameStyle = llvm::to_underlying(cl::PrintingPolicy::AnonymousTagMode::Plain);
    policy.SuppressInlineNamespace = llvm::to_underlying(cl::PrintingPolicy::SuppressInlineNamespaceMode::All);
    return cl::TypeName::getFullyQualifiedName(type, ctx, policy);
}

// A class template's, an alias template's or a function template's parameters (MC3 0.6.0, 0.7.0): "class T", "class ...Ts",
// "class A = std::allocator<T>", "std::size_t N", "template class C".
std::vector<std::string> template_parameters_of(const cl::ASTContext& ctx, const cl::NamedDecl* d) {
    const cl::TemplateParameterList* list { nullptr };
    if (const auto* record = llvm::dyn_cast<cl::CXXRecordDecl>(d))
        if (const auto* t = record->getDescribedClassTemplate()) list = t->getTemplateParameters();
    if (const auto* alias = llvm::dyn_cast<cl::TypeAliasDecl>(d))
        if (const auto* t = alias->getDescribedAliasTemplate()) list = t->getTemplateParameters();
    if (const auto* fn = llvm::dyn_cast<cl::FunctionDecl>(d))
        if (const auto* t = fn->getDescribedFunctionTemplate()) list = t->getTemplateParameters();
    std::vector<std::string> out;
    if (list == nullptr) return out;
    for (const auto* p : *list) {
        const std::string name { p->getName() };
        const auto named = [&](std::string head, bool pack) {
            if (pack) return head + " ..." + name;
            return name.empty() ? head : head + " " + name;
        };
        if (const auto* type = llvm::dyn_cast<cl::TemplateTypeParmDecl>(p)) {
            std::string text { named("class", type->isParameterPack()) };
            if (type->hasDefaultArgument()) text += " = " + qualified_type_text(ctx, type->getDefaultArgument().getArgument().getAsType());
            out.push_back(std::move(text));
        } else if (const auto* value = llvm::dyn_cast<cl::NonTypeTemplateParmDecl>(p)) {
            out.push_back(named(type_text(ctx, value->getType()), value->isParameterPack()));
        } else if (const auto* tt = llvm::dyn_cast<cl::TemplateTemplateParmDecl>(p)) {
            out.push_back(named("template class", tt->isParameterPack()));
        }
    }
    return out;
}

// A class's direct bases (MC3 0.5.0).
std::vector<std::string> bases_of(const cl::NamedDecl* d) {
    std::vector<std::string> out;
    const auto* record = llvm::dyn_cast<cl::CXXRecordDecl>(d);
    const auto* def = record != nullptr ? record->getDefinition() : nullptr;
    if (def == nullptr) return out;
    for (const auto& base : def->bases())
        if (const auto* named = base_class(base.getType())) {
            std::string name { plain_name(named) };
            if (std::ranges::find(out, name) == out.end()) out.push_back(std::move(name));
        }
    return out;
}

// A declaration's MC3 members that do not depend on where it is written (its ranges apart): entity,
// qualified name, container, kind, exported, local, the flags, and with `types` its type and templates.
fact::Declaration describe(const cl::ASTContext& ctx, const cl::NamedDecl* d, bool types) {
    fact::Declaration decl;
    decl.container = namespace_of(d->getDeclContext());
    decl.entity = usr_of(d);
    decl.qualified_name = plain_name(d);
    decl.kind = kind_of(d);
    decl.exported = d->isInExportDeclContext();
    // Inside a function's body: a local variable, or a local class and what it declares.
    decl.local = !llvm::isa<cl::ParmVarDecl>(d) && d->getParentFunctionOrMethod() != nullptr;
    cl::QualType type;
    // An enumerator's type is its enumeration, a namespace alias's the namespace it names: each by
    // the qualified name this specification gives it (MC3 0.8.0).
    if (const auto* enumerator = llvm::dyn_cast<cl::EnumConstantDecl>(d)) {
        if (const auto* enumeration = llvm::dyn_cast<cl::EnumDecl>(enumerator->getDeclContext()); enumeration && types)
            decl.type = plain_name(enumeration);
        return decl;
    }
    if (const auto* alias = llvm::dyn_cast<cl::NamespaceAliasDecl>(d)) {
        if (const auto* named = alias->getNamespace(); named && types) decl.type = plain_name(named);
        return decl;
    }
    // A using-declaration's is what it names, by its qualified name (`mcpplibs::cmdline::detail::Option`
    // for `using detail::Option;`): what an importer finds through the name it introduces.
    if (const auto* using_ = llvm::dyn_cast<cl::UsingDecl>(d)) {
        if (types)
            for (const auto* shadow : using_->shadows())
                if (const auto* target = shadow->getTargetDecl()) {
                    decl.type = plain_name(target);
                    break;
                }
        return decl;
    }
    if (const auto* parm = llvm::dyn_cast<cl::ParmVarDecl>(d)) type = parm->getOriginalType();
    else if (const auto* value = llvm::dyn_cast<cl::ValueDecl>(d)) type = value->getType();
    else if (const auto* alias = llvm::dyn_cast<cl::TypedefNameDecl>(d)) type = alias->getUnderlyingType();
    if (const auto* fn = llvm::dyn_cast<cl::FunctionDecl>(d)) {
        if (types) collect_templates(ctx, fn->getReturnType(), decl.templates);
        decl.pointer = holds_pointer(ctx, fn->getReturnType());
        // A function's or a method's return type (MC3 0.5.0); a constructor, a destructor and a
        // conversion function have none (their names say it).
        const bool returns { !llvm::isa<cl::CXXConstructorDecl, cl::CXXDestructorDecl, cl::CXXConversionDecl>(fn) };
        if (decl.pointer || (types && returns)) decl.type = type_text(ctx, fn->getReturnType());
        decl.c_variadic = fn->isVariadic();
        // Its parameters' types (MC3 0.7.0): what a call's arguments choose an overload by.
        if (types) {
            decl.parameters.emplace();
            for (const auto* p : fn->parameters())
                decl.parameters->push_back(type_text(ctx, p->getOriginalType()) + (p->hasDefaultArg() ? " =" : ""));
        }
    } else if (types) {
        collect_templates(ctx, type, decl.templates);
    }
    if (!type.isNull() && !llvm::isa<cl::FunctionDecl>(d)) {
        decl.c_array = type.getNonReferenceType()->isArrayType();
        decl.pointer = holds_pointer(ctx, type);
        if (types || decl.c_array || decl.pointer) decl.type = type_text(ctx, type);
    }
    if (const auto* record = llvm::dyn_cast<cl::RecordDecl>(d)) decl.is_union = record->isUnion();
    if (types) {
        decl.bases = bases_of(d);
        decl.template_parameters = template_parameters_of(ctx, d);
    }
    return decl;
}

class Collector final : public cl::DynamicRecursiveASTVisitor {
public:
    Collector(cl::ASTContext& ctx, fact::Facts& facts, fact::Kinds needs)
        : ctx_ { ctx }, sm_ { ctx.getSourceManager() }, lo_ { ctx.getLangOpts() }, facts_ { facts }, needs_ { needs } {
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
        if (wants(fact::Kinds::attributes))
            for (const auto* attr : d->specific_attrs<cl::AnnotateAttr>()) {
                const llvm::StringRef text { attr->getAnnotation() };
                if (!text.starts_with("mcpp::attr|")) continue;
                fact::Attribute a;
                a.range = range_of(d->getSourceRange()).value_or(Range {});
                a.name_range = range_of(attr->getRange()).value_or(a.range);
                a.container = namespace_of(d->getDeclContext());
                a.entity = usr_of(d);
                if (const auto* named = llvm::dyn_cast<cl::NamedDecl>(d)) a.declaration = plain_name(named);
                a.kind = kind_of(d);
                const auto [name, args] = text.drop_front(std::string_view { "mcpp::attr|" }.size()).split('|');
                a.name = name.str();
                if (!args.empty()) {
                    llvm::SmallVector<llvm::StringRef, 4> parts;
                    args.split(parts, '\x1f');
                    for (auto p : parts) a.arguments.emplace_back(p.str());
                }
                facts_.attributes.push_back(std::move(a));
            }
        if (!wants(fact::Kinds::suppressions)) return true;
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
        if (!wants(fact::Kinds::declarations)) return true;
        // A parameter of a function; one written in a function type inside another declaration
        // (`std::function<void(int level)>`) is part of that type (MC3 §4.2).
        if (const auto* parm = llvm::dyn_cast<cl::ParmVarDecl>(d);
            parm != nullptr && !llvm::isa<cl::FunctionDecl, cl::BlockDecl, cl::ObjCMethodDecl>(parm->getDeclContext()))
            return true;
        if (!llvm::isa<cl::VarDecl, cl::FieldDecl, cl::FunctionDecl, cl::TypedefNameDecl, cl::RecordDecl, cl::EnumDecl, cl::EnumConstantDecl, cl::NamespaceDecl,
                       cl::NamespaceAliasDecl, cl::UsingDecl>(d))
            return true;
        // An inheriting constructor's using-declaration (`using Base::Base;`) names no member of its own.
        if (const auto* using_ = llvm::dyn_cast<cl::UsingDecl>(d); using_ && using_->getDeclName().getNameKind() == cl::DeclarationName::CXXConstructorName)
            return true;
        if (const auto* record = llvm::dyn_cast<cl::RecordDecl>(d); record && !record->isThisDeclarationADefinition()) return true;
        // Types as text only when asked for, or for a declaration a flag marks: the text is what
        // costs (a gate over C arrays pays for the arrays' types, not for every variable's).
        fact::Declaration decl { describe(ctx_, d, wants(fact::Kinds::declaration_types)) };
        decl.range = range_of(d->getSourceRange()).value_or(Range {});
        decl.name = range_of(d->getLocation()).value_or(decl.range);
        facts_.declarations.push_back(std::move(decl));
        return true;
    }

    bool VisitVarDecl(cl::VarDecl* v) override {
        if (!wants(fact::Kinds::initializations) || llvm::isa<cl::ParmVarDecl>(v)) return true;
        if (!v->hasInit()) {
            // `int x;`: a local left indeterminate (a static or a thread_local is zero-initialized).
            if (v->hasLocalStorage() && !v->isExceptionVariable() && indeterminate_type(ctx_, v->getType()))
                add_default_initialization(v);
            return true;
        }
        if (const auto* construct = llvm::dyn_cast<cl::CXXConstructExpr>(v->getInit()->IgnoreImplicit());
            construct != nullptr && construct->getNumArgs() == 0 && !construct->isListInitialization() && construct->getParenOrBraceRange().isInvalid()) {
            // `T x;` of a class: default-initialization by its constructor (Clang's "callinit" with no parentheses).
            if (v->hasLocalStorage() && trivially_default_constructed(construct)) add_default_initialization(v);
            return true;
        }
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
        if (!wants(fact::Kinds::initializations) || !f->hasInClassInitializer() || f->getInClassInitializer() == nullptr) return true;
        const fact::InitForm form { f->getInClassInitStyle() == cl::ICIS_ListInit ? fact::InitForm::direct_list : fact::InitForm::copy };
        add_initialization(f, f->getType(), f->getInClassInitializer(), form, true);
        return true;
    }

    bool VisitExplicitCastExpr(cl::ExplicitCastExpr* e) override {
        if (!wants(fact::Kinds::casts) || !in_main(e->getBeginLoc())) return true;
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
        cast.to_scalar = e->getType()->isScalarType();
        facts_.casts.push_back(std::move(cast));
        return true;
    }

    bool VisitCXXNewExpr(cl::CXXNewExpr* e) override {
        if (!wants(fact::Kinds::allocations) || !in_main(e->getBeginLoc())) return true;
        facts_.allocations.push_back({ { range_of(e->getSourceRange()).value_or(Range {}), container() }, false, e->isArray(),
                                       type_text(ctx_, e->getAllocatedType()) });
        return true;
    }

    bool VisitCXXDeleteExpr(cl::CXXDeleteExpr* e) override {
        if (!wants(fact::Kinds::allocations) || !in_main(e->getBeginLoc())) return true;
        facts_.allocations.push_back({ { range_of(e->getSourceRange()).value_or(Range {}), container() }, true, e->isArrayForm(),
                                       type_text(ctx_, e->getDestroyedType()) });
        return true;
    }

    bool VisitBinaryOperator(cl::BinaryOperator* e) override {
        if (!wants(fact::Kinds::pointer_arithmetic) || !in_main(e->getOperatorLoc())) return true;
        const auto op = e->getOpcode();
        if (op != cl::BO_Add && op != cl::BO_Sub && op != cl::BO_AddAssign && op != cl::BO_SubAssign) return true;
        const cl::Expr* pointer { e->getLHS()->getType()->isPointerType() ? e->getLHS() : e->getRHS()->getType()->isPointerType() ? e->getRHS() : nullptr };
        if (pointer == nullptr) return true;
        facts_.pointer_arithmetic.push_back({ { range_of(e->getSourceRange()).value_or(Range {}), container() }, e->getOpcodeStr().str(),
                                              type_text(ctx_, pointer->getType()) });
        return true;
    }

    bool VisitUnaryOperator(cl::UnaryOperator* e) override {
        if (!wants(fact::Kinds::pointer_arithmetic) || !in_main(e->getOperatorLoc()) || !e->isIncrementDecrementOp() || !e->getSubExpr()->getType()->isPointerType()) return true;
        facts_.pointer_arithmetic.push_back({ { range_of(e->getSourceRange()).value_or(Range {}), container() },
                                              e->isIncrementOp() ? "++" : "--", type_text(ctx_, e->getSubExpr()->getType()) });
        return true;
    }

    bool VisitArraySubscriptExpr(cl::ArraySubscriptExpr* e) override {
        if (!wants(fact::Kinds::pointer_arithmetic) || !in_main(e->getBeginLoc())) return true;
        const cl::Expr* base { e->getBase()->IgnoreParenImpCasts() };
        if (!base->getType()->isPointerType()) return true;   // a C array's own subscript is the c-array feature's
        facts_.pointer_arithmetic.push_back({ { range_of(e->getSourceRange()).value_or(Range {}), container() }, "[]", type_text(ctx_, base->getType()) });
        return true;
    }

    bool VisitGotoStmt(cl::GotoStmt* s) override {
        if (!wants(fact::Kinds::gotos) || !in_main(s->getGotoLoc())) return true;
        facts_.gotos.push_back({ { range_of(s->getSourceRange()).value_or(Range {}), container() }, s->getLabel()->getName().str() });
        return true;
    }

    bool VisitIndirectGotoStmt(cl::IndirectGotoStmt* s) override {
        if (!wants(fact::Kinds::gotos) || !in_main(s->getGotoLoc())) return true;
        facts_.gotos.push_back({ { range_of(s->getSourceRange()).value_or(Range {}), container() }, "*" });
        return true;
    }

    // Uses of constructs a gate may subtract.
    bool VisitCXXThrowExpr(cl::CXXThrowExpr* e) override { return use(e->getThrowLoc(), e->getSourceRange(), "throw"); }
    bool VisitCXXTryStmt(cl::CXXTryStmt* s) override { return use(s->getTryLoc(), { s->getTryLoc(), s->getTryBlock()->getLBracLoc() }, "try"); }
    bool VisitAsmStmt(cl::AsmStmt* s) override { return use(s->getAsmLoc(), s->getSourceRange(), "asm"); }
    bool VisitFileScopeAsmDecl(cl::FileScopeAsmDecl* d) override { return use(d->getAsmLoc(), d->getSourceRange(), "asm"); }
    bool VisitVAArgExpr(cl::VAArgExpr* e) override { return use(e->getBeginLoc(), e->getSourceRange(), "va_arg"); }
    bool VisitCXXTypeidExpr(cl::CXXTypeidExpr* e) override {
        const cl::QualType operand { e->isTypeOperand() ? e->getTypeOperand(ctx_) : e->getExprOperand()->getType() };
        return use(e->getBeginLoc(), e->getSourceRange(), "typeid", type_text(ctx_, operand));
    }

private:
    cl::ASTContext& ctx_;
    const cl::SourceManager& sm_;
    const cl::LangOptions& lo_;
    fact::Facts& facts_;
    fact::Kinds needs_;
    cl::FileID main_;
    const cl::Decl* current_ { nullptr };

    bool wants(fact::Kinds kind) const { return fact::contains(needs_, kind); }

    bool use(cl::SourceLocation at, cl::SourceRange range, std::string construct, std::string detail = {}) {
        if (!wants(fact::Kinds::uses) || !in_main(at)) return true;
        facts_.uses.push_back({ { range_of(range).value_or(Range {}), container() }, std::move(construct), std::move(detail) });
        return true;
    }

    void add_default_initialization(const cl::VarDecl* v) {
        if (!in_main(v->getLocation())) return;
        fact::Initialization fact;
        fact.range = range_of(v->getSourceRange()).value_or(Range {});
        fact.name = range_of(v->getLocation()).value_or(fact.range);
        fact.container = namespace_of(v->getDeclContext());
        fact.entity = usr_of(v);
        fact.variable = plain_name(v);
        fact.type = type_text(ctx_, v->getType());
        if (const auto* spec = specialization_of(v->getType())) fact.type_template = plain_name(spec->getSpecializedTemplate());
        fact.form = fact::InitForm::default_init;
        fact.indeterminate = true;
        facts_.initializations.push_back(std::move(fact));
    }

    bool in_main(cl::SourceLocation loc) const { return loc.isValid() && sm_.getFileID(sm_.getFileLoc(loc)) == main_; }

    std::optional<Range> range_of(cl::SourceRange range) const { return source_range(sm_, lo_, range, main_); }
    // A name token in the main file: its position and length, without naming the file (a path
    // per declaration is what made collecting declarations cost more than parsing them).
    std::optional<Range> range_of(cl::SourceLocation loc) const {
        if (loc.isInvalid()) return std::nullopt;
        const cl::SourceLocation file { sm_.getFileLoc(loc) };
        if (sm_.getFileID(file) != main_) return std::nullopt;
        const Position begin { position_of(sm_, file) };
        const unsigned length { cl::Lexer::MeasureTokenLength(sm_.getSpellingLoc(loc), sm_, lo_) };
        return Range { begin, { begin.line, begin.column + std::max(1u, length) } };
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

// The #include directives of the main file that were entered, from the source manager: each file
// entered from the main file is one (a guarded header entered again is not). In the global module
// fragment when it comes before the module declaration.
void collect_includes(cl::ASTContext& ctx, fact::Facts& facts) {
    const auto& sm = ctx.getSourceManager();
    const cl::FileID main { sm.getMainFileID() };
    cl::SourceLocation module_decl;
    if (const cl::Module* m = ctx.getCurrentNamedModule()) module_decl = m->DefinitionLoc;
    const llvm::StringRef buffer { sm.getBufferData(main) };
    for (unsigned i { 0 }; i < sm.local_sloc_entry_size(); ++i) {
        const auto& entry = sm.getLocalSLocEntry(i);
        if (!entry.isFile()) continue;
        const cl::SourceLocation at { entry.getFile().getIncludeLoc() };
        if (at.isInvalid() || at.isMacroID() || sm.getFileID(at) != main) continue;
        const unsigned offset { sm.getFileOffset(at) };
        if (offset >= buffer.size()) continue;
        const char open { buffer[offset] };
        const char close { open == '<' ? '>' : open == '"' ? '"' : '\0' };
        if (close == '\0') continue;
        const std::size_t end { buffer.find(close, offset + 1) };
        if (end == llvm::StringRef::npos) continue;
        fact::Include include;
        include.header = buffer.slice(offset, end + 1).str();
        const unsigned line { sm.getLineNumber(main, offset) - 1 };
        const std::size_t line_start { buffer.rfind('\n', offset) == llvm::StringRef::npos ? 0 : buffer.rfind('\n', offset) + 1 };
        include.range = { { line, 0 }, { line, static_cast<std::uint32_t>(end + 1 - line_start) } };
        include.global_module_fragment = module_decl.isValid() && sm.isBeforeInTranslationUnit(at, module_decl);
        facts.includes.push_back(std::move(include));
    }
}

// A module's BMI, as this compile loaded it ("" when it came from no file).
std::string bmi_of(const cl::Module* m) {
    if (m == nullptr) return {};
    const cl::ModuleFileName* file { m->getASTFileName() };
    if (file == nullptr || file->str().empty()) return {};
    return absolute_path(file->str().str());
}

// The file's imports of named modules, and what each brings in as the modules' MC2 interfaces say
// -- the .ifc beside each BMI, or the store's copy -- following their re-exports: never a source
// (M1.2, A1.2.2). A module is found by name among those this compile loaded.
std::vector<fact::Import> imports_of(cl::ASTContext& ctx, const cl::Preprocessor* pp) {
    std::vector<fact::Import> out;
    const auto& sm = ctx.getSourceManager();
    const cl::FileID main { sm.getMainFileID() };
    const auto range_of = [&](cl::SourceRange r) { return source_range(sm, ctx.getLangOpts(), r, main); };
    const auto loaded = [&](std::string_view name) -> const cl::Module* {
        if (pp == nullptr) return nullptr;
        return pp->getHeaderSearchInfo().getModuleMap().findModule(llvm::StringRef { name.data(), name.size() });
    };
    const auto visit = [&](const cl::ImportDecl* d, bool exported) {
        const cl::Module* m { d->getImportedModule() };
        if (d->isImplicit() || m == nullptr || !m->isNamedModule() || !sm.isInMainFile(sm.getExpansionLoc(d->getLocation()))) return;
        fact::Import im;
        im.range = range_of(d->getSourceRange()).value_or(Range {});
        im.container = {};
        im.module = m->getFullModuleName();
        const auto locs = d->getIdentifierLocs();
        if (!locs.empty()) {
            const auto first = range_of({ locs.front(), locs.front() });
            const auto last = range_of({ locs.back(), locs.back() });
            if (first && last) im.name = { first->begin, last->end };
        }
        im.exported = exported;
        std::vector<std::string> todo { im.module };
        std::set<std::string> seen;
        for (std::size_t i { 0 }; i < todo.size(); ++i) {
            if (!seen.insert(todo[i]).second) continue;
            fact::Import::Interface in;
            in.module = todo[i];
            const cl::Module* mod { i == 0 ? m : loaded(todo[i]) };
            const std::string bmi { bmi_of(mod) };
            const auto unit = bmi.empty() ? nullptr : ifc::interface_for(bmi);
            if (unit) {
                in.found = true;
                in.profiles = unit->dialect.profiles;
                for (const auto& f : unit->dialect.features) in.levels.emplace_back(f.id, f.level);
                for (const auto& decl : unit->declarations)
                    if (decl.exported) in.exported.push_back(decl);
                for (const auto& r : unit->reexports) todo.push_back(r);
            }
            im.interfaces.push_back(std::move(in));
        }
        out.push_back(std::move(im));
    };
    for (const cl::Decl* d : ctx.getTranslationUnitDecl()->decls()) {
        if (const auto* import = llvm::dyn_cast<cl::ImportDecl>(d)) visit(import, false);
        else if (const auto* e = llvm::dyn_cast<cl::ExportDecl>(d))
            for (const cl::Decl* inner : e->decls())
                if (const auto* import = llvm::dyn_cast<cl::ImportDecl>(inner)) visit(import, true);
    }
    return out;
}

} // namespace

fact::Facts facts_of(cl::ASTContext& ctx, const cl::Preprocessor* pp, fact::Kinds needs) {
    fact::Facts facts;
    facts.collected = needs;
    constexpr fact::Kinds from_ast { fact::Kinds::declarations | fact::Kinds::initializations | fact::Kinds::casts | fact::Kinds::allocations |
                                     fact::Kinds::pointer_arithmetic | fact::Kinds::gotos | fact::Kinds::uses | fact::Kinds::suppressions |
                                     fact::Kinds::attributes };
    if ((std::to_underlying(needs) & std::to_underlying(from_ast)) != 0) {
        Collector collector { ctx, facts, needs };
        collector.TraverseDecl(ctx.getTranslationUnitDecl());
    }
    if (fact::contains(needs, fact::Kinds::includes)) collect_includes(ctx, facts);
    if (fact::contains(needs, fact::Kinds::imports)) facts.imports = imports_of(ctx, pp);
    if (fact::contains(needs, fact::Kinds::control_flow)) facts.control_flow = flows_of(ctx);
    if (pp != nullptr && fact::contains(needs, fact::Kinds::macros)) {
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

std::vector<fact::Declaration> reachable_of(cl::ASTContext& ctx) {
    std::vector<fact::Declaration> out;
    std::set<const cl::Decl*> seen;
    const auto push = [&](const cl::NamedDecl* d) {
        fact::Declaration decl { describe(ctx, d, true) };
        // A member of a partial specialization (a template defined only by its partial specializations
        // has theirs): named under the template, as a reference to it names it.
        if (const auto* part = llvm::dyn_cast<cl::ClassTemplatePartialSpecializationDecl>(d->getDeclContext()))
            decl.qualified_name = plain_name(part->getSpecializedTemplate()) + "::" + d->getNameAsString();
        decl.exported = true;
        decl.local = false;
        out.push_back(std::move(decl));
    };
    const auto enumerators = [&](const cl::EnumDecl* e) {
        if (const auto* def = e->getDefinition())
            for (const auto* c : def->enumerators())
                if (seen.insert(c).second) push(c);
    };
    std::function<void(const cl::NamedDecl*, int)> add = [&](const cl::NamedDecl* d, int depth) {
        if (d == nullptr || depth > 8 || d->isImplicit()) return;
        if (const auto* t = llvm::dyn_cast<cl::TemplateDecl>(d); t != nullptr && t->getTemplatedDecl() != nullptr) d = t->getTemplatedDecl();
        if (!seen.insert(d->getCanonicalDecl()).second) return;
        if (llvm::isa<cl::UsingShadowDecl, cl::UsingDecl, cl::NamespaceDecl>(d)) return;
        push(d);
        if (const auto* e = llvm::dyn_cast<cl::EnumDecl>(d)) enumerators(e);
        // An alias reaches the class it names (MC2 1.3.0): `using json = basic_json<>` is a use of
        // basic_json's members; and the enumeration it names, with its enumerators (1.4.0:
        // `json::value_t::string` is `nlohmann::detail::value_t`'s).
        if (const auto* alias = llvm::dyn_cast<cl::TypedefNameDecl>(d)) {
            if (const auto* named = base_class(alias->getUnderlyingType())) add(named, depth + 1);
            if (const auto* e = alias->getUnderlyingType()->getAsEnumDecl()) add(e, depth + 1);
        }
        const auto* record = llvm::dyn_cast<cl::CXXRecordDecl>(d);
        const auto* def = record != nullptr ? record->getDefinition() : nullptr;
        // Its definition; a class template's only declared, its partial specializations' (libc++'s
        // `__optional_destruct_base<_Tp, bool>`, where std::optional's reset() is; `__atomic_base`).
        std::vector<const cl::CXXRecordDecl*> bodies;
        if (def != nullptr) bodies.push_back(def);
        else if (record != nullptr && record->getDescribedClassTemplate() != nullptr) {
            llvm::SmallVector<cl::ClassTemplatePartialSpecializationDecl*, 4> parts;
            record->getDescribedClassTemplate()->getPartialSpecializations(parts);
            for (const auto* part : parts)
                if (part->getDefinition() != nullptr) bodies.push_back(part->getDefinition());
        }
        for (const auto* body : bodies) {
            // What it inherits reaches an importer too: its bases (MC2 1.3.0), with their members.
            for (const auto& base : body->bases())
                if (const auto* named = base_class(base.getType())) add(named, depth + 1);
            for (const auto* member : body->decls()) {
                const auto* nd = llvm::dyn_cast<cl::NamedDecl>(member);
                if (nd == nullptr || nd->isImplicit()) continue;
                // A private or protected member is not an importer's, but a type alias among them is what
                // a public member's type may be written with (libc++'s `const _Path& directory_entry::path()`).
                const bool hidden { nd->getAccess() == cl::AS_private || nd->getAccess() == cl::AS_protected };
                if (hidden && !llvm::isa<cl::TypedefNameDecl>(nd)) continue;
                if (llvm::isa<cl::FieldDecl, cl::CXXMethodDecl, cl::CXXRecordDecl, cl::EnumDecl, cl::TypedefNameDecl, cl::FunctionTemplateDecl,
                              cl::ClassTemplateDecl, cl::VarDecl, cl::TypeAliasTemplateDecl, cl::VarTemplateDecl>(nd))
                    add(nd, depth + 1);
            }
        }
    };
    // What an exported alias of the unit names, when another unit declares it: `using Spec =
    // pm::Spec;` makes pm::Spec's members an importer's to use (MC2 1.6.0). Not std's: every importer
    // reads std's interface itself, and carrying it again is what made every interface megabytes.
    const auto named_by = [&](cl::QualType type) {
        const cl::NamedDecl* named { base_class(type) };
        if (named == nullptr) named = type->getAsEnumDecl();
        if (named != nullptr && named->isFromASTFile() && !plain_name(named).starts_with("std::")) add(named, 0);
    };
    // A class (or enumeration) the unit declares and does not export, named by the type an exported
    // function returns or takes -- an exported class's public member function among them (MC2 1.7.0):
    // reachable, [module.reach]/3, and what an importer's call is a member access on
    // (mcpplibs.cmdline: `App::option(std::string_view)` returns an `OptBuilder`, whose `help()` and
    // `takes_value()` are what `app.option("x").help("...")` calls).
    const auto unexported_of = [&](cl::QualType type) {
        type = type.getNonReferenceType();
        while (type->isPointerType()) type = type->getPointeeType();
        const cl::NamedDecl* named { base_class(type) };
        if (named == nullptr) named = type->getAsEnumDecl();
        if (named != nullptr && !named->isFromASTFile() && !named->isInExportDeclContext()) add(named, 0);
    };
    const auto function_reaches = [&](const cl::FunctionDecl* f) {
        if (f == nullptr || f->isImplicit() || f->getAccess() == cl::AS_private || f->getAccess() == cl::AS_protected) return;
        unexported_of(f->getReturnType());
        for (const auto* parameter : f->parameters()) unexported_of(parameter->getType());
    };
    // The unit's exported using-declarations and aliases (in classes too). Its own: what it imports is
    // in its context too (a unit that imports std sees std's exported using-declarations there), and
    // is that module's interface's to carry, not every importer's. Its own enumerators are its
    // declarations (MC3 0.8.0).
    std::function<void(const cl::DeclContext*)> walk = [&](const cl::DeclContext* dc) {
        for (const auto* d : dc->decls()) {
            if (d->isFromASTFile()) continue;
            if (const auto* u = llvm::dyn_cast<cl::UsingDecl>(d); u != nullptr && u->isInExportDeclContext()) {
                for (const auto* shadow : u->shadows()) add(shadow->getTargetDecl(), 0);
                // One an included file writes whose name is not what it names (libc++'s std module:
                // `using std::uint64_t;`, which is `::uint64_t`): the name is an importer's too, a
                // using-declaration (MC2 1.6.1) -- when it names one declaration: an overload set's
                // (`std::floor`, `::floor` and `std::__math::floor`) is not one `type` to follow. The
                // main file's own are its declarations (MC3 0.8.0).
                if (!ctx.getSourceManager().isInMainFile(u->getLocation()) && u->shadow_size() == 1 && seen.insert(u).second) {
                    fact::Declaration decl { describe(ctx, u, true) };
                    if (!decl.type.empty() && decl.type != decl.qualified_name) push(u);
                }
            } else if (const auto* alias = llvm::dyn_cast<cl::TypedefNameDecl>(d); alias != nullptr && alias->isInExportDeclContext()) {
                named_by(alias->getUnderlyingType());
            } else if (const auto* t = llvm::dyn_cast<cl::TypeAliasTemplateDecl>(d); t != nullptr && t->isInExportDeclContext() && t->getTemplatedDecl()) {
                named_by(t->getTemplatedDecl()->getUnderlyingType());
            } else if (const auto* alias = llvm::dyn_cast<cl::NamespaceAliasDecl>(d); alias != nullptr && alias->isInExportDeclContext() &&
                                                                                 !ctx.getSourceManager().isInMainFile(alias->getLocation())) {
                // An exported namespace alias an included file writes (libc++'s std module:
                // `namespace views = ranges::views;` in its std/ranges.inc): the main file's own are its
                // declarations (MC3 0.8.0).
                if (seen.insert(alias).second) push(alias);
            } else if (const auto* f = llvm::dyn_cast<cl::FunctionDecl>(d); f != nullptr && f->isInExportDeclContext()) {
                function_reaches(f);
            } else if (const auto* ft = llvm::dyn_cast<cl::FunctionTemplateDecl>(d); ft != nullptr && ft->isInExportDeclContext()) {
                function_reaches(ft->getTemplatedDecl());
            } else if (const auto* r = llvm::dyn_cast<cl::CXXRecordDecl>(d); r != nullptr && r->isInExportDeclContext() && r->isThisDeclarationADefinition()) {
                walk(r);
            } else if (llvm::isa<cl::NamespaceDecl, cl::ExportDecl, cl::LinkageSpecDecl>(d)) {
                walk(llvm::cast<cl::DeclContext>(d));
            }
        }
    };
    walk(ctx.getTranslationUnitDecl());
    return out;
}

namespace {

// A0.4.3's reference count: Clang's own visitor, its defaults (no implicit code, no template
// instantiations), each declaration by Clang's kind name. What makes a declaration the file's is
// the only rule shared with MSA: its location is in the main file, it is not implicit, and a
// parameter belongs to a function.
class Census final : public cl::RecursiveASTVisitor<Census> {
public:
    explicit Census(const cl::SourceManager& sm) : sm_ { sm } {}
    std::map<std::string, std::int64_t> counts;

    bool TraverseDecl(cl::Decl* d) {
        if (d != nullptr && !llvm::isa<cl::TranslationUnitDecl>(d) && (d->isImplicit() || !sm_.isInMainFile(sm_.getExpansionLoc(d->getLocation()))))
            return true;
        return cl::RecursiveASTVisitor<Census>::TraverseDecl(d);
    }
    bool VisitNamedDecl(cl::NamedDecl* d) {
        if (const auto* p = llvm::dyn_cast<cl::ParmVarDecl>(d); p && !llvm::isa<cl::FunctionDecl, cl::BlockDecl>(p->getDeclContext())) return true;
        if (const auto* r = llvm::dyn_cast<cl::RecordDecl>(d); r && !r->isThisDeclarationADefinition()) return true;
        ++counts[d->getDeclKindName()];
        return true;
    }

private:
    const cl::SourceManager& sm_;
};

} // namespace

std::map<std::string, std::int64_t> UnitImpl::census() const {
    return pool_->run([&] {
        std::lock_guard lock { mutex_ };
        if (!ast_) return std::map<std::string, std::int64_t> {};
        Census census { ast_->getSourceManager() };
        census.TraverseDecl(ast_->getASTContext().getTranslationUnitDecl());
        return std::move(census.counts);
    });
}

// UnitImpl::facts: computed once, on the Workspace's Clang stack.
const msa::fact::Facts& UnitImpl::facts() const {
    return *pool_->run([&]() -> const msa::fact::Facts* {
        std::lock_guard lock { mutex_ };
        if (!facts_) {
            facts_.emplace();
            if (ast_) *facts_ = facts_of(ast_->getASTContext(), &ast_->getPreprocessor());
            else facts_->certainty = msa::Certainty::unknown;   // no AST: nothing is known, which is not "none" (MC3-3-2)
        }
        return &*facts_;
    });
}

} // namespace mcxx::clang_backend
