# Lego-Microscope-Source-Code

This is incomplete, exclusively for a LEGO Computational Microscope outreach project based on ESP32-S3.

# Setting Up in VS-Code

Step 1: Prerequisites
The only prerequisite is VS-Code, per the documentation https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/prerequisites.html.

Step 2: Install ESP-IDF extension
Install the ESP-IDF extension on VS-Code
Ensure that the extension is activated.

Step 3: Open ESP-IDF Installation Manager (EIM)
Select View - Command Palette
Type ESP-IDF: Open ESP-IDF Installation Manager to open the ESP-IDF installation manager.
You may also install ESP-IDF Installation Manager at https://dl.espressif.com/dl/eim/index.html.

Step 3: Install ESP-IDF v5.3.5
Install ESP-IDF v5.3.5 on the EIM.

Step 4: Configuring with VS-Code
All ESP-IDF installations should be automatically detected by the VS-Code extension by reading EIM's eim_idf.json file. 
Note, this file is stored by default on Windows at C:\Espressif\tools\eim_idf.json
And on MacOS/Linux at $HOME/.espressif/tools/eim_idf.json
If your ESP-IDF installation is not detected by the VS-Code extension, you can configure the extension's path for eim_idf.json by configuring the extension configuration setting idf.eimIdfJsonPath in VS-Code's Preferences: Open Settings (UI) command.

Step 5: Configuring ESP-IDF version
Return to the command palette and type select current esp-idf version and select ESP-IDF: Select Current ESP-IDF Version. This should allow you to see your ESP-IDF installations. Select v5.3.5.

Step 6: Creating a new project.
Return to the command palette and type ESP-IDF: New Project. Select v5.3.5. Select get-started and sample project. Set the target to ESP32-S3. Set the board to ESP32-S3 (via builtin USB-JTAG). Serial Port can be configured later.

Step 7: Copy Project.
Open the project created by the project wizard. Make the following modifications to the main folder generated:
-Delete main.c
-Replace contents of CMakeLists.txt with the contents of CMakeLists.txt on GitHub.
-Replace contents of idf_component.yml with the contents of that on GitHub.
-If either of these files do not exist, create them.
-Paste the source code, which is contained in the folders main, images, computation, and components on GitHub into the main folder.
Replace the contents of SDKConfig with that on Github. If it was not created, create it in the project folder, not in the main folder

Step 8: Configure Project.
Return to the Command View Palette, type ESP-IDF: run idf.py reconfigure Task
Wait for it to finish

Step 9: Test project
Test to make sure that the project builds. You can do this in two ways.
-Select the build icon at the bottom of the screen (wrench)
-Open Command View Palette, type ESP-IDF: Build Your Project.

This may take a while, but should result in an image created.

Step 10: Configure COM Port
Connect the ESP32 and your computer. ENSURE THAT THE CABLE USED IS A DATA CABLE.
Return to the Command View Palette, type ESP-IDF: Select Port to Use
The extension should be able to automatically detect the port, but if it cannot, you must configure it yourself. (If multiple devices are connected, for example)

To do this, open device manager, and look under ports. If nothing is there, then your computer is not detecting your ESP32. Ensure that you are connected to the USB port, not ports like COM. Ensure that the cable you are using is a data cable.

Configure the extension to use the port that is connected to your ESP32.

Step 11: Using the extension
Note that building a project does not flash the project. Also note that flashing the project does not build changes made since the last build. Also note that you can monitor the connection, which allows for receiving console logs from the ESP32. When making changes to the code, make sure that you build, flash, and monitor the project.

# Things to Note
If any files are added or deleted, you must run reconfigure project. You also must run reconfigure project if you modify SDKConfig.

SDKConfig must be modified to support PSRAM and external flash, as done in the SDKConfig file on this REPO.

PSRAM must be configured to use OCTAL SPI PSRAM

Flash must be configured to use QUAD SPI PSRAM





