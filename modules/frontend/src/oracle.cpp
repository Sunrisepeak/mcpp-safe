// The default semantic feedback to the full parse: what the file's own declarations and its imports say a
// name is (name lookup, :lookup), by the kind of the declaration it finds.
module mcxx.frontend;

import std;
import mcxx.msa;
import mcxx.base;
import :resolver;

namespace mcxx::frontend {

namespace {

class LookupOracle final : public NameOracle {
public:
    LookupOracle(const Syntax& syntax, const Imported& imported) : resolver_ { syntax, imported } {}

    NameAnswer classify(const NameQuery& query) override {
        using resolution::Resolver;
        const Resolver::Nature n { resolver_.nature_of(query.name, query.at) };
        if (!n.found) return {};
        NameAnswer answer;
        answer.basis = ast::Basis::lookup;
        switch (n.kind) {
        case msa::Kind::class_: case msa::Kind::struct_: case msa::Kind::union_: case msa::Kind::enum_:
            answer.what = n.template_ ? NameClass::class_template : NameClass::type;
            break;
        case msa::Kind::type_alias: answer.what = n.template_ ? NameClass::alias_template : NameClass::type; break;
        case msa::Kind::concept_: answer.what = NameClass::concept_; break;
        case msa::Kind::namespace_: case msa::Kind::namespace_alias: answer.what = NameClass::namespace_; break;
        case msa::Kind::template_parameter:
            answer.what = n.parameter_form == 0 ? NameClass::type : n.parameter_form == 2 ? NameClass::class_template : NameClass::value;
            break;
        case msa::Kind::function: case msa::Kind::method: case msa::Kind::constructor: case msa::Kind::destructor: case msa::Kind::conversion:
            answer.what = n.template_ ? NameClass::function_template : NameClass::value;
            break;
        case msa::Kind::variable: case msa::Kind::field: case msa::Kind::parameter: case msa::Kind::enumerator:
            answer.what = n.template_ ? NameClass::variable_template : NameClass::value;
            break;
        default: break;
        }
        return answer;
    }

private:
    resolution::Resolver resolver_;
};

} // namespace

std::unique_ptr<NameOracle> lookup_oracle(const Syntax& syntax, const Imported& imported) { return std::make_unique<LookupOracle>(syntax, imported); }

} // namespace mcxx::frontend
