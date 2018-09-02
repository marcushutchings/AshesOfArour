
	#r18,r19,r20,r21 = arg1
	#r22,r23,r24,r25 = arg0
	#r22,r23,r24,r25 = out

.global f32_imult
f32_imult: # (66 cycles) +6 -1 [signed mult +5] (just counted 84 cycles)
	# don't a0b3 * a1b3 the result is not kept in answer
	# r30,r31 high answer (off=4)
	# r26,r27 low answer (off=2)

	# The mulsu only works on r16-r23 - so some reg shifting is in order
	push r16

	# a0.b3*a1.b1 (off=4) !!!!
	mov r16, r25
	mulsu r16, r19
	movw r30, r0

	# a0.b2*a1.b0 (off=2)
	mul r24, r18
	movw r26, r0

	# a0.b3*a1.b2 (off=5) !!!! <save on mov r16, r25
	mulsu r16, r20
	add r31, r0
	# a0.b3*a1.b0 (off=3) !!!! <save on mov r16, r25
	mulsu r16, r18
	# -- use for zero for now
	eor r25, r25

	# -- unless the result is negative
	sbrc r1, 7
	dec r25

	add r27, r0
	adc r30, r1
	adc r31, r25

	# must be done in case 0xff was used for sign
	eor r25, r25

	# a0.b2*a1.b3 (off=5) !!!!
	mov r16, r24
	mulsu r21, r16
	add r31, r0

	# a0.b2*a1.b2 (off=4)
	mul r24, r20
	add r30, r0
	adc r31, r1
	# a0.b2*a1.b1 (off=3)
	mul r24, r19
	add r27, r0
	adc r30, r1
	adc r31, r25
	
	# Can now store in high
	# a0.b1*a1.b3 (off=4)
	mulsu r21, r23
	movw r24, r0
	# a0.b1*a1.b1 (off=2)
	mul r23, r19
	add r26, r0
	adc r27, r1
	adc r24, r30
	adc r25, r31

	# a0.b1*a1.b2 (off=3)
	mul r23, r20
	movw r30, r0
	# a0.b1*a1.b0 (off=1)
	mul r23, r18
	# new regster available (r23)
	eor r16, r16
	add r26, r1
	adc r27, r30
	adc r24, r31
	adc r25, r16
	# holds the byte for the rounding
	mov r23, r0

	# a0.b0*a1.b3 (off=3)
	mulsu r21, r22
	movw r30, r0

	# adjust r16 (0) if result is negative
	sbrc r1, 7
	dec r16

	# a0.b0*a1.b1 (off=1)
	mul r22, r19
	add r23, r0
	adc r26, r1
	adc r27, r30
	adc r24, r31
	adc r25, r16

	#eor r16, r16 - restore not needed
	
	# a0.b0*a1.b2 (off=2)
	mul r22, r20
	movw r30, r0
	# a0.b0*a1.b0 (off=0)
	mul r22, r18
	add r23, r1
	adc r26, r30
	adc r27, r31
	# does not clear C
	eor r1, r1
	adc r24, r1
	adc r25, r1

	# Regetably rounding off without embedding any useful data in other adds
	#add r23, r23
	#adc r26, r1
	#adc r27, r1
	#adc r24, r1
	#adc r25, r1

	movw r22, r26
	
	# restore r16
	pop r16
	ret

	#r18,r19,r20,r21 = arg1
	#r22,r23,r24,r25 = arg0
	
	# a0/a1
	#    b0 b1 b2 b3
	# b0 x  x  x  x
	# b1 x  x  x  x
	# b2 x  x  x  x
	# b3 x  x  x  x
