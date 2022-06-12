CFLAGS   = -Wall -std=gnu99 
INCLUDES = -I .
OBJDIR   = obj

CLIENT_SRCS = client_defines.c defines.c err_exit.c shared_memory.c semaphore.c fifo.c client_0.c 
CLIENT_OBJS = $(addprefix $(OBJDIR)/, $(CLIENT_SRCS:.c=.o))

SERVER_SRCS = defines.c server_defines.c err_exit.c shared_memory.c semaphore.c fifo.c server.c 
SERVER_OBJS = $(addprefix $(OBJDIR)/, $(SERVER_SRCS:.c=.o))

all: $(OBJDIR) client_0 server 

client_0: $(CLIENT_OBJS)
		@echo "Making executable: "$@
		@$(CC) $^ -o $@  -lm

server: $(SERVER_OBJS)
		@echo "Making executable: "$@
		@$(CC) $^ -o $@  -lm
$(OBJDIR):
		@mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c
		$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

clean:
		@rm -vf ${CLIENT_OBJS}
		@rm -vf ${SERVER_OBJS}
		@rm -vf client_0
		@rm -vf server
		@rm -rf ${OBJDIR}
		@find . -type p -delete
		@ipcrm --all
		@echo "Removed object files and executables..."
.PHONY: run clean