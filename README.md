Silentcast
==========

<!--
BETTER TO VIEW THIS IN A MARKDOWN VIEWER
When viewed in a markdown viewer, you'll get pictures and even animations made with silentcast. If you don't have a markdown viewer, I personally use a Google Chrome Extension:

https://chrome.google.com/webstore/detail/markdown-preview/jmchmkecamhbiokiopfpnfgbidieafmd

After installing the Markdown Preview extenstion, you can't use it until you go into

chrome://extensions/

to checkmark the box under

Markdown Preview
 for *Allow access to file URLs*

Then, just open this file with Chrome. You'll probably want to make Google Chrome the default for .md files. You can also just drag and drop this file to an empty tab in Chrome.
-->

##Demos
Notice there's a stop icon in the Notificaton Area before I even start Silentcast, then a 2nd stop icon appears when recording begins. That's because I already had Silentcast running to make these animated gifs of how to use Silentcast. Silentcast 1 keeps going after I stop Silentcast 2.
###Fullscreen: How to use Silentcast to record Gimp
![Fullscreen](http://i.imgur.com/I1mXz9N.gif)
###Transparent: How to use Silentcast to record 2 windows
![Transparent](http://i.imgur.com/ak6NQZB.gif)
###Interior: How to use Silentcast to only record the drawing
![Interior](http://i.imgur.com/VAmUl8d.gif)
###Entirety: How to use Silentcast to record 1 window
![Entirety](http://i.imgur.com/XlWzLRW.gif)
##Guides
###Installation

- Desktop Environment Requirements
    - A desktop that includes a standard system-tray notification area with clickable icons.
    - [A compositing window manager](http://en.wikipedia.org/wiki/Compositing_window_manager#List_of_compositing_window_managers) that is compatible with the [EWMH/NetWM](http://en.wikipedia.org/wiki/Extended_Window_Manager_Hints) specification.

- Dependencies (Package names may vary slightly across different Linux distros. These are how they're named in Arch Linux.)
<table>
  <thead>
    <tr>
      <th>package</th>
      <th>reason</th>
    </tr>
  </thead>
  <tr>
    <td>bash</td>
    <td>because Silentcast is mostly bash scripts and I use bashisms</td>
  </tr>
  <tr>
    <td>ffmpeg</td>
    <td>for recording and extracting images</td>
  </tr>
  <tr>
    <td>imagemagick</td>
    <td>for 'convert' to animate images</td>
  </tr>
  <tr>
    <td>yad</td>
    <td>for the GUI - popup dialogue windows</td>
  </tr>
  <tr>
    <td>xrandr</td>
    <td>for getting screen size</td>
  </tr>
  <tr>
    <td>xdotool</td>
    <td>for getting the active window id</td>
  </tr>
  <tr>
    <td>xorg-xwininfo</td>
    <td>for getting window size and position</td>
  </tr>
  <tr>
    <td>wmctrl</td>
    <td>for resizing and positioning windows</td>
  </tr>
  <tr>
    <td>python-gobject</td>
    <td>for 'gi.repository' which has Gtk</td>
  </tr>
  <tr>
    <td>python-cairo</td>
    <td>for making Gtk+ window transparent</td>
  </tr>
  <tr>
    <td>xdg-utils</td>
    <td>for 'xdg-open' to open the file-browser</td>
  </tr>
</table>

- Arch Linux
    - After I've completed this README.md, I'll put it in the AUR so it would be installed with an AUR helper, like `yaourt -S silentcast`. This will automatically install missing dependencies. **Uninstall** with `pacman -R silentcast`
    - Without an AUR helper, just [Download Silentcast master.zip from github.com](https://github.com/colinkeenan/silentcast/archive/master.zip), extract, and do `makepkg -i` from the extracted directory. This will automatically install missing dependencies. **Uninstall** with `pacman -R silentcast`

- Any Linux Distro
    - Install missing dependencies (see the second point in this list)
    - [Download Silentcast master.zip from github.com](https://github.com/colinkeenan/silentcast/archive/master.zip) and extract. Then, either open the extracted folder from a file browser **as root** and double-click **install**, or from a terminal, `cd` into the extracted directory and `sudo ./install` **Uninstall** instructions are the same replacing *install* with *uninstall*. The **install** (or **uninstall**) bash script just copies (or deletes) files. You may want to edit them if your distro puts files in unusual places.

###Launch Methods

- Menu Hierarchy
    - Graphics -> Silentcast
    - Multimedia -> Silentcast
- Search Box Terms
    - silentcast
    - screencast
    - record
    - gif
    - (and other things will work too)
- ALT+F2
    - silentcast
- Terminal
    - silentcast

Find *Silentcast* in the menu under either *Graphics* or *Multimedia*, type *silentcast* into the search box, or *ALT+F2 silentcast*. It can also be run from a terminal as *silentcast*.
### Dialogues: Set 3 2 1 Record Stop Convert
#### Set Area to be recorded and Frames per second
![Set Area](http://i.imgur.com/9EW41mC.png)

- Fullscreen
    - Records the entire screen. Dialogue <span style="color:green;">1</span> will be next because dialogues <span style="color:green;">3</span> and <span style="color:green;">2</span> aren't needed.
- Transparent Window Interior
    - Records the area defined by a transparent window. The next dialogue will be <span style="color:green;">3</span> and it will popup along with a transparent window which you can use to outline the area of the screen you want to record. The transparent window will automatically close before recording begins.
- Interior of a Window
    - Records the interior of the "next active window" which is the window that will become active after closing dialogue <span style="color:green;">2</span>. The interior may include a menu bar or other UI elements, but does not include the title-bar or borders.
- Entirety of a Window
    - Records the "next active window" which is the window that will become active after closing dialogue <span style="color:green;">2</span>. The entire window will be recorded, including the title-bar and borders.

![Set Rate](http://i.imgur.com/GhbZ1PC.png)

#### 3 Auto Resize
![3](http://i.imgur.com/cYcUwF4.png)
![3_trans](http://i.imgur.com/o05GZGx.png)
#### 2 Manual Resize and Position
![2](http://i.imgur.com/tUm3GaI.png)
![2_trans](http://i.imgur.com/KkKvPgG.png)
#### 1 Get Ready
![1](http://i.imgur.com/GL52fLO.png)
#### Record
There's no dialogue, but during recording there will be a stop icon in the system tray notification area. That icon will look different depending on your installed icon theme, but here are a couple of examples: ![gnome-stop](http://i.imgur.com/teUa0iV.png) ![numix-stop](http://i.imgur.com/nEg3PPh.png)
#### Stop
When you click the stop icon in the system tray notification area, you'll see a progress dialogue and your file browser open to the silentcast folder as it makes images from the recording, then the Prepare Images dialogue will popup:
![Prepare Images](http://i.imgur.com/diXXYeY.png)
If you choose to automatically delete a lot of images to create a smaller anim.gif or because your system can't handle so many images at once, then as the dialogue explains, entering *1* will delete every other image, *2* will delete 2 out of every 3 images, etc. *Convert* will adjust timing so playback of anim.gif won't move too fast.

In addition to automatically deleting images, you may choose to "edit" the yet to be made anim.gif by selectively deleting images. Just view an image in any viewer and move forward through them to see frame by frame what anim.gif will look like. Then, if you notice for example that your mouse cursor makes a slow pointless circle, just delete those images. For more ideas on what can be done while this dialogue is up, see *List of Tips* below.
#### Convert
After preparing images and clicking OK, you'll see a progress dialogue as *convert* works on making anim.gif. If successful, Screencast has finished it's job.

However, if you don't have a large enough swap (or no swap as with my own system) and run out of memory, *convert* may crash, and you'll see this dialogue popup:
![convert-error](http://i.imgur.com/K1SEvMz.png)

After clicking OK, you'll be back at the Prepare Images dialogue so that you can delete uneeded images or make other changes that will allow *convert* to complete anim.gif.
##List of Tips
