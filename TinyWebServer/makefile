APP = TinyWebServer.out
src = $(wildcard *.c)
target = $(patsubst %.c, %.o, $(src))

$(APP):$(target)
	gcc $^ -o $@ -g && ls

%.o:%.c
	gcc $< -c


.PHONYr:clean
clean:
	rm $(target) $(APP) -rf 2> /dev/null 

