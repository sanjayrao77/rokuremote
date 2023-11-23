# rokuremote, a joystick and shell-interface remote control for roku devices, written in C

## Overview

Rokuremote can be used with roku devices as a substitute for a remote control, using
a CLI interface and/or joystick control.

This supports the normal roku remote buttons as well as keyboard input.

Rokuremote is written for unix-like systems, mainly glibc linux. It also works
on Mac OSX but I don't test that as much.

Joystick support only works on linux. 

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
	set a settings value: --Dname=value
	print operations: --verbose
	don't use stdin or stdout: --nostdin
	run in background: --background
```

### no parameters

When run with no command line arguments, rokuremote will use SSDP to find roku devices on
a local network and use the first one that replies. If it picks the wrong local network,
you can use --Dmulticastipv4=XX to specify an interface IP address.

The program will print the serial numbers of all the roku devices that are found.

### sn:SERIALNUMBER

This is the best way to use the program if you have more than one roku device.

You can find your serial number by using the --verbose option. When --verbose is
enabled, it will print device name and location of all discovered Roku devices.

Example:
```
./rokuremote sn:X12300ABC123
```


### ip:IPADDRESS[:port]

This is almost useless for now. It lets you specify an IP for your target Roku device. Roku doesn't currently
support static IPs, so this is only useful if you assign a fixed IP address for your device on your
dhcp server/router.

If you **do** specify a fixed IP for your Roku device, this is the most efficient method to target your
Roku device.

As far as I know, all Roku devices use port 8060, so specifying the port is only useful if you're doing
some sort of port mapping.

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

The following have also worked: Power

Example: This will start rokuremote, find a roku device, send the Home keypress to wake up the TV, enter interactive mode,
then turn off the TV after you exit the program.
```
rokuremote --keypress_Power ; rokuremote --keypress_PowerOff --quit
```

### --quit

This option will make rokuremote exit instead of listening for user input. It's best used in conjunction with --keypress\_XXX commands.

### --nomute

For Roku devices that don't support __VolumeMute__, you can use this option to simulate mute. It will
raise and lower the volume in steps instead of instantly.

You may prefer this mute simulation as it's more gradual. You can tweak the speed with "--Dincvolumedelay=" and
"--Ddecvolumedelay=" variables.

If you use the automute feature, it will use this muting method.

### --Dname=value

There are settings available in the Settings menu. You can specify them on the command line
with "--Dname=value".

"--Dfullvolume=XX" can be used to set the desired volume on the tv, where XX is a number.

"--Dpowerquit=True" can be used to quit after sending a PowerOff keypress.

"--Dincvolumedelay=XX" can be used to add a XX millisecond delay between volume increases with mute simulation.

"--Ddecvolumedelay=XX" can be used to add a XX millisecond delay between volume decreases with mute simulation.

Here's an example. It enables mute simulation with a normal volume level of 15.
```
rokuremote --nomute --Dfullvolume=15 --Dincvolumedelay=100 --Ddecvolumedelay=50
```

You can also specify a multicast ipv4 with "--Dmulticastipv4=IPADDRESS". This IP should be your local ip
for the interface that shares your Roku device. This is not on the Settings menu.

### --verbose

This will print more info to the terminal. It's meant for debugging.

## Usage

After rokuremote starts, it will print a menu on the screen listing the available keys, unless --quit is specified.
As devices are found on the network, it will print those too.

If you want to use it to send commands without waiting for input, you can use the --quit and --keypress\_XXX arguments.

## Auto-mute

I like to turn down the volume for commercials and have the program automatically unmute the volume
after an elapsed time.

You can press 'z' from the main menu to mute for a minute. You can add more time with 'x', 'c', and 'v'.
You can decrease the time with 'Z', 'X', 'C' and 'V'.

To un-mute manually (e.g. if the commercials end early), you can press 'Z' 'Z' to zero out the time
or press "volume up (=/+)".

Some devices support VolumeMute. If your device doesn't support that, you can use the "--nomute" argument
to simulate mute/unmute by using multiple volume down/up commands.

The program is clever enough to stop a mute/unmute simulation if the user specifies the reverse in
the middle. For example, while it is gradually increasing the volume to simulate an unmute, you
can request more automuting by pressing 'z' and then the program will re-mute without completing
the unmute.

## Menus

When the program starts (without --quit) it will print the main menu:
```
./rokuremote --nomute --Dfullvolume=15
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
         m,M                     :   Volume down/up 15 steps
         p                       :   Power off
         q                       :   Quit
         Q                       :   Reboot
         r                       :   Instant Replay
         s                       :   Settings menu
         z,x,c,v                 :   Auto-mute (60/30/10/5 seconds)
         Z,X,C,V                 :   Reduce or cancel auto-mute (60/30/10/5 seconds)
```

If you press 'a' or '1' to enter keyboard mode, you'll get this menu:
```
Roku remote, keyboard mode:
         a-z,0-9,Backspace       :   send key
         Left,Right,Up,Down      :   send key and exit keyboard mode
         Tab,Esc                 :   exit keyboard mode
```

## Youtube muting

On the main menu, 'y' will mute for 5 seconds, send "Select" then unmute. This can be useful
for skipping youtube ads.

## Joystick support

There's some joystick support. It's currently working with a USB SNES-style
gamepad and a Bluetooth 8BitDo Lite gamepad. It only works on linux.

It will auto-detect joysticks with "--joystick". It only supports the Vilros
USB SNES Gamepad and the 8BitDo Lite (Bluetooth or USB, in "X" mode).

You can add your own joystick types in joystick.c but it takes some patience.
You can "#define DEBUGONLY" in joystick.c to make it easier. There is also
"--joystick\_devfile=/dev/input/eventXX" to specify a device file instead of
auto-detect. You'd also need to choose a button mapping, with
"--joystick\_mapping=XX". You can see the snes and 8bitdo examples to copy
them. To add to auto-detect, you can append to the joystick.c:devices\_global[] table.

If you'd like to run rokuremote headless, using only a joystick, you can use the --background argument.
