# Combat2CX compile-order fix

Combat2CW reached C compilation and exposed a C declaration-order defect in the CU runtime parity injection: `source_check_combo_go_port()` is injected before the concrete `drone_check_combo_go()` definition. Clang therefore saw an implicit declaration and then a conflicting static declaration.

Combat2CX adds the required forward prototype to the injected helper block. No gameplay logic or callback behavior is changed. A regression test asserts the prototype exists before the wrapper.
