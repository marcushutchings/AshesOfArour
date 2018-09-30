
.data

; display buffer base pointer

disp_mh: .word 0xffee


.text

; void set_display_buffer(uint8_t *p)

.global set_display_buffer
set_display_buffer:
	ldi r30,lo8(disp_mh)
	ldi r31,hi8(disp_mh)
	st Z+,r24
	st Z,r25
	ret

; uint8_t* get_display_buffer(void)

.global get_display_buffer
get_display_buffer:
	ldi r30,lo8(disp_mh)
	ldi r31,hi8(disp_mh)
	ld r24,Z+
	ld r25,Z
	ret


; draw vertical

; uint8_t x
; uint16_t line_height
; *struct wall_sect {
;	texture[2];
;
; }

	;r18 = height in pixels
	;r19 =
	;r21 =
	;r20 = arg2 x

	;r22,r23 = arg0 (line height 8.8) (lo,hi)
	;r24,r25 = arg1 (texture *) (lo,hi)


	;r1 = 0
	;r0 = (sign) 0 = both pos, 1 = 1 neg, 2= both neg

.extern div16

#define pbuffer r28
#define draw_start r31
#define draw_end r30

.global draw_vertical
draw_vertical:
	push r28
	push r29
	push r16
	push r17
	push r2
	push r3
	push r4
	push r5
	push r6
	push r7
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	;r18 = height in pixels
	;r2 = draw_texture_y_offset_steps
	;r31 = draw start
	;r30 = draw_end

	cpi r20, 84
	brlo cont
	ldi r24,0xff
	ldi r25,0xff
	rjmp finish

cont:
	; r26,r27 = buffer reference
	; r28,r29 = texture reference
	movw r28,r24

; load screen buffer
	ldi r30,lo8(disp_mh)
	ldi r31,hi8(disp_mh)
	ld r26,Z+
	ld r27,Z
	ldi r21,6
	mul r20,r21
	add r26,r0
	adc r27,r1
	eor r1,r1
	eor r2,r2

	; draw_start = (-(line_height>>1)) + 24;
	; draw_end = (line_height>>1) + 24;
	movw r30,r22
	add r30,r30
	adc r31,r1
	dec r31
	ror r31
	mov r30,r31
	adc r31,r1
	neg r31
	subi r31,-24

	;' if draw_start < 0
	brpl draw_end_limit
	; draw_texture_y_offset_steps = 0 - draw_start;
	sub r2,r31
	; draw_start = 0;
	eor r31,r31

draw_end_limit:
	subi r30,-24
	cpi r30, 48
	brlo line_draw_remaining
	ldi r30,47

	;r29, line_draw_remaining
line_draw_remaining:
	mov r4,r30
	sub r4,r31
	inc r4
	eor r5,r5

	;r31 = draw_start
	; write ceiling
	ldi r16,0xff
	cpi r31, 8
	brlo ceil_end

	cpi r31, 48
	brlo write_ceil
	ldi r24,0xff
	ldi r25,0xff
	rjmp finish

write_ceil:
	st X+,r16
	subi r31, 8
	cpi r31, 8
	brsh write_ceil

	# r16 = scaled texture
ceil_end:

	# setup texture source
	# r24 = offset from drawing
	# r16,r17 = ctexture y inc
	# r24,r25 = curr texture y


	ldi r17,1
; floor pattern for the greyscale
	and r17,r20
	; r20 not needed now

	# Draw intial byte
	# r31 = remaining ceiling
	movw r22,r4
	ldi r25,16 ; textures are fixed to 16 bit heights
	ldi r24,0

	;ldi r22,2
	;ldi r23,0

	call div16

next:
	;movw r24,r4
	;mov r25,r30
	;mov r24,r31
	; draw ceiling



			; r20 = i, r21 = scaled texture, r22,r23 = texture cur y
			;* r18 = sample mask, r19 = texture sample, r24,r25 = texture step y
			;* r28 = texture
			;*
			;*	loop:
			; *		lsr r21
			; *		cpse r19,r1
			; *		ori r21,0x80
			; *		add r22,r24
			; *		adc r23,r25
			; *		breq skip
			; *	move_texture_sample:
			; *		lsl r18
			; *		dec r23
			; *		brne move_texture_sample
			; *		mov r19,r28
			; *		and r19,r18
			; *	skip:
			; * 		dec r20
			; * 		brne loop
			;*/



finish:
	;eor r1,r1
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop r7
	pop r6
	pop r5
	pop r4
	pop r3
	pop r2
	pop r17
	pop r16
	pop r29
	pop r28
	ret
