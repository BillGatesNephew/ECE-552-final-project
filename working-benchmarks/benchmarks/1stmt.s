gcc2_compiled.:
___gnu_compiled_c:
.text
	.align 4
	.proc	0110
_label_rtx:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	ldub [%i0+12],%o0
	cmp %o0,40
	be,a L2
	ld [%i0+64],%o0
	call _abort,0
	nop
L2:
	cmp %o0,0
	be,a L4,a L4
	ld [%i0+64],%i0
	call _gen_label_rtx,0
	nop
	st %o0,[%i0+64]
	mov %o0,%i0
L4:
	ret
	restore
	.align 4
	.global _emit_jump
	.proc	020
_emit_jump:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	call _do_pending_stack_adjust,0
	nop
	call _gen_jump,0
	mov %i0,%o0
	call _emit_jump_insn,0
	nop
	call _emit_barrier,0
	nop
	ret
	restore
	.align 4
	.global _expand_label
	.proc	020
_expand_label:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	call _do_pending_stack_adjust,0
	sethi %hi(_stack_block_stack),%l0
	call _label_rtx,0
	mov %i0,%o0
	call _emit_label,0
	nop
	ld [%l0+%lo(_stack_block_stack)],%o0
	cmp %o0,0
	be L7
	nop
	call _oballoc,0
	mov 8,%o0
	ld [%l0+%lo(_stack_block_stack)],%o2
	ld [%o2+36],%o1
	st %o1,[%o0]
	st %o0,[%o2+36]
	st %i0,[%o0+4]
L7:
	ret
	restore
	.align 4
	.global _expand_goto
	.proc	020
_expand_goto:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	call _label_rtx,0
	mov %i0,%o0
	mov %o0,%o1
	mov %i0,%o0
	call _expand_goto_internal,0
	mov 0,%o2
	ret
	restore
	.align 8
LC0:
	.ascii "jump to `%s' invalidly jumps into binding contour\0"
	.align 4
	.proc	020
_expand_goto_internal:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov %i2,%o2
	lduh [%i1],%o0
	cmp %o0,17
	be L10
	mov 0,%l0
	call _abort,0
	nop
L10:
	ld [%i1+8],%o0
	cmp %o0,0
	be L11
	sethi %hi(_block_stack),%o0
	ld [%o0+%lo(_block_stack)],%i2
	cmp %i2,0
	be L24
	cmp %l0,0
	ld [%i2+20],%o0
L26:
	ld [%o0+4],%o1
	ld [%i1+4],%o0
	cmp %o1,%o0
	bl L24
	cmp %l0,0
	ld [%i2+16],%o0
	cmp %o0,0
	bl L16,a L16
	mov %o0,%l0
L16:
	ld [%i2+28],%o0
	cmp %o0,0
	be,a L25
	ld [%i2+4],%i2
	call _expand_cleanups,0
	mov 0,%o1
	ld [%i2+4],%i2
L25:
	cmp %i2,0
	be,a L26,a L26
	ld [%i2+20],%o0
	cmp %l0,0
L24:
	be L19
	sethi %hi(_stack_pointer_rtx),%o0
	ld [%o0+%lo(_stack_pointer_rtx)],%o0
	call _emit_move_insn,0
	mov %l0,%o1
L19:
	cmp %i0,0
	be L21
	sethi %hi(524288),%o0
	ld [%i0+12],%o1
	andcc %o1,%o0,%g0
	be L21
	sethi %hi(LC0),%o0
	ld [%i0+36],%o1
	or %o0,%lo(LC0),%o0
	call _error,0
	ld [%o1+20],%o1
	b,a L21
L11:
	mov %i0,%o0
	call _expand_fixup,0
	mov %i1,%o1
	cmp %o0,0
	be L21 L21
	cmp %i0,0
	be L21
	sethi %hi(16384),%o1
	ld [%i0+12],%o0
	or %o0,%o1,%o0
	st %o0,[%i0+12]
L21:
	call _emit_jump,0
	mov %i1,%o0
	ret
	restore
	.align 4
	.proc	04
_expand_fixup:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	sethi %hi(_cond_stack),%o0
	ld [%o0+%lo(_cond_stack)],%o1
	cmp %o1,0
	be L28
	mov %i0,%l1
	ld [%o1+16],%o0
	cmp %i1,%o0
	be L58
	sethi %hi(_cond_stack),%o0
	ld [%o1+20],%o0
	cmp %i1,%o0
	be L59 L59
	sethi %hi(_loop_stack),%o0
	sethi %hi(_cond_stack),%o0
L58:
	b L30
	ld [%o0+%lo(_cond_stack)],%o2
L28:
	sethi %hi(_loop_stack),%o0
L59:
	ld [%o0+%lo(_loop_stack)],%o1
	cmp %o1,0
	be L30
	mov 0,%o2
	ld [%o1+16],%o0
	cmp %i1,%o0
	be L60
	sethi %hi(_loop_stack),%o0
	ld [%o1+20],%o0
	cmp %i1,%o0
	be L60
	sethi %hi(_loop_stack),%o0
	ld [%o1+24],%o0
	cmp %i1,%o0
	be L61 L61
	cmp %o2,0
	sethi %hi(_loop_stack),%o0
L60:
	ld [%o0+%lo(_loop_stack)],%o2
L30:
	cmp %o2,0
L61:
	be L34
	sethi %hi(_block_stack),%o0
	ld [%o2],%o1
	ld [%o0+%lo(_block_stack)],%i0
L37:
	cmp %o1,0
	be L36
	cmp %o1,%i0
	be L37,a L37
	ld [%o1],%o1
L36:
	cmp %o1,0
	be L38
	sethi %hi(_block_stack),%o0
	b L56
	mov 0,%i0
L38:
	ld [%o0+%lo(_block_stack)],%o0
	mov %o0,%i0
	cmp %i0,%o2
	be L40
	ld [%o0+4],%o1
	cmp %i0,%o1
L62:
	be,a L41
	ld [%i0+4],%o1
L41:
	ld [%i0],%i0
	cmp %i0,%o2
	be,a L62 L62
	cmp %i0,%o1
L40:
	mov %o1,%o2
L34:
	sethi %hi(_block_stack),%o0
	ld [%o0+%lo(_block_stack)],%i0
	cmp %i0,%o2
	be L65
	subcc %g0,%i0,%g0
	ld [%i0+16],%o0
L64:
	cmp %o0,0
	be L63 L63
	cmp %i0,%o2
	ld [%i0+28],%o0
	cmp %o0,0
	be L63 L63 L63
	cmp %i0,%o2
	ld [%i0+4],%i0
	cmp %i0,%o2
	be L64 L64 L64,a L64
	ld [%i0+16],%o0
	cmp %i0,%o2
L63:
	be L65
	subcc %g0,%i0,%g0
	call _oballoc,0
	mov 24,%o0
	call _do_pending_stack_adjust,0
	mov %o0,%l0
	cmp %i2,0
	be L51
	nop
	b L52
	st %i2,[%l0+4]
L51:
	call _get_last_insn,0
	nop
	st %o0,[%l0+4]
L52:
	st %l1,[%l0+8]
	st %i1,[%l0+12]
	st %g0,[%l0+16]
	ld [%i0+32],%o0
	cmp %o0,0
	be L66
	nop L66
	mov 0,%o0
	ld [%i0+28],%o0
	cmp %o0,0
	be L53
	mov 0,%o0
L66:
	ld [%i0+28],%o1
	call _tree_cons,0
	ld [%i0+32],%o2
	b L67
	st %o0,[%l0+20]
L53:
	st %o0,[%l0+20]
L67:
	sethi %hi(_goto_fixup_chain),%o1
	ld [%o1+%lo(_goto_fixup_chain)],%o0
	st %o0,[%l0]
	st %l0,[%o1+%lo(_goto_fixup_chain)]
	subcc %g0,%i0,%g0
L65:
	addx %g0,0,%i0
L56:
	ret
	restore
	.align 8
LC1:
	.ascii "label `%s' used before containing binding contour\0"
	.align 4
	.proc	020
_fixup_gotos:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	sethi %hi(_goto_fixup_chain),%o0
	ld [%o0+%lo(_goto_fixup_chain)],%l1
	cmp %l1,0
	be L70
	mov 0,%o1
	sethi %hi(16384),%l2
	ld [%l1+4],%o0
L103:
	cmp %o0,0
	be L72,a L72
	ld [%l1+12],%o0
	cmp %o1,0
	be,a L94
	mov %l1,%o1
	ld [%l1],%o0
	b L71
	st %o0,[%o1]
L72:
	ld [%o0+8],%o0
	cmp %o0,0
	be L75
	cmp %i0,0
	ld [%l1+8],%o0
	cmp %o0,0
	be L76
	cmp %i4,0
	be L95,a L95
	ld [%l1+4],%o0
	cmp %i1,0
	be L95,a L95,a L95
	ld [%l1+4],%o0
	cmp %i2,0
	be,a L96
	ld [%l1+20],%o0
	ld [%l1+4],%o0
L95:
	ld [%i3+4],%o1
	ld [%o0+4],%o0
	cmp %o1,%o0
	be,a L96,a L96
	ld [%l1+20],%o0
	ld [%l1+8],%o1
	ld [%o1+12],%o0
	andcc %o0,%l2,%g0
	be,a L96,a L96,a L96
	ld [%l1+20],%o0
	mov %o1,%o0
	sethi %hi(LC1),%o1
	call _error_with_decl,0
	or %o1,%lo(LC1),%o1
	ld [%l1+8],%o1
	ld [%o1+12],%o0
	or %o0,%l2,%o0
	st %o0,[%o1+12]
L76:
	ld [%l1+20],%o0
L96:
	cmp %o0,0
	be,a L97
	ld [%l1+16],%o1
	mov %o0,%l0
	ld [%l0+12],%o0
L99:
	andcc %o0,%l2,%g0
	be,a L98
	ld [%l0+4],%l0
	ld [%l0+20],%o0
	cmp %o0,0
	be,a L98
	ld [%l0+4],%l0
	call _fixup_cleanups,0
	add %l1,4,%o1
	ld [%l0+4],%l0
L98:
	cmp %l0,0
	be,a L99,a L99
	ld [%l0+12],%o0
	ld [%l1+16],%o1
L97:
	cmp %o1,0
	be L84
	sethi %hi(_stack_pointer_rtx),%o0
	call _gen_move_insn,0
	ld [%o0+%lo(_stack_pointer_rtx)],%o0
	call _emit_insn_after,0
	ld [%l1+4],%o1
L84:
	b L71
	st %g0,[%l1+4]
L75:
	be L94
	mov %l1,%o1
	ld [%l1+20],%o2
	cmp %o2,0
	be L100
	cmp %i1,0
	ld [%o2+4],%o1
L102:
	ld [%i0+32],%o0
	cmp %o1,%o0
	be L101,a L101
	ld [%o2+4],%o2
	ld [%o2+12],%o0
	or %o0,%l2,%o0
	st %o0,[%o2+12]
	ld [%o2+4],%o2
L101:
	cmp %o2,0
	be L102,a L102,a L102
	ld [%o2+4],%o1
	cmp %i1,0
L100:
	be L71,a L71,a L71,a L71
	st %i1,[%l1+16]
L71:
	mov %l1,%o1
L94:
	ld [%l1],%l1
	cmp %l1,0
	be L103,a L103,a L103,a L103,a L103
	ld [%l1+4],%o0
L70:
	ret
	restore
	.align 4
	.global _expand_asm
	.proc	020
_expand_asm:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	mov 21,%o0
	mov 0,%o1
	call _gen_rtx,0
	ld [%i0+24],%o2
	call _emit_insn,0
	nop
	sethi %hi(_last_expr_type),%o0
	st %g0,[%o0+%lo(_last_expr_type)]
	ret
	restore
