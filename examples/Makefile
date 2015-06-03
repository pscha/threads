all:
	gcc -Wall -Werror -o setjmp setjmp.c
	gcc -Wall -Werror -o stack stack.c
	gcc -Wall -Werror -o stack_adv stack_adv.c
	gcc -Wall -Werror -o threadexample threadexample.c
	gcc -Wall -o unblocked_io_read unblocked_io_read.c

pack: clean
	tar cvf ../lab-3-examples.tar ../examples

clean:
	rm -f setjmp stack threadexample unblocked_io_read stack_adv jmp_back_context jmp_back_setjmp switch_context
