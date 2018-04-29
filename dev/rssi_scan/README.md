ELFloader and shell command 'exec' example for ez240 platform
-----------------------------------------------------------

Compiles the Contiki hello-world application as a Contiki executable (.ce).
The Contiki executable is then uploaded to the ez240 platform via serial, and is
stored in the filesystem.  Finally, the executable is loaded via the shell
command 'exec'.

NOTE: You may have to reduce the ELF loader memory usage
(/platform/ez240/contiki-conf.h).  Since hello-world uses very little memory:

#define ELFLOADER_CONF_DATAMEMORY_SIZE 0x100
#define ELFLOADER_CONF_TEXTMEMORY_SIZE 0x100

1. Upload ez240 shell with 'exec' command and symbols (requires several
   recompilations to generate correct symbols):

    make rssi-scanner
    make MOTES=/dev/tty-node rssi-scanner.upload
    make viewrssi
