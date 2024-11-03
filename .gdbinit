set disassembly-flavor intel
set architecture i386
set pagination off
set print pretty on

define hook-stop
  x/i $pc
end

define hook-run
  info proc mappings
end

define regs
  info registers
  info float
  info all-registers
end

define dasm
  disassemble $pc, $pc+40
end

symbol-file build/rom.elf
file build/rom.elf

# If you're debugging an nxdk-built XBE, you can uncomment the following line.
# You'll need to double-check the .text address though.
# add-symbol-file /path/to/nxdk/samples/triangle/main.exe 0x17000

# If you built using a different path (e.g. via Docker), you'll need to set
# a substitution path for GDB to locate the source files.
# set substitute-path /work /path/to/src/

#172.25.192.1
target remote localhost:1234
b reload_segment_selectors
continue