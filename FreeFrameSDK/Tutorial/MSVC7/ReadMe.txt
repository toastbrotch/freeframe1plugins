
////////////////////////////////////////////////////////////////////////////////

The FreeFrame Open Video Plugin System
Copyright (c) 2002, 2003  www.freeframe.org

The FreeFrame AppWizard
Developer: Gualtiero Volpe
Mailto: Gualtiero.Volpe@unige.it

///////////////////////////////////////////////////////////////////////////////


The FreeFrame AppWizard has created a project for making a new FreeFrame plugin 
using the FreeFrame SDK. This is the starting point for making a new FreeFrame 
plugin. Now you have to fill with your own code the methods the Wizard have 
prepared for you.

You will find the following files in the newly crated project.

FFSDKTutorial.dsw
    This file (the project workspace file) contains information on the contents
    and organization of the project workspace. You don't have to modify it.

FFSDKTutorial.dsp
    This file (the project file) contains information at the project level and
    is used to build a single project or subproject. You don't have to modify it.

FFSDKTutorial.opt
    This file (the workspace options file) contains the workspace settings that
    you specify in the Project Settings dialog. These settings specify the
    appearance of the project workspace using your hardware and configuration.
    This binary file is automatically generated when you open the .dsw or .dsp
    file in the IDE. You should not share the .opt file, because it contains
    information specific to your computer.

FFSDKTutorial.ncb
    This file provides information on the NCB (No Compile Browse) parser, the
    mechanism that updates ClassView and WizardBar.
    This is a binary file that is generated automatically and should not be
    shared.

FFSDKTutorial.cpp
    This file is the main source file that contains the plugin methods.
    You will have to fill them with your own code in order to implement your
    plugin. Browse it and look for the TO DO comments.

FFSDKTutorial.h
    This file is the main header file for your plugin. It includes the 
    definition of the class incapsulating your plugin.
   
FFSDKTutorial.def
    This file specifies the methods exported by the dll implementing your
    plugin, namely the plugMain method. Usually, you don't have to modify it.

FFPluginInfoData.cpp
   This file includes the DllMain function of the DLL implementing your plugin.
   It also defines a global variable used by the FreeFrame SDK for keeping
   information about the main features of your plugin. Usually, you don't have 
   to modify it.
