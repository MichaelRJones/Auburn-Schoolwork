#
# Code to evaluate a quadratic
#

	.text
	
main:
	lw $t0, X
	lw $t1, A
	lw $t2, B
	lw $t3, C

	# In this code, the THIRD parameter is the target destination
	# for the calculation. However the FIRST parameter is where 
	# the information should be stored 
	#
	# mul $t0, $t0, $t4  # X**2
	# mul $t1, $t4, $t4  # A*X**2
	# mul $t0, $t2, $t5   # B*X
	# add $t4, $t5, $t4  # A*X**2 + B*X
	# add $t3, $t4, $t4  # A*X**2 + B*X + C

	mul $t4, $t0, $t0  # X**2
	mul $t4, $t4, $t1  # A*X**2
	mul $t5, $t2, $t0   # B*X
	add $t4, $t5, $t4  # A*X**2 + B*X
	add $t4, $t4, $t3  # A*X**2 + B*X + C

	la $a0, ans        # print a string
	li $v0, 4
	syscall

	move $a0, $t4      # now the result
	li $v0, 1
	syscall

	la $a0, nl         # and a newline
	li $v0, 4
	syscall

	li $v0, 10         # g'bye
	syscall

	.data
X:	.word 3
A:	.word 7
B:	.word 5
C:	.word 4
ans:	.asciiz "Answer = "
nl:	.asciiz "\n"
