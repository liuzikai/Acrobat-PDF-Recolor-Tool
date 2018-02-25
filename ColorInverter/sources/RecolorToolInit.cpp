/*********************************************************************
 
 File: RecolorToolInit.cpp
 Description: Initialization of the plug-in.
 Author: liuzikai
 Revise based on Adobe Acrobat DC SDK BasicPluginInit.cpp
 Data: 02/22/2018
 
 *********************************************************************/

// Acrobat Headers.
#ifndef MAC_PLATFORM
#include "PIHeaders.h"
#endif

/*-------------------------------------------------------
	Constants/Declarations
-------------------------------------------------------*/
// stuff for Menu set up
const int menuItemCount = 2;
static AVMenuItem menuItem[menuItemCount] = {NULL};
static long long menuItemData[menuItemCount];
ACCB1 ASBool ACCB2 PluginMenuItem(char* MyMenuItemTitle, char* MyMenuItemName);

// callback functions implemented in file "BasicPlugin.cpp"
extern ACCB1 void ACCB2 StartRecolor(void *clientData);
extern ACCB1 ASBool ACCB2 HaveActiveDocument(void *clientData);
extern ACCB1 ASBool ACCB2 MyPluginSetmenu();

extern const char* MyPluginExtensionName;

/*-------------------------------------------------------
	Core Handshake Callbacks
-------------------------------------------------------*/

/**
	Callback invoked by the application to give the plug-in an opportunity to register 
	an HFTServer with the application.
	@return true to indicate the plug-in should continue loading.
*/
ACCB1 ASBool ACCB2 PluginExportHFTs(void)
{
	return true;
}

/** 
	The application calls this function to allow it to
	<ul>
	<li> Import plug-in supplied HFTs.
	<li> Replace functions in the HFTs you're using (where allowed).
	<li> Register to receive notification events.
	</ul>
*/
ACCB1 ASBool ACCB2 PluginImportReplaceAndRegister(void)
{
	return true;
}

/** 
	The main initialization routine.
	We register our action handler with the application.
	@return true to continue loading the plug-in
	@return false to cause plug-in loading to stop.
*/
/* PluginInit
** ------------------------------------------------------
**
** The main initialization routine.
**
** Return true to continue loading plug-in.
** Return false to cause plug-in loading to stop.
*/
ACCB1 ASBool ACCB2 PluginInit(void)
{
	return MyPluginSetmenu();
}

/** 
	The unload routine.
	Called when your plug-in is being unloaded when the application quits.
	Use this routine to release any system resources you may have
	allocated.

	Returning false will cause an alert to display that unloading failed.
	@return true to indicate the plug-in unloaded.
*/
ACCB1 ASBool ACCB2 PluginUnload(void)
{
    for (int i = 0; i < menuItemCount; i++) {
        if (menuItem[i]) AVMenuItemRemove(menuItem[i]);
    }
	return true;
}

/**
	Return the unique ASAtom associated with your plug-in.
	@return the plug-ins name as an ASAtom.
*/
ASAtom GetExtensionName()
{
	return ASAtomFromString(MyPluginExtensionName);
}


/**
	Function that provides the initial interface between your plug-in and the application.
	This function provides the callback functions to the application that allow it to 
	register the plug-in with the application environment.

	Required Plug-in handshaking routine: <b>Do not change it's name!</b>
	
	@param handshakeVersion the version this plug-in works with. There are two versions possible, the plug-in version 
	and the application version. The application calls the main entry point for this plug-in with its version.
	The main entry point will call this function with the version that is earliest. 
	@param handshakeData OUT the data structure used to provide the primary entry points for the plug-in. These
	entry points are used in registering the plug-in with the application and allowing the plug-in to register for 
	other plug-in services and offer its own.
	@return true to indicate success, false otherwise (the plug-in will not load).
*/
ACCB1 ASBool ACCB2 PIHandshake(Uns32 handshakeVersion, void *handshakeData)
{
	if (handshakeVersion == HANDSHAKE_V0200) {
		/* Cast handshakeData to the appropriate type */
		PIHandshakeData_V0200 *hsData = (PIHandshakeData_V0200 *)handshakeData;

		/* Set the name we want to go by */
		hsData->extensionName = GetExtensionName();

		/* If you export your own HFT, do so in here */
		hsData->exportHFTsCallback = (void*)ASCallbackCreateProto(PIExportHFTsProcType, &PluginExportHFTs);

		/*
		** If you import plug-in HFTs, replace functionality, and/or want to register for notifications before
		** the user has a chance to do anything, do so in here.
		*/
		hsData->importReplaceAndRegisterCallback = (void*)ASCallbackCreateProto(PIImportReplaceAndRegisterProcType,
																		 &PluginImportReplaceAndRegister);

		/* Perform your plug-in's initialization in here */
		hsData->initCallback = (void*)ASCallbackCreateProto(PIInitProcType, &PluginInit);

		/* Perform any memory freeing or state saving on "quit" in here */
		hsData->unloadCallback = (void*)ASCallbackCreateProto(PIUnloadProcType, &PluginUnload);

		/* All done */
		return true;

	} /* Each time the handshake version changes, add a new "else if" branch */

	/*
	** If we reach here, then we were passed a handshake version number we don't know about.
	** This shouldn't ever happen since our main() routine chose the version number.
	*/
	return false;
}

/*-------------------------------------------------------
	Menu Utility
-------------------------------------------------------*/

ACCB1 ASBool ACCB2 AddMenuItem(long long menuItemIndex, const char* menuItemTitle, const char* menuItemName)
{
	AVMenubar menubar = AVAppGetMenubar();
	AVMenu volatile commonMenu = NULL;

	if (!menubar)
		return false;

	DURING

	   	// Create our menuitem
		menuItem[menuItemIndex] = AVMenuItemNew (menuItemTitle, menuItemName, NULL, true, NO_SHORTCUT, 0, NULL, gExtensionID);
        //Set clientData for the callback function to determine which command the user chose.
        menuItemData[menuItemIndex] = menuItemIndex;
		AVMenuItemSetExecuteProc (menuItem[menuItemIndex], ASCallbackCreateProto(AVExecuteProc, StartRecolor), &menuItemData[menuItemIndex]);
		AVMenuItemSetComputeEnabledProc (menuItem[menuItemIndex],
					ASCallbackCreateProto(AVComputeEnabledProc, HaveActiveDocument), (void *)pdPermEdit);

        commonMenu = AVMenubarAcquireMenuByName (menubar, "ADBE:Plug_In_Tool");
        // if "Plug-In Tool" menu is not existing, create one.
        if (!commonMenu) {
            commonMenu = AVMenuNew ("Plug-In Tool", "ADBE:Plug_In_Tool", gExtensionID);
            AVMenubarAddMenu(menubar, commonMenu, APPEND_MENU);
        }
		
		AVMenuAddMenuItem(commonMenu, menuItem[menuItemIndex], APPEND_MENUITEM);

		AVMenuRelease(commonMenu);

	HANDLER
		if (commonMenu)
			AVMenuRelease (commonMenu);
		return false;
	END_HANDLER

	return true;
}

