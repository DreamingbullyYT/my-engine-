#include <chaiscript/chaiscript.hpp>

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

struct ExecutionLimitExceeded final : std::runtime_error {
    ExecutionLimitExceeded() : std::runtime_error("DMod execution budget exceeded") {}
};

struct ExecutionBudgetTracer final {
    std::shared_ptr<std::atomic<std::size_t>> steps;
    std::size_t maximum_steps;

    template <typename Node>
    void trace(const chaiscript::detail::Dispatch_State&,
               const chaiscript::eval::AST_Node_Impl<Node>*) {
        if (steps->fetch_add(1, std::memory_order_relaxed) >= maximum_steps) {
            throw ExecutionLimitExceeded();
        }
    }
};

struct Identity_Optimizer final {
    template <typename T>
    auto optimize(chaiscript::eval::AST_Node_Impl_Ptr<T> node) {
        return node;
    }
};

bool contains_compiled_node(const chaiscript::AST_Node& node) {
    if (node.identifier == chaiscript::AST_Node_Type::Compiled) {
        return true;
    }

    for (const auto& child : node.get_children()) {
        if (contains_compiled_node(*child)) {
            return true;
        }
    }
    return false;
}

} // namespace

int main() {
    using BudgetTracer = chaiscript::eval::Tracer<ExecutionBudgetTracer>;
    using Parser = chaiscript::parser::ChaiScript_Parser<
        BudgetTracer,
        Identity_Optimizer>;

    auto steps = std::make_shared<std::atomic<std::size_t>>(0);
    auto parser = std::make_unique<Parser>(
        BudgetTracer(ExecutionBudgetTracer{steps, 1000}),
        Identity_Optimizer{});

    chaiscript::ChaiScript_Basic chai(
        chaiscript::Std_Lib::library(),
        std::move(parser),
        {},
        {},
        {chaiscript::Options::No_Load_Modules,
         chaiscript::Options::No_External_Scripts},
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

    steps->store(0, std::memory_order_relaxed);
    bool limit_enforced = false;
    try {
        (void)chai.eval("while (true) { }");
    } catch (const ExecutionLimitExceeded&) {
        limit_enforced = true;
    }

    if (!limit_enforced) {
        std::cerr << "Execution budget did not stop an infinite script loop.\n";
        return EXIT_FAILURE;
    }

    std::cout << "ChaiScript direct AST and execution-budget proofs passed.\n";
    return EXIT_SUCCESS;
}
