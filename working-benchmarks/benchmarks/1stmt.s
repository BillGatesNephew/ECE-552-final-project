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
	.align 8
LC2:
	.ascii "input operand constraint contains `+'\0"
	.align 8
LC3:
	.ascii "output operand constraint lacks `='\0"
	.align 8
LC4:
	.ascii "more than %d operands in `asm'\0"
	.align 8
LC5:
	.ascii "\0"
	.align 8
LC6:
	.ascii "hard register `%s' listed as input operand to `asm'\0"
	.align 8
LC7:
	.ascii "input operand constraint contains `%c'\0"
	.align 8
LC8:
	.ascii "unknown register name `%s' in `asm'\0"
	.align 4
	.global _expand_asm_operands
	.proc	020
_expand_asm_operands:
	!#PROLOGUE# 0
	save %sp,-168,%sp
	!#PROLOGUE# 1
	st %i0,[%fp-20]
	st %i1,[%fp-28]
	st %i2,[%fp-36]
	st %i3,[%fp-44]
	st %i4,[%fp-52]
	ld [%fp+92],%i4
	call _list_length,0
	ld [%fp-36],%o0
	mov %o0,%l7
	call _list_length,0
	ld [%fp-28],%o0
	mov %o0,%l5
	call _list_length,0
	ld [%fp-44],%o0
	mov %o0,%i2
	sll %l5,2,%o0
	add %o0,122,%o0
	and %o0,-8,%o0
	sub %sp,%o0,%sp
	add %sp,112,%l6
	sethi %hi(_last_expr_type),%o0
	st %g0,[%o0+%lo(_last_expr_type)]
	ld [%fp-28],%l2
	cmp %l2,0
	be L107
	mov 0,%l3
	sethi %hi(_error_mark_node),%i1
	sethi %hi(_save_expr_regs),%i0
L118:
	ld [%l2+20],%l4
	ld [%l4+8],%o1
	ld [%i1+%lo(_error_mark_node)],%o0
	cmp %o1,%o0
	be L105
	mov 0,%o3
	ld [%l2+16],%o0
	mov %o0,%o1
	ld [%o0+20],%o0
	cmp %o3,%o0
	be L111 L111
	mov 0,%o2
	ld [%o1+24],%o0
L164:
	ldsb [%o0+%o2],%o0
	cmp %o0,43
	be L160
	sethi %hi(LC2),%o0
	ld [%l2+16],%o0
	ld [%o0+24],%o0
	ldsb [%o0+%o2],%o0
	cmp %o0,61
	be,a L112
	mov 1,%o3
L112:
	add %o2,1,%o2
	ld [%l2+16],%o1
	ld [%o1+20],%o0
	cmp %o2,%o0
	bl,a L164
	ld [%o1+24],%o0
L111:
	cmp %o3,0
	be L161
	sethi %hi(LC3),%o0
	ldub [%l4+12],%o1
	add %o1,-43,%o0
	and %o0,0xff,%o0
	cmp %o0,1
	be % L117
	cmp %o1,49
	be,a L165
	ld [%l2+20],%o0
	ld [%l4+8],%o0
	call _gen_reg_rtx,0
	ldub [%o0+28],%o0
	mov %o0,%l0
	mov 112,%o0
	mov %l4,%o1
	call _build_nt,0
	mov %l0,%o2
	mov %o0,%l1
	mov 2,%o0
	mov 0,%o1
	mov %l0,%o2
	call _gen_rtx,0
	ld [%i0+%lo(_save_expr_regs)],%o3
	st %o0,[%i0+%lo(_save_expr_regs)]
	st %l1,[%l2+20]
	ld [%l4+8],%o0
	st %o0,[%l1+8]
L117:
	ld [%l2+20],%o0
L165:
	mov 0,%o1
	mov 0,%o2
	call _expand_expr,0
	mov 0,%o3
	sll %l3,2,%o1
	st %o0,[%l6+%o1]
	ld [%l2+4],%l2
	cmp %l2,0
	be,a L118 L118
	add %l3,1,%l3
L107:
	add %l7,%l5,%o0
	cmp %o0,5
	be,a L119 L119 L119
	nop
	sethi %hi(LC4),%o0
	or %o0,%lo(LC4),%o0
	call _error,0
	mov 5,%o1
	b,a L105
L119:
	call _rtvec_alloc,0
	mov %l7,%o0
	mov %o0,%i1
	call _rtvec_alloc,0
	mov %l7,%o0
	mov %o0,%i0
	st %i0,[%sp+92]
	st %i5,[%sp+96]
	st %i4,[%sp+100]
	mov 22,%o0
	mov 0,%o1
	ld [%fp-20],%g2
	ld [%g2+24],%o2
	sethi %hi(LC5),%o3
	or %o3,%lo(LC5),%o3
	mov 0,%o4
	call _gen_rtx,0
	mov %i1,%o5
	mov %o0,%l4
	ld [%fp-52],%g2
	and %g2,1,%o1
	sll %o1,4,%o1
	ld [%l4],%o0
	and %o0,-17,%o0
	or %o0,%o1,%o0
	st %o0,[%l4]
	ld [%fp-36],%l2
	cmp %l2,0
	be L121
	mov 0,%l3
	sethi %hi(_error_mark_node),%i3
	sethi %hi(LC7),%l1
L131:
	ld [%l2+20],%o2
	ld [%o2+8],%o1
	ld [%i3+%lo(_error_mark_node)],%o0
	cmp %o1,%o0
	be L105
	nop
	ld [%l2+16],%o0
	cmp %o0,0
	be L162
	mov %o0,%o1
	mov 0,%o2
	ld [%o0+20],%o0
	cmp %o2,%o0
	be L166,a L166
	ld [%l2+20],%o0
	ld [%o1+24],%o0
L167:
	ldsb [%o0+%o2],%o0
	cmp %o0,61
	be L129
	cmp %o0,43
	be L127,a L127
	add %o2,1,%o2
L129:
	ld [%l2+16],%o0
	ld [%o0+24],%o1
	or %l1,%lo(LC7),%o0
	call _error,0
	ldsb [%o1+%o2],%o1
	b,a L105
L127:
	ld [%l2+16],%o1
	ld [%o1+20],%o0
	cmp %o2,%o0
	bl,a L167
	ld [%o1+24],%o0
	ld [%l2+20],%o0
L166:
	mov 0,%o1
	mov 0,%o2
	call _expand_expr,0
	mov 0,%o3
	ld [%l4+16],%o1
	sll %l3,2,%l0
	add %o1,%l0,%o1
	st %o0,[%o1+4]
	ld [%l2+20],%o0
	ld [%o0+8],%o1
	ld [%l2+16],%o2
	mov 21,%o0
	ldub [%o1+28],%o1
	call _gen_rtx,0
	ld [%o2+24],%o2
	ld [%l4+20],%o1
	add %o1,%l0,%o1
	st %o0,[%o1+4]
	ld [%l2+4],%l2
	cmp %l2,0
	bl,a L131 L131
	add %l3,1,%l3
L121:
	mov 0,%l3
	cmp %l3,%l7
	bl,a L168 L168 L168
	cmp %l3,%l5
L135:
	ld [%l4+16],%o0
	sll %l3,2,%l0
	add %o0,%l0,%o0
	ld [%o0+4],%o0
	call _protect_from_queue,0
	mov 0,%o1
	ld [%l4+16],%o1
	add %o1,%l0,%o1
	add %l3,1,%l3
	cmp %l3,%l7
	bl L135
	st %o0,[%o1+4]
	mov 0,%l3
	cmp %l3,%l5
L168:
	bl L169 L169
	cmp %l5,1
L139:
	sll %l3,2,%l0
	ld [%l6+%l0],%o0
	call _protect_from_queue,0
	mov 1,%o1
	add %l3,1,%l3
	cmp %l3,%l5
	bl L139
	st %o0,[%l6+%l0]
	cmp %l5,1
L169:
	bl L170 L170
	cmp %l5,0
	cmp %i2,0
	bl L170 L170 L170
	cmp %l5,0
	ld [%fp-28],%g2
	ld [%g2+16],%o0
	ld [%o0+24],%o0
	st %o0,[%l4+8]
	mov 25,%o0
	mov 0,%o1
	ld [%l6],%o2
	call _gen_rtx,0
	mov %l4,%o3
	call _emit_insn,0
	nop
	b L171
	sethi %hi(_last_expr_type),%o0
L170:
	bl L172 L172 L172 L172
	mov %l5,%o0
	cmp %i2,0
	bl L173 L173 L173 L173 L173
	cmp %o0,0
	b,a L151
L160:
	call _error,0
	or %o0,%lo(LC2),%o0
	b,a L105
L161:
	call _error,0
	or %o0,%lo(LC3),%o0
	b,a L105
L162:
	sethi %hi(LC6),%o0
	or %o0,%lo(LC6),%o0
	call _error,0
	ld [%o2+24],%o1
	b,a L105
L163:
	sethi %hi(LC8),%o0
	or %o0,%lo(LC8),%o0
	call _error,0
	mov %l1,%o1
	b,a L105
L172:
	cmp %o0,0
L173:
	bl L144 L144 L144 L144 L144 L144
	mov %l4,%l7
	mov 1,%o0
L144:
	call _rtvec_alloc,0
	add %o0,%i2,%o0
	mov %o0,%o2
	mov 20,%o0
	call _gen_rtx,0
	mov 0,%o1
	mov %o0,%l4
	ld [%fp-28],%l2
	cmp %l2,0
	be L146
	mov 0,%l3
	ld [%fp-52],%g2
	and %g2,1,%o0
	sll %o0,4,%l1
L148:
	sll %l3,2,%l0
	ld [%l2+16],%o3
	st %i0,[%sp+92]
	st %i5,[%sp+96]
	st %i4,[%sp+100]
	mov 22,%o0
	mov 0,%o1
	ld [%fp-20],%g2
	ld [%g2+24],%o2
	ld [%o3+24],%o3
	mov %l3,%o4
	call _gen_rtx,0
	mov %i1,%o5
	mov %o0,%o3
	mov 25,%o0
	mov 0,%o1
	call _gen_rtx,0
	ld [%l6+%l0],%o2
	ld [%l4+4],%o1
	add %o1,%l0,%o1
	st %o0,[%o1+4]
	ld [%l4+4],%o0
	add %o0,%l0,%o0
	ld [%o0+4],%o0
	ld [%o0+8],%o1
	ld [%o1],%o0
	and %o0,-17,%o0
	or %o0,%l1,%o0
	st %o0,[%o1]
	ld [%l2+4],%l2
	cmp %l2,0
	be L148 L148
	add %l3,1,%l3
L146:
	cmp %l3,0
	be L174 L174 L174
	ld [%fp-44],%l2
	ld [%l4+4],%o0
	st %l7,[%o0+4]
	mov 1,%l3
	ld [%fp-44],%l2
L174:
	cmp %l2,0
	be L151
	sethi %hi(_reg_names),%o0
	or %o0,%lo(_reg_names),%l5
L159:
	ld [%l2+20],%o0
	ld [%o0+24],%l1
	mov 0,%l0
	sll %l0,2,%o1
L176:
	mov %l1,%o0
	call _strcmp,0
	ld [%o1+%l5],%o1
	cmp %o0,0
	be L175
	cmp %l0,56
	add %l0,1,%l0
	cmp %l0,55
	be L176,a L176
	sll %l0,2,%o1
	cmp %l0,56
L175:
	be L163
	mov 34,%o0
	mov 1,%o1
	call _gen_rtx,0
	mov %l0,%o2
	mov %o0,%o2
	mov 27,%o0
	call _gen_rtx,0
	mov 0,%o1
	ld [%l4+4],%o1
	sll %l3,2,%o2
	add %o1,%o2,%o1
	st %o0,[%o1+4]
	ld [%l2+4],%l2
	cmp %l2,0
	be L159 L159
	add %l3,1,%l3
L151:
	call _emit_insn,0
	mov %l4,%o0
	sethi %hi(_last_expr_type),%o0
L171:
	st %g0,[%o0+%lo(_last_expr_type)]
L105:
	ret
	restore
	.align 8
LC9:
	.ascii "statement with no effect\0"
	.align 4
	.global _expand_expr_stmt
	.proc	020
_expand_expr_stmt:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	sethi %hi(_extra_warnings),%o0
	ld [%o0+%lo(_extra_warnings)],%o0
	cmp %o0,0
	be L178
	sethi %hi(_expr_stmts_for_value),%o0
	ld [%o0+%lo(_expr_stmts_for_value)],%o0
	cmp %o0,0
	be L182,a L182
	ld [%i0+8],%o0
	ld [%i0+12],%o1
	sethi %hi(1048576),%o0
	andcc %o1,%o0,%g0
	be L182,a L182,a L182
	ld [%i0+8],%o0
	sethi %hi(_error_mark_node),%o0
	ld [%o0+%lo(_error_mark_node)],%o0
	cmp %i0,%o0
	be L178
	sethi %hi(_emit_filename),%o0
	sethi %hi(_emit_lineno),%o1
	ld [%o0+%lo(_emit_filename)],%o0
	ld [%o1+%lo(_emit_lineno)],%o1
	sethi %hi(LC9),%o2
	call _warning_with_file_and_line,0
	or %o2,%lo(LC9),%o2
L178:
	ld [%i0+8],%o0
L182:
	sethi %hi(_last_expr_type),%o1
	st %o0,[%o1+%lo(_last_expr_type)]
	sethi %hi(_flag_syntax_only),%o0
	ld [%o0+%lo(_flag_syntax_only)],%o0
	cmp %o0,0
	be L179 L179
	sethi %hi(_expr_stmts_for_value),%o0
	ld [%o0+%lo(_expr_stmts_for_value)],%o0
	cmp %o0,0
	be L181 L181 L181
	mov 0,%o1
	sethi %hi(_const0_rtx),%o0
	ld [%o0+%lo(_const0_rtx)],%o1
L181:
	mov %i0,%o0
	mov 0,%o2
	call _expand_expr,0
	mov 0,%o3
	sethi %hi(_last_expr_value),%o1
	st %o0,[%o1+%lo(_last_expr_value)]
L179:
	call _emit_queue,0
	nop
	ret
	restore
	.align 4
	.global _clear_last_expr
	.proc	020
_clear_last_expr:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	sethi %hi(_last_expr_type),%g2
	st %g0,[%g2+%lo(_last_expr_type)]
	ret
	restore
	.align 4
	.global _expand_start_stmt_expr
	.proc	0111
_expand_start_stmt_expr:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	call _start_sequence,0
	nop
	call _suspend_momentary,0
	mov %o0,%l1
	mov %o0,%l0
	call _make_node,0
	mov 113,%o0
	mov %o0,%i0
	call _resume_momentary,0
	mov %l0,%o0
	st %l1,[%i0+24]
	sethi %hi(_expr_stmts_for_value),%o1
	ld [%o1+%lo(_expr_stmts_for_value)],%o0
	add %o0,1,%o0
	st %o0,[%o1+%lo(_expr_stmts_for_value)]
	ret
	restore
	.align 4
	.global _expand_end_stmt_expr
	.proc	0111
_expand_end_stmt_expr:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	call _do_pending_stack_adjust,0
	ld [%i0+24],%l3
	sethi %hi(_last_expr_type),%o1
	ld [%o1+%lo(_last_expr_type)],%o0
	cmp %o0,0
	be L186 L186 L186 L186
	sethi %hi(_last_expr_type),%l2
	sethi %hi(_void_type_node),%o0
	ld [%o0+%lo(_void_type_node)],%o0
	st %o0,[%o1+%lo(_last_expr_type)]
	sethi %hi(_const0_rtx),%o0
	ld [%o0+%lo(_const0_rtx)],%o0
	sethi %hi(_last_expr_value),%o1
	st %o0,[%o1+%lo(_last_expr_value)]
L186:
	ld [%l2+%lo(_last_expr_type)],%o0
	st %o0,[%i0+8]
	sethi %hi(_last_expr_value),%l1
	ld [%l1+%lo(_last_expr_value)],%o0
	call _get_insns,0
	st %o0,[%i0+24]
	st %o0,[%i0+20]
	sethi %hi(_rtl_expr_chain),%l0
	mov 0,%o0
	mov %i0,%o1
	call _tree_cons,0
	ld [%l0+%lo(_rtl_expr_chain)],%o2
	st %o0,[%l0+%lo(_rtl_expr_chain)]
	call _end_sequence,0
	mov %l3,%o0
	ld [%i0+12],%o0
	sethi %hi(1048576),%o1
	or %o0,%o1,%o0
	st %o0,[%i0+12]
	call _volatile_refs_p,0
	ld [%l1+%lo(_last_expr_value)],%o0
	and %o0,1,%o0
	sll %o0,12,%o0
	ld [%i0+12],%o2
	sethi %hi(4096),%o1
	andn %o2,%o1,%o1
	or %o1,%o0,%o1
	st %o1,[%i0+12]
	st %g0,[%l2+%lo(_last_expr_type)]
	sethi %hi(_expr_stmts_for_value),%o1
	ld [%o1+%lo(_expr_stmts_for_value)],%o0
	add %o0,-1,%o0
	st %o0,[%o1+%lo(_expr_stmts_for_value)]
	ret
	restore
	.align 4
	.global _expand_start_cond
	.proc	020
_expand_start_cond:
	!#PROLOGUE# 0
	save %sp,-112,%sp
	!#PROLOGUE# 1
	call _xmalloc,0
	mov 40,%o0
	mov %o0,%l0
	sethi %hi(_cond_stack),%o0
	ld [%o0+%lo(_cond_stack)],%o0
	st %o0,[%l0+4]
	sethi %hi(_nesting_stack),%o0
	ld [%o0+%lo(_nesting_stack)],%o0
	st %o0,[%l0]
	sethi %hi(_nesting_depth),%o1
	ld [%o1+%lo(_nesting_depth)],%o0
	add %o0,1,%o0
	st %o0,[%o1+%lo(_nesting_depth)]
	st %o0,[%l0+8]
	call _gen_label_rtx,0
	st %g0,[%l0+20]
	cmp %i1,0
	be L188
	st %o0,[%l0+16]
	b L189
	st %o0,[%l0+12]
L188:
	st %g0,[%l0+12]
L189:
	sethi %hi(_cond_stack),%o0
	st %l0,[%o0+%lo(_cond_stack)]
	sethi %hi(_nesting_stack),%o0
	st %l0,[%o0+%lo(_nesting_stack)]
	mov %i0,%o0
	ld [%l0+16],%o1
	call _do_jump,0
	mov 0,%o2
	ret
	restore
