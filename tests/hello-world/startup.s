.global _start
.code 16

.syntax unified
_start:
init_stack:
    @@ Initialize the stack pointer
    ldr sp, =stack_top
    blx test_entry
