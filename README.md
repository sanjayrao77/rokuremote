# rokuremote, a shell-interface remote control for roku devices, written in C

## Overview

Rokuremote can be used with roku devices as a substitute for a remote control, using
a CLI interface.

This supports the normal roku remote buttons as well as keyboard input.

Rokuremote is written for unix-like systems, mainly glibc linux. It also works on
Mac OSX but I don't test it much.

## Installing

After downloading the source with "git clone https://github.com/sanjayrao77/rokuremote"
you should be able to build it with a simple "make" on glibc linux or Mac OSX.

If you want to run this on a unix-like system without getifaddrs(), you can remove
the getifaddrs requirement by specifying a manual local v4 ip. You can easily do this
by setting MULTICASTIP at the top of the Makefile and running "make rokuremote\_staticip".

## Security

Roku devices currently have no security restrictions. Anyone with TCP access can control
the device. As such, no passwords are needed.

## Command line arguments

```
Usage:
	autodetect: rokuremote
	connect to serial number: rokuremote sn:SERIALNUMBER
	connect to ip: rokuremote ip:IPADDRESS[:port]
	send keypress: rokuremote --keypress_XXX --keypress_YYY
	don't run interactively: rokuremote --keypress_XXX --quit
	simulate mute for streaming devices: --nomute
	print operations: --verbose
```

### no parameters

When run with no command line arguments, rokuremote will use SSDP to find roku devices on
a local network and use the first one that replies.

The program will print the serial numbers of all the roku devices that are found.

### sn:SERIALNUMBER

This is the best way to use the program if you have more than one roku device.
You can find your serial number by using the "ip:" option (below).

Example:
```
./rokuremote sn:X12300ABC123
```


### ip:IPADDRESS[:port]

If you have multiple roku devices, you can use this to determine which one has which
serial number.

If you run the program with no parameters, it will list the IP and serial number of
all roku devices found. After this, you can connect to each IP to determine which one
is the device you want to control. After finding the device, you can use the serial
number with the "sn:" parameter (above).

Example:
```
./rokuremote ip:192.168.1.231
```


### --keypress\_XXXX

This sends the keypress code XXX to the roku device. These codes will be sent before entering interactive
mode. If --quit is used, rokuremote will not enter interactive mode.

You can use multiple arguments to send more than one keypress.

According to Roku, the following codes are supported (substitute for XXX):
Home, Rev, Fwd, Play, Select, Left, Right, Down, Up, Back, InstantReplay, Info, Backspace, Search, 
Enter, FindRemote, VolumeDown, VolumeMute, VolumeUp, PowerOff, ChannelUp, ChannelDown,
InputTuner, InputHDMI1 InputHDMI2, InputHDMI3, InputHDMI4, InputAV1

Example: This will start rokuremote, find a roku device, send the Home keypress to wake up the TV, enter interactive mode,
then turn off the TV after you exit the program.
```
rokuremote --keypress_Home ; rokuremote --keypress_PowerOff --quit
```

### --quit

This option will make rokuremote exit instead of listening for user input. It's best used in conjunction with --keypress\_XXX commands.

### --nomute

For roku devices that don't support __VolumeMute__, you can use this option. I don't
know which devices **do** support VolumeMute, but it might be the TVs.

Instead of sending VolumeMute with the m,M keys, it sends 5 volume up or down
commands. You can lower the volume for commercials with a single key.

### --verbose

This will print more info. It's meant for debugging.

## Usage

After rokuremote starts, it will print a menu on the screen listing the available keys, unless --quit is specified.
As devices are found on the network, it will print those too.

If you want to use it to send commands without waiting for input, you can use the --quit and --keypress\_XXX arguments.

## Menus

When the program starts (without --quit) it will print the main menu:
```
Roku remote, commands:
         a,1                     :   Enter keyboard mode
         Left,Right,Up,Down      :   Send key
         Backspace               :   Back
         Enter                   :   Select
         Escape                  :   Home
         Space                   :   Play/Pause
         <                       :   Reverse
         >                       :   Forward
         -,_                     :   Volume down
         +,=                     :   Volume up
         ?                       :   Search
         d                       :   Discover roku devices
         f                       :   Find remote
         i                       :   Information
         m                       :   Mute
         p                       :   Power off
         q,x                     :   Quit
         r                       :   Instant Replay
```

If you press 'a' or '1' to enter keyboard mode, you'll get this menu:
```
Roku remote, keyboard mode:
         a-z,0-9,Backspace       :   send key
         Left,Right,Up,Down      :   send key and exit keyboard mode
         Tab,Esc                 :   exit keyboard mode
```
