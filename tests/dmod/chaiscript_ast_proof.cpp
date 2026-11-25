#include <chaiscript/chaiscript.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace {

bool contains_compiled_node(const chaiscript::AST_Node& node) {
    if (node.identifier == chaiscript::AST_Node_Type::Compiled) {
        return true;
    }

    for (const auto& child : node.get_children()) {
        if (contains_compiled_node(child.get())) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    using Parser = chaiscript::parser::ChaiScript_Parser<
        chaiscript::eval::Noop_Tracer,
        chaiscript::optimizer::Optimizer<>>;

    chaiscript::ChaiScript_Basic chai(
        chaiscript::Std_Lib::library(),
        std::make_unique<Parser>(),
        {},
        {},
        chaiscript::default_options(),
        true);

    auto ast = chai.parse("var answer = 6 * 7; answer");
    if (contains_compiled_node(*ast)) {
        std::cerr << "Unexpected optimizer-generated AST node found.\n";
        return EXIT_FAILURE;
    }

    const auto result = chaiscript::boxed_cast<int>(chai.eval(*ast));
    if (result != 42) {
        std::cerr << "AST interpreter returned an unexpected value.\n";
        return EXIT_FAILURE;
    }

    std::cout << "ChaiScript no-optimizer AST interpretation proof passed.\n";
    return EXIT_SUCCESS;
}
