.section INTERRUPT_VECTOR, "x"
.global _VectorTable
_VectorTable:
  B reset_handler /* Reset */
  B . /* Undefined */
  B . /* SWI */
  B . /* Prefetch Abort */
  B . /* Data Abort */
  B . /* reserved */
  B . /* IRQ */
  B . /* FIQ */
 
reset_handler:
  LDR sp, =stack_top
  BL _start
  B .
  