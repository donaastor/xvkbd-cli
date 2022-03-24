# xvkbd-cli
This should be a fork or something of xvkbd project, but I can't figure out how to link it properly, so here it is: http://t-sato.in.coocan.jp/xvkbd/<br>
This is a kind of debloated edition, though calling xvkbd a bloat is not right. Even though this edition splits the size of the binary in half, it's goal was to actually remove the annoying font error messages that print out in the terminal whenever you run the binary from it.<br>
The difference:<br>
&emsp;&emsp;1. Files that are missing are deleted completely<br>
&emsp;&emsp;2. I deleted some lines from these 4 files:<br>
&emsp;&emsp;&emsp;&emsp;1.  xvkbd.c<br>
&emsp;&emsp;&emsp;&emsp;2.  Imakefile<br>
&emsp;&emsp;&emsp;&emsp;3.  XVkbd-common.h<br>
&emsp;&emsp;&emsp;&emsp;4.  XVkbd-common.ad<br>
How to install:<br>
&emsp;&emsp;mkdir /tmp/i-love-you<br>
&emsp;&emsp;cd /tmp/i-love-you<br>
&emsp;&emsp;git clone https://github.com/donaastor/xvkbd-cli.git<br>
&emsp;&emsp;makepkg -esi
