;;
;;

%include '/home/user/Desktop/nasmx/inc/nasmx.inc'
%include '/home/user/Desktop/nasmx/inc/linux/libc.inc'
%include '/home/user/Desktop/nasmx/inc/linux/syscall.inc'

FIONBIO equ 0x5421

ENTRY main

struc sockaddr_in
    .sin_family resw 1
    .sin_port resw 1
    .sin_addr resd 1
    .sin_zero resb 8
endstruc

struc pollfd
	.fd resq 1
	.events resw 1
	.revents resw 1
endstruc

[SECTION .data]

tcs_so istruc sockaddr_in
    at sockaddr_in.sin_family, dw 2
    at sockaddr_in.sin_port, dw 0x1700
    at sockaddr_in.sin_addr, dd 0
    at sockaddr_in.sin_zero, dd 0, 0
iend

sockaddr_in_size equ $ - tcs_so

fds istruc pollfd
	at pollfd.fd, dq 0
	at pollfd.events, dw 1
	at pollfd.revents, dw 0
iend

fds_size equ $ - fds

do_linemode db 255, 253, 34
do_linemode_size equ $ - do_linemode

on_linemode db 255, 250, 34, 1, 1, 255, 240
on_linemode_size equ $ - on_linemode

will_echo db 255, 253, 1
will_echo_size equ $ - will_echo

wont_echo db 255, 252, 1
wont_echo_size equ $ - wont_echo



[SECTION .bss]

buffer: resb 1728
ioctlchar: resq 1

reply_buff db 512 dup(0)
reply_buff_size equ  $ - reply_buff

[SECTION .text]

proc main
locals none

	syscall socket, 2, 1, 0
	mov r13, rax

	syscall bind, r13, tcs_so, sockaddr_in_size  
	syscall listen, r13, 4096

	syscall ioctl, r13, FIONBIO, ioctlchar

main_loop:


	mov [pollfd.fd], r13
	syscall poll, fds, fds_size, 100

	syscall accept, r13, tcs_so, sockaddr_in_size


telnet_negotiate_linemode:

	syscall write, pollfd.fd, do_linemode, do_linemode_size 
	syscall write, pollfd.fd, on_linemode, on_linemode_size
	syscall write, pollfd.fd, will_echo, will_echo_size
	syscall write, pollfd.fd, wont_echo, wont_echo_size
 
	syscall read, pollfd.fd, reply_buff, reply_buff_size

	jmp main_loop
	
endproc
