compile:
	gcc *.c -Wall -Wextra -g -lcurl -lexpat -lsqlite3 -lncurses -Iinclude lib/*.c -o bc
