// mcxx.frontend:lookup -- what the names a file writes name, by MC++'s own front end: C++'s name lookup
// ([basic.lookup]) over the declarations F1 reads. A function's locals by their blocks, its
// parameters, a class's members (a member function's body sees them all, and its bases'), the
// enclosing namespaces (what is declared before the name), using-directives and using-declarations,
// and what the file's imports bring in (M2.2); `a::b::c` left to right; a member access (`x.m`,
// `p->m`, `this->m`) through the class the object's declared type names.
//
// What F1 cannot resolve -- a member of an object whose type is deduced, a dependent name, a name only
// argument-dependent lookup finds -- is left out, as the Clang backend leaves a dependent one out:
// a reference given is one F1 is sure of. Checked against the Clang backend's references by
// tools/checks/refsdiff.py (M2.1).
export module mcxx.frontend:lookup;

import std;
import mcxx.msa;
import :lex;
import :preprocess;
import :syntax;
import :types;

export namespace mcxx::frontend {

// Each declaration's namespace (container) and qualified name, as MC3 names them (§4.2): inline
// namespaces left out, an unnamed one `(anonymous namespace)`, what a function declares by its name
// alone, an unscoped enumerator in its enum's enclosing scope.
struct Names {
    std::vector<std::string> container;
    std::vector<std::string> qualified;
};
Names names_of(const Syntax& syntax);

struct Reference {
    msa::Range range;                  // the name as written
    std::string name;
    std::string target;                // the qualified name of what it names (a local's: its name)
    msa::Kind kind { msa::Kind::unknown };
    std::int32_t declaration { -1 };   // the file's declaration it names (Syntax::declarations), or -1: one it imports
    // False for a name F1 cannot resolve and will not guess (A2.2.3): a member of an object whose type
    // it cannot tell -- one a function template deduces (`auto`), or an initializer it cannot type.
    // Its target is empty; a service asks the Clang backend instead. `why`: "deduced" or "unknown".
    bool certain { true };
    std::string why;
};

// What the file can name but does not declare: the declarations its imports bring in, by qualified
// name and kind (and a variable's type, for member access), from the modules' MC2 interfaces (M2.2).
struct Imported {
    std::vector<msa::fact::Declaration> declarations;
};

// Every name the file writes (outside macro expansions) that F1 resolves, in order; and, not certain,
// each member name whose object's type F1 cannot tell.
std::vector<Reference> references(const Syntax& syntax, const Imported& imported = {});

} // namespace mcxx::frontend
