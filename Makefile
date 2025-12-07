obj-m += the_guardian.o the_void.o

PWD := $(CURDIR)

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	@# 1. Back up the file if it exists
	@if [ -f compile_commands.json ]; then cp compile_commands.json compile_commands.json.bak; fi

	@# 2. Run the actual kernel clean
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

	@# 3. Restore the file if the backup exists
	@if [ -f compile_commands.json.bak ]; then mv compile_commands.json.bak compile_commands.json; fi
