# Introduction #
So you wanted to compile the priiloader source eh? well, this wiki will explain how using Visual Studio 2008 or Visual C++

# Prequsites #
to compile the source is easy but you need quiet some stuff. what you'll need is :
  * DevkitPPC found [here](http://www.devkitpro.org/downloads/)
  * Visual Studio 2008 or Visual C++
  * a SVN client to download source from. tortoiseSVN recommended
  * A brain (o noez! i don't have one!)

# Guide #

  * Install DevkitPPC (or more if you feel like it) using the DevkitPro installer. note that there should be _NO SPACES IN THE DIRECTORY_ example: C:\DevkitPro
  * Install your SVN Client, throughout the guide ill act like that is tortoiseSVN
  * If Visual Studio/Visual C++ isn't installed yet, do that now

  * Make a folder where the source should be. this should also _NOT_ contain any spaces
  * Right click this folder and choose SVN Checkout. the url of the repository should be http://priiloader.googlecode.com/svn/trunk/ and the checkout directory should be set to the folder you just made. click ok and let it get the source from svn.
  * Go into the folder and open the Priiloader.sln. it should open Visual Studio/Visual C++ with the projects of priiloader and the installer on the right.
  * Get a cert.sys copy from your wii (its in the sys folder) and copy it to the data folder of the priiloader&installer source( priiloader/data & installer/data) and finally rename them to certs.bin
  * Build :P QUICK NOTE : Debug only builds priiloader. Release builds both

> if you want you can also setup wiiload so the build wont error when trying to send the dol to your wii(not that its bad that it does error):
  * Set the environment variable by going to your computer's Control Panel by pressing start -> Control Panel -> System
  * In the new window click the "Advanced" tab and then click the Environment Variables button
  * Click "new" under either category. The variable name is WIILOAD and the value is tcp:yourIP, where yourIP is the Wii's IP/hostname. Click "OK" here and in System Properties.