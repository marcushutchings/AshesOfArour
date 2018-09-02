
	#r18,r19,r20,r21 = arg1
	#r22,r23,r24,r25 = arg0
	#r22,r23,r24,r25 = out (q)
	#r26,r27,r30,r31 = (r)
	#r1 = 0
	#r0 = (sign) 0 = both pos, 1 = 1 neg, 2= both neg

.global f32_idiv
f32_idiv:
	push r28
	push r29
	push r16
	push r17
	push r2

	ldi r30, 16
	mov r2, r30

	# abs(arg0)
	eor r0, r0
	sbrs r25, 7
	rjmp abs_arg1
	inc r0
	com r25
	com r24
	com r23
	neg r22
	sbci r23, 0xFF
	sbci r24, 0xFF
	sbci r25, 0xFF
	
abs_arg1:
	sbrs r21, 7
	rjmp check_args
	inc r0
	com r21
	com r20
	com r19
	neg r18
	sbci r19, 0xFF
	sbci r20, 0xFF
	sbci r21, 0xFF

check_args:
	cpse r25, r1
	rjmp clz_arg0_b3
	cpse r24, r1
	rjmp clz_arg0_b2
	cp r1, r18
	cpc r1, r19
	cpc r22, r20
	cpc r23, r21
	brcs divisor_too_large

clz_arg0:
	# checks for r25 and r24 have been shortcutted
	cpse r23, r1
	rjmp clz_arg0_b1
	cpse r22, r1
	rjmp clz_arg0_b0
	# dividend and answer are zero
	rjmp finish

divisor_too_large:
result_is_zero:
	eor r22, r22
	eor r23, r23
	movw r24, r22
	rjmp finish

clz_arg0_b0:
	mov r26, r22
	ldi r30, 24
	cpi r26, 0x10
	brlo clz_arg0_half_loop_init
	rjmp clz_arg0_loop
clz_arg0_b1:
	mov r26, r23
	ldi r30, 16
	cpi r26, 0x10
	brlo clz_arg0_half_loop_init
	rjmp clz_arg0_loop
clz_arg0_b2:
	mov r26, r24
	ldi r30, 8
	cpi r26, 0x10
	brlo clz_arg0_half_loop_init
	rjmp clz_arg0_loop
clz_arg0_b3:
	mov r26, r25
	eor r30, r30
	cpi r26, 0x10
	brlo clz_arg0_half_loop_init
	rjmp clz_arg0_loop

clz_arg0_half_loop_init:
	subi r30,-4
clz_arg0_half_loop:
	inc r30
	lsl r26
	brhc clz_arg0_half_loop 
	rjmp clz_arg1

clz_arg0_loop:
	inc r30
	lsl r26
	brcc clz_arg0_loop

clz_arg1:
	cpse r21, r1
	rjmp clz_arg1_b3
	cpse r20, r1
	rjmp clz_arg1_b2
	cpse r19, r1
	rjmp clz_arg1_b1
	cpse r18, r1
	rjmp clz_arg1_b0
	rjmp divide_by_zero

clz_arg1_b0:
	mov r26, r18
	ldi r31, 24
	cpi r26, 0x10
	brlo clz_arg1_half_loop_init
	rjmp clz_arg1_loop
clz_arg1_b1:
	mov r26, r19
	ldi r31, 16
	cpi r26, 0x10
	brlo clz_arg1_half_loop_init
	rjmp clz_arg1_loop
clz_arg1_b2:
	mov r26, r20
	ldi r31, 8
	cpi r26, 0x10
	brlo clz_arg1_half_loop_init
	rjmp clz_arg1_loop
clz_arg1_b3:
	mov r26, r21
	clr r31
	cpi r26, 0x10
	brlo clz_arg1_half_loop_init
	rjmp clz_arg1_loop

clz_arg1_half_loop_init:
	subi r31,-4
clz_arg1_half_loop:
	inc r31
	lsl r26
	brhc clz_arg1_half_loop 
	rjmp clz_arg1_finished

clz_arg1_loop:
	inc r31
	lsl r26
	brcc clz_arg1_loop

clz_arg1_finished:
	sub r31, r30
	brlo align_divisor_right
	breq do_div

align_divisor_left:
	add r2, r31
align_divisor_left_loop:
	lsl r18
	rol r19
	rol r20
	rol r21
	dec r31
	brne align_divisor_left_loop
	rjmp do_div

align_divisor_right:
	add r2, r31
align_divisor_right_loop:
	lsl r22
	rol r23
	rol r24
	rol r25
	inc r31
	brne align_divisor_right_loop

	# push r16,r17,r28,r29
	# (r16,r17) r18,r19,r20,r21 = y
	# (r26,r27) r22,r23,r24,r25 = r
	# r28, r29, r30, r31 = q
do_div:
	eor r16, r16
	eor r17, r17
	movw r26,r16
	movw r28,r16
	movw r30,r16
	# we go through at least once from here (to benefit from dec z-flag setting)
	inc r2

cont_div_loop:
	cp r26, r16
	cpc r27, r17
	cpc r22, r18
	cpc r23, r19
	cpc r24, r20
	cpc r25, r21
	brlo progress_values

add_to_quotiant:
	sub r26, r16
	sbc r27, r17
	sbc r22, r18
	sbc r23, r19
	sbc r24, r20
	sbc r25, r21

	adiw r28, 1
	adc r30, r1
	adc r31, r1

progress_values:
	dec r2
	breq prepare_return
	add r28, r28
	adc r29, r29
	adc r30, r30
	adc r31, r31

	lsr r21
	ror r20
	ror r19
	ror r18
	ror r17
	ror r16
	rjmp cont_div_loop

prepare_return:
	# Check whether the answer should be negative
	sbrs r0, 0
	rjmp set_answer
inv_sign_of_answer:
	com r31
	com r30
	com r29
	neg r28
	sbci r29, 0xFF
	sbci r30, 0xFF
	sbci r31, 0xFF

set_answer:
	movw r22, r28
	movw r24, r30

finish:
	pop r2
	pop r17
	pop r16
	pop r29
	pop r28
	ret

divide_by_zero:
	clr r22
	clr r23
	clr r24
	ldi r25,0x80
	rjmp finish

