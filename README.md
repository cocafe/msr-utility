# msr-cmd, msr-watchdog 

------

Two little utilities to set and keep Model Specific Registers (MSR) as you like without any painful experience with using other things like ThrottleStop (this thing leaves a stupid icon on taskbar not notification icon when runs in background, and it resets some settings like C-States when launches) or dumb OEM overclocking tools. 

------

For example, my Xeon E5v4 does not set maximum uncore (cache) clock (results +5%~10% cache performance) by default, and it's unable to tweak via UEFI. Figure out the the uncore clock multiplier MSR register (check out Intel Software Developer Manual) and use **msr-watchdog** to set and keep its value on the fly (as a daemon). Because suspend or resume system will reset the MSR registers.

Also, it can lock MSR mailbox registers (some relate to voltages), too. These values like voltages settings can be yielded from ThrottleStop.

**msr-cmd** is something like **wrmsr** from Intel msr-tools in Linux world, which can be used in batch scripts.

All these utilities are based on WinRing0 driver, no WDK is required.

-----

<u>**ABSOLUTELY NO WARRANTIES, USE ON YOUR OWN RISK.**</u>

------

Build: Microsoft Visual Studio 2017 Community

Run: Microsoft Visual C++ 2015/2017 Runtime is required

Usages (may require administrator privilege):

> msr-watchdog:
>
> ​	Option:
>
> ​		-h		open help text GUI
>
> ​	
>
> ​	[Config Example](msr-watchdog/E5-2683v4-QHV3.ini)
>
> 

> msr-cmd:
>
> ​	Usage:
>
> ​		msr-cmd.exe \[options\] \[read\] \[reg]
>
> ​		msr-cmd.exe \[options] \[write] \[reg] \[edx(63-32)] \[eax(32-0)]
>
> 
>
> ​	Options:
>
> ​		-a		apply to all available processors
>
> ​		-p [CPU]	processor (default: CPU0) to apply
>
> 

------

```
MIT License

Copyright (c) 2018 cocafehj@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

