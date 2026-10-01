// mcxx.backend.clang implementation unit: the definitions of :flow (MC3 0.9.0 control flow).
module;

#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTLambda.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DynamicRecursiveASTVisitor.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/StmtCXX.h>
#include <clang/Analysis/CFG.h>
#include <clang/Basic/SourceManager.h>
#include <llvm/ADT/DenseSet.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

module mcxx.backend.clang;

import mcxx.msa;
import mcxx.base;
import :unit;
import :facts;
import :flow;

namespace mcxx::clang_backend {

namespace cl = ::clang;
namespace fact = msa::fact;
using msa::Range;

namespace {

// The function's own local variable or parameter `e` names, if it is one MC3's events follow: a
// reference names another object, a static or thread_local one is not the function's.
const cl::VarDecl* local_of(const cl::Expr* e) {
    if (e == nullptr) return nullptr;
    const auto* ref = llvm::dyn_cast<cl::DeclRefExpr>(e->IgnoreParens());
    if (ref == nullptr) return nullptr;
    const auto* var = llvm::dyn_cast<cl::VarDecl>(ref->getDecl());
    return var != nullptr && var->hasLocalStorage() && !var->getType()->isReferenceType() ? var : nullptr;
}

const cl::DeclRefExpr* ref_of(const cl::Expr* e) { return llvm::dyn_cast<cl::DeclRefExpr>(e->IgnoreParens()); }

// Whether a local starts indeterminate: no initializer and a scalar type, or a class whose trivial
// default constructor leaves its members so (as facts' Initialization::indeterminate says it).
bool starts_indeterminate(const cl::ASTContext& ctx, const cl::VarDecl* v) {
    if (llvm::isa<cl::ParmVarDecl>(v) || v->isExceptionVariable()) return false;
    if (!v->hasInit()) return indeterminate_type(ctx, v->getType());
    return trivially_default_constructed(llvm::dyn_cast<cl::CXXConstructExpr>(v->getInit()->IgnoreImplicit()));
}

bool call_noreturn(const cl::CallExpr* e) {
    if (const auto* fn = e->getDirectCallee(); fn != nullptr && fn->isNoReturn()) return true;
    cl::QualType type { e->getCallee()->getType() };
    if (const auto* pointer = type->getAs<cl::PointerType>()) type = pointer->getPointeeType();
    const auto* ft = type->getAs<cl::FunctionType>();
    return ft != nullptr && ft->getNoReturnAttr();
}

class FlowBuilder {
public:
    explicit FlowBuilder(cl::ASTContext& ctx) : ctx_ { ctx }, sm_ { ctx.getSourceManager() }, main_ { sm_.getMainFileID() } {}

    fact::Flow build(const cl::FunctionDecl* fn) {
        fact::Flow flow;
        const cl::Stmt* body { fn->getBody() };
        flow.range = range_of(fn->getSourceRange()).value_or(Range {});
        flow.container = namespace_of(fn->getDeclContext());
        flow.entity = usr_of(fn);
        flow.function = fn->getQualifiedNameAsString();
        flow.noreturn = fn->isNoReturn();
        flow.end = range_of(cl::SourceRange { body->getEndLoc(), body->getEndLoc() }).value_or(Range {});
        if (fn->isDependentContext()) {
            flow.known = false;
            base::trace::debug("flow", "{}: a template's pattern, not computed", flow.function);
            return flow;
        }
        flow.returns_value = !fn->getReturnType()->isVoidType() && !fn->isMain() && !llvm::isa<cl::CoroutineBodyStmt>(body) &&
                             !llvm::isa<cl::CXXConstructorDecl, cl::CXXDestructorDecl>(fn);
        cl::CFG::BuildOptions options;
        options.PruneTriviallyFalseEdges = true;
        options.AddLifetime = true;
        options.AddImplicitDtors = true;
        options.setAllAlwaysAdd();   // every subexpression its own element, in evaluation order
        const std::unique_ptr<cl::CFG> cfg { cl::CFG::buildCFG(fn, const_cast<cl::Stmt*>(body), &ctx_, options) };
        if (cfg == nullptr) {
            flow.known = false;
            base::trace::debug("flow", "{}: Clang built no CFG", flow.function);
            return flow;
        }
        // A reference to a local that an enclosing expression reads, writes or takes the address of is
        // that event; any other reference to it (bound to a reference, the object of a member call)
        // may let anything write it.
        consumed_.clear();
        for (const cl::CFGBlock* block : *cfg)
            for (const auto& element : *block)
                if (const auto s = element.getAs<cl::CFGStmt>()) consume(s->getStmt());
        flow.blocks.resize(cfg->getNumBlockIDs());
        for (const cl::CFGBlock* block : *cfg) {
            fact::Block& out { flow.blocks[block->getBlockID()] };
            for (const auto& element : *block) {
                if (const auto s = element.getAs<cl::CFGStmt>()) events_of(s->getStmt(), out.events);
                else if (const auto end = element.getAs<cl::CFGLifetimeEnds>()) {
                    const cl::VarDecl* var { end->getVarDecl() };
                    if (var->getType()->isReferenceType()) continue;
                    out.events.push_back({ fact::EventKind::scope_end, range_of(end->getTriggerStmt()->getEndLoc()).value_or(Range {}), usr_of(var),
                                           var->getNameAsString() });
                }
            }
            for (const auto& next : block->succs())
                if (const cl::CFGBlock* reachable = next.getReachableBlock()) out.successors.push_back(reachable->getBlockID());
        }
        flow.entry = cfg->getEntry().getBlockID();
        flow.exit = cfg->getExit().getBlockID();
        base::trace::count("flow.functions");
        return flow;
    }

private:
    void consume(const cl::Stmt* s) {
        const auto mark = [&](const cl::Expr* e) {
            if (local_of(e) != nullptr) consumed_.insert(ref_of(e));
        };
        if (const auto* cast = llvm::dyn_cast<cl::ImplicitCastExpr>(s); cast != nullptr && cast->getCastKind() == cl::CK_LValueToRValue)
            mark(cast->getSubExpr());
        else if (const auto* op = llvm::dyn_cast<cl::BinaryOperator>(s); op != nullptr && op->isAssignmentOp())
            mark(op->getLHS());
        else if (const auto* un = llvm::dyn_cast<cl::UnaryOperator>(s); un != nullptr && (un->isIncrementDecrementOp() || un->getOpcode() == cl::UO_AddrOf))
            mark(un->getSubExpr());
    }

    void add(std::vector<fact::Event>& out, fact::EventKind kind, const cl::Expr* e, const cl::VarDecl* var) {
        out.push_back({ kind, range_of(e->getSourceRange()).value_or(Range {}), usr_of(var), var->getNameAsString() });
    }

    void events_of(const cl::Stmt* s, std::vector<fact::Event>& out) {
        if (const auto* decl = llvm::dyn_cast<cl::DeclStmt>(s)) {
            for (const cl::Decl* d : decl->decls()) {
                const auto* var = llvm::dyn_cast<cl::VarDecl>(d);
                if (var == nullptr || !var->hasLocalStorage() || var->getType()->isReferenceType()) continue;
                fact::Event e { fact::EventKind::declare, range_of(var->getLocation()).value_or(Range {}), usr_of(var), var->getNameAsString() };
                e.indeterminate = starts_indeterminate(ctx_, var);
                out.push_back(std::move(e));
            }
        } else if (const auto* cast = llvm::dyn_cast<cl::ImplicitCastExpr>(s); cast != nullptr && cast->getCastKind() == cl::CK_LValueToRValue) {
            if (const cl::VarDecl* var { local_of(cast->getSubExpr()) }) add(out, fact::EventKind::read, cast->getSubExpr(), var);
        } else if (const auto* op = llvm::dyn_cast<cl::BinaryOperator>(s); op != nullptr && op->isAssignmentOp()) {
            if (const cl::VarDecl* var { local_of(op->getLHS()) }) {
                if (op->isCompoundAssignmentOp()) add(out, fact::EventKind::read, op->getLHS(), var);
                add(out, fact::EventKind::write, op->getLHS(), var);
            }
        } else if (const auto* un = llvm::dyn_cast<cl::UnaryOperator>(s)) {
            if (const cl::VarDecl* var { local_of(un->getSubExpr()) }) {
                if (un->isIncrementDecrementOp()) {
                    add(out, fact::EventKind::read, un->getSubExpr(), var);
                    add(out, fact::EventKind::write, un->getSubExpr(), var);
                } else if (un->getOpcode() == cl::UO_AddrOf)
                    add(out, fact::EventKind::address, un->getSubExpr(), var);
            }
        } else if (const auto* ref = llvm::dyn_cast<cl::DeclRefExpr>(s)) {
            if (const cl::VarDecl* var { local_of(ref) }; var != nullptr && !consumed_.contains(ref)) add(out, fact::EventKind::address, ref, var);
        } else if (const auto* call = llvm::dyn_cast<cl::CallExpr>(s)) {
            fact::Event e { fact::EventKind::call, range_of(call->getSourceRange()).value_or(Range {}) };
            if (const cl::FunctionDecl* callee { call->getDirectCallee() }) {
                e.entity = usr_of(callee);
                e.name = callee->getQualifiedNameAsString();
            }
            e.noreturn = call_noreturn(call);
            out.push_back(std::move(e));
        } else if (llvm::isa<cl::CXXThrowExpr>(s)) {
            out.push_back({ fact::EventKind::throw_, range_of(s->getSourceRange()).value_or(Range {}) });
        } else if (llvm::isa<cl::ReturnStmt>(s)) {
            out.push_back({ fact::EventKind::return_, range_of(s->getSourceRange()).value_or(Range {}) });
        }
    }

    std::optional<Range> range_of(cl::SourceRange range) const { return source_range(sm_, ctx_.getLangOpts(), range, main_); }
    std::optional<Range> range_of(cl::SourceLocation loc) const { return range_of(cl::SourceRange { loc, loc }); }

    cl::ASTContext& ctx_;
    const cl::SourceManager& sm_;
    cl::FileID main_;
    llvm::DenseSet<const cl::DeclRefExpr*> consumed_;
};

// The functions with bodies the file's own code defines, lambdas' included.
class Finder final : public cl::DynamicRecursiveASTVisitor {
public:
    explicit Finder(const cl::SourceManager& sm) : sm_ { sm }, main_ { sm.getMainFileID() } {}

    bool TraverseDecl(cl::Decl* d) override {
        if (d == nullptr) return true;
        if (!llvm::isa<cl::TranslationUnitDecl>(d) && (d->isImplicit() || !in_main(d->getLocation()))) return true;
        return cl::DynamicRecursiveASTVisitor::TraverseDecl(d);
    }
    bool VisitFunctionDecl(cl::FunctionDecl* fn) override {
        if (fn->doesThisDeclarationHaveABody() && fn->getBody() != nullptr && !cl::isLambdaCallOperator(fn)) found.push_back(fn);
        return true;
    }
    bool VisitLambdaExpr(cl::LambdaExpr* e) override {
        if (const cl::CXXMethodDecl* call { e->getCallOperator() }; call != nullptr && call->getBody() != nullptr) found.push_back(call);
        return true;
    }

    std::vector<const cl::FunctionDecl*> found;

private:
    bool in_main(cl::SourceLocation loc) const { return loc.isValid() && sm_.getFileID(sm_.getFileLoc(loc)) == main_; }

    const cl::SourceManager& sm_;
    cl::FileID main_;
};

} // namespace

std::vector<fact::Flow> flows_of(cl::ASTContext& ctx) {
    Finder finder { ctx.getSourceManager() };
    finder.TraverseDecl(ctx.getTranslationUnitDecl());
    FlowBuilder builder { ctx };
    std::vector<fact::Flow> flows;
    flows.reserve(finder.found.size());
    for (const cl::FunctionDecl* fn : finder.found) flows.push_back(builder.build(fn));
    return flows;
}

} // namespace mcxx::clang_backend
