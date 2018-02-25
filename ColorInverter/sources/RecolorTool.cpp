/*********************************************************************
 
 File: RecolorTool.cpp
 Description: Functions of the plug-in.
 Author: liuzikai
 Revise based on Adobe Acrobat DC SDK BasicPlugin.cpp
 Data: 02/22/2018
 
 *********************************************************************/

#include <iostream>
#include <json/json.h>

# if 0
    char str[32768];
    char str_sub[1024];
    #define DBPRINT(args...) sprintf(str, args); AVAlertNote(str)
    #define DBAPPEND(args...) sprintf(str_sub, args); strcat(str, str_sub)
#endif

// Acrobat Headers.
#ifndef MAC_PLATFORM
#include "PIHeaders.h"
#endif

/*-------------------------------------------------------
	Constants/Declarations
-------------------------------------------------------*/
const char* MyPluginExtensionName = "ADBE:RecolorTool";

// A convenient function to add a menu item for your plugin.
ACCB1 ASBool ACCB2 AddMenuItem(long long menuItemIndex, const char* MyMenuItemTitle, const char* MyMenuItemName);

/*-------------------------------------------------------
	Functions
-------------------------------------------------------*/

ACCB1 ASBool ACCB2 MyPluginSetmenu()
{
	return AddMenuItem(0, "Recolor All Pages...", "ADBE:RecolorToolMenu0") && AddMenuItem(1, "Recolor Current Page...", "ADBE:RecolorToolMenu1");
}

// Show a dialog allowing users to select the color rules file.
ASPathName SelectColorRulesFile(ASFileSys * ASF)
{
    AVOpenSaveDialogParamsRec dialogParams;
    
    //Local variables
    AVFileFilterRec filterRec,*filterRecP ;
    AVFileDescRec descRec;
    ASPathName * pathName = NULL;
    ASFileSys fileSys = NULL;
    ASBool bSelected = false;
    char errorBuf[256];
    
    //Set up the PDF file filter description
    strcpy (descRec.extension, "json");
    descRec.macFileType = 0;
    descRec.macFileCreator = 0;
    
    //Set attributes that belong to the AVFileFilterRec object
    memset (&filterRec, 0, sizeof(AVFileFilterRec));
    filterRec.fileDescs = &descRec;
    filterRec.numFileDescs = 1;
    filterRecP = &filterRec;
    
    //Set attributes that belong to the AVOpenSaveDialogParamsRec object
    memset (&dialogParams, 0, sizeof (AVOpenSaveDialogParamsRec));
    dialogParams.size = sizeof(AVOpenSaveDialogParamsRec);
    dialogParams.fileFilters = &filterRecP;
    dialogParams.numFileFilters = 1;
    
    DURING
        //Set the AVFileFilterRec object’s filterDescription attribute
        filterRec.filterDescription = ASTextNew();
        ASTextSetEncoded (filterRec.filterDescription, "JSON Files",
                          ASScriptToHostEncoding(kASRomanScript));
    
        //Set the AVOpenSaveDialogParamsRec object's windowTitle attribute
        dialogParams.windowTitle = ASTextNew();
        ASTextSetEncoded (dialogParams.windowTitle, "Select the File with Color Rules (if there is one)", ASScriptToHostEncoding(kASRomanScript));
    
        //Display the Open dialog box - pass the address of the ASFileSys object
        bSelected = AVAppOpenDialog(&dialogParams, &fileSys, (ASPathName**)&pathName, NULL, NULL);
    
    HANDLER
        //Display an error message to the user
        ASGetErrorString (ASGetExceptionErrorCode(), errorBuf, 256);
        AVAlertNote (errorBuf);
    END_HANDLER
    
    //Destroy the ASText object then return
    ASTextDestroy (filterRec.filterDescription);
    ASTextDestroy (dialogParams.windowTitle);
    
    //Point the ASFileSys argument to the address of the ASFileSys object
    *ASF = fileSys;
    
    return bSelected ? *pathName : NULL;
}

//Store replace colors
PDEColorSpec textStrokeColorSpec, textFillColorSpec;
PDEColorSpec pathStrokeColorSpec, pathFillColorSpec;

//Initializa ColorSpecs
void InitializeColorSpecs() {
    
    PDEColorSpace colourSpace = PDEColorSpaceCreateFromName(ASAtomFromString("DeviceRGB"));
    
    PDEColorValue colourValue;
    colourValue.color[0]=FloatToFixed(1.0);
    colourValue.color[1]=FloatToFixed(1.0);
    colourValue.color[2]=FloatToFixed(1.0);
    
    colourValue.colorObj =NULL;
    colourValue.colorObj2 =NULL;
    
    // create a colour spec based on the colour space and values defined above
    PDEColorSpec colourSpec;
    colourSpec.space = colourSpace;
    colourSpec.value = colourValue;
    
    textStrokeColorSpec = textFillColorSpec = pathStrokeColorSpec = pathFillColorSpec = colourSpec;
}

//Allow users to select the color rules file and read it.
bool SelectAndReadColorRulesFile() {
    ASPathName pathName;
    ASInt32 retVal;
    ASFileSys myFileSys;
    ASFile myFile;
    ASTFilePos myFileSize;
    std::string data;
    
    //Allow user to select the catalog txt file.
    pathName = SelectColorRulesFile(&myFileSys);
    
    //If PathName is valid
    if (pathName) {
        
        //Open the file specified in the file system
        retVal = ASFileSysOpenFile (myFileSys, pathName, ASFILE_READ, &myFile);
        
        if (retVal == 0) {
            
            //Read the content of the file
            myFileSize = ASFileGetEOF(myFile);
            data.resize(myFileSize);
            ASFileRead(myFile, &data[0], myFileSize);
            
            Json::Reader jsonReader;
            Json::Value jsonRoot;
            
            //Apply JSON reader
            if(jsonReader.parse(data, jsonRoot)) {
                if(!jsonRoot["text"].isNull()) {
                    if(!jsonRoot["text"]["stroke"].isNull()) {
                        for (int i = 0; i < 3; i++)
                            textStrokeColorSpec.value.color[i] = FloatToFixed(jsonRoot["text"]["stroke"][i].asFloat());
                    }
                    if(!jsonRoot["text"]["fill"].isNull()) {
                        for (int i = 0; i < 3; i++)
                            textFillColorSpec.value.color[i] = FloatToFixed(jsonRoot["text"]["fill"][i].asFloat());
                    }
                }
                if(!jsonRoot["path"].isNull()) {
                    if(!jsonRoot["path"]["stroke"].isNull()) {
                        for (int i = 0; i < 3; i++)
                            pathStrokeColorSpec.value.color[i] = FloatToFixed(jsonRoot["path"]["stroke"][i].asFloat());
                    }
                    if(!jsonRoot["path"]["fill"].isNull()) {
                        for (int i = 0; i < 3; i++)
                            pathFillColorSpec.value.color[i] = FloatToFixed(jsonRoot["path"]["fill"][i].asFloat());
                    }
                }
                return true;
            } else {
                AVAlertNote("Invalid JSON File!");
            }
        }
    }
    return false;
}

void ProcessPDEContent(PDEContent pdeContent) {
    
    //Get the number of elements located in the PDEContent object
    ASInt32 eleNum = PDEContentGetNumElems(pdeContent);

    //Retrieve each element in the PDEContent object
    for (int eleIndex = 0; eleIndex < eleNum; eleIndex++){
        
        DURING
            //Get a specific element
            PDEElement pdeElement = PDEContentGetElem(pdeContent, eleIndex);
        
            ASInt32 pdeType = PDEObjectGetType((PDEObject)pdeElement);
        
            if (pdeType == kPDEText){

                PDEText textObject = (PDEText)pdeElement;
                
                //Get the number of text runs in the text element
                int numTextRuns = PDETextGetNumRuns(textObject);

                for (int i = 0; i < numTextRuns; i++){
                    
                    PDEGraphicState stateOfRun;
                    
                    //Get its current graphics state
                    PDETextGetGState(textObject, kPDETextRun, i, &stateOfRun, sizeof(PDEGraphicState));
                    
                    //Change its color
                    stateOfRun.strokeColorSpec = textStrokeColorSpec;
                    stateOfRun.fillColorSpec = textFillColorSpec;
                    
                    //Apply changes
                    PDETextRunSetGState(textObject, i, &stateOfRun, sizeof(PDEGraphicState));
                }
            } else if(pdeType == kPDEPath) {

                PDEGraphicState stateOfRun;
                
                //Get its current graphics state
                PDEElementGetGState(pdeElement, &stateOfRun, sizeof(PDEGraphicState));
                
                //Change its color
                stateOfRun.strokeColorSpec = pathStrokeColorSpec;
                stateOfRun.fillColorSpec = pathFillColorSpec;
                
                //Apply changes
                PDEElementSetGState(pdeElement, &stateOfRun, sizeof(PDEGraphicState));
            } else if (pdeType == kPDEContainer) {
                
                //Acquire the PDEContent of the PDEContainer
                PDEContent subContent = PDEContainerGetContent((PDEContainer)pdeElement);
                
                //Process the PDEContent
                ProcessPDEContent(subContent);
                
                //Set the PDEcontent
                PDEContainerSetContent((PDEContainer)pdeElement, subContent);
            }
        HANDLER
            ASRaise(ERRORCODE);
        END_HANDLER
    }
}

void ProcessPage(PDPage pdPage) {
    
    DURING
    
        PDEContent pdeContent = PDPageAcquirePDEContent(pdPage, gExtensionID);
    
        ProcessPDEContent(pdeContent);
    
        //Inform the page that changes have been made
        PDPageSetPDEContentCanRaise(pdPage, gExtensionID);
        PDPageNotifyContentsDidChange(pdPage);
    
        //Release objects
        PDPageReleasePDEContent(pdPage, gExtensionID);
        PDPageRelease(pdPage);
    
    HANDLER
        ASRaise(ERRORCODE);
    END_HANDLER
}

ACCB1 void ACCB2 StartRecolor(void *clientData)
{
    InitializeColorSpecs();
    
    if (SelectAndReadColorRulesFile()) {
        
        DURING
        
            //Create a PDEContent object based on the current page view
            AVDoc avDoc = AVAppGetActiveDoc();
            PDDoc pdDoc = AVDocGetPDDoc(avDoc);
        
            if (*reinterpret_cast<long long *>(clientData) == 0) {
                
                //"Recolor All Pages" is chosen
                
                ASCount pageCount = PDDocGetNumPages(pdDoc);
            
                for (int pageNum = 0; pageNum < pageCount; pageNum++) {
                    
                    PDPage pdPage = PDDocAcquirePage(pdDoc, pageNum);
                    ProcessPage(pdPage);
                    
                }
            } else {
                //"Recolor Current Page" is chosen
                PDPage pdPage =  AVPageViewGetPage(AVDocGetPageView(avDoc));
                ProcessPage(pdPage);
            }
            //Display message
            AVAlertNote("Complete!");
        
        HANDLER
            ASRaise(ERRORCODE);
        END_HANDLER
    }
}

ACCB1 ASBool ACCB2 HaveActiveDocument(void *clientData)
{
	//This code make it is enabled only if there is a open PDF document. 
	return (AVAppGetActiveDoc() != NULL);
}

