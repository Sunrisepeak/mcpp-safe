// mcxx.frontend:declared -- what a declaration's type is beyond its text: MC3's `templates` and
// `pointer` as Clang has them, the canonical type's (aliases followed, a specialization's defaulted
// arguments filled in from its template's parameters -- MC3 0.6.0 for an imported one; a dependent
// type as written), with :lookup's name lookup and the imports' interfaces (A2.1.2). What F1 cannot
// tell -- a deduced type (`auto`, a class template's arguments deduced), a name it cannot resolve, a
// member type of a specialization -- it says, and does not guess (A2.2.3). Checked against the Clang
// backend by tools/checks/declsdiff.py.
export module mcxx.frontend:declared;

import std;
import mcxx.msa;
import :syntax;
import :lookup;

export namespace mcxx::frontend {

struct DeclaredType {
    std::string type;                   // MC3's `type`: as written (Clang's TypePrinter), a function's return type
    std::vector<std::string> templates;
    bool pointer { false };
    // A class's direct bases as the scopes they name (MC3 0.5.0's `bases`), and a class template's or
    // an alias template's parameters as MC3 0.6.0 writes them ("class T", "std::size_t N"): what an
    // importer reading this file as its module's interface needs (a member it inherits, a member's
    // type in a specialization).
    std::vector<std::string> bases;
    std::vector<std::string> template_parameters;
    // A function's parameters' types as MC3 0.7.0 writes them ("int =": with a default argument);
    // none when F1 cannot print one of them.
    std::optional<std::vector<std::string>> parameters;
    // Which of them F1 knows. `why` for one it does not: "deduced" (a placeholder type, a class
    // template's arguments deduced), "unknown" (a name it cannot resolve, a template whose parameters
    // it cannot see, a member type of a specialization).
    bool type_certain { true };
    bool templates_certain { true };
    bool pointer_certain { true };
    std::string why;
};

// One per declaration of the file (Syntax::declarations' order).
std::vector<DeclaredType> declared_types(const Syntax& syntax, const Imported& imported = {});

// Where MC3 has a declaration's name: its selection range, an alias template's at its name (its
// outline is at its `using`, as Clang's is).
msa::Range fact_name(const Syntax& syntax, const Declaration& d);

} // namespace mcxx::frontend
