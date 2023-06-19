/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2009, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************
******************************************************************
* 
* Filename: Ipm_Ifc.c
*
* Descriptions: contains API to access MS IPMI Driver
*
* Author: Rajaganesh R <rajaganeshr@amiindia.co.in>
*         Ramamoorthy Venkatesh  <ramamoorthyv@ami.com>
*
******************************************************************/
#ifdef WIN32 || WIN64	
#define  _WIN32_DCOM // MS IPMI driver needs COM
#include <Windows.h>
#include <stdio.h>
//#include <iostream.h>
//#include <iostream> 

#include <comdef.h>
#include <Wbemidl.h>
#include <comutil.h>
#include <wbemcli.h>

//using namespace std;

//#include 	"Ipm_ifc.h"
extern "C" {
#include "Bmc_ifc.h"
#include "Debug.h"
}

#define MAX_BUFF_SIZE	128

static IWbemLocator *pLoc = NULL;
static IWbemServices *pSvc = NULL;
static IWbemClassObject* pClass = NULL;
static IEnumWbemClassObject* pEnumerator = NULL;
static IWbemClassObject* pInstance = NULL;
static VARIANT varPath;
VARIANT vRespBuf[MAX_BUFF_SIZE];
unsigned char Buff[MAX_BUFF_SIZE];
unsigned char buf[MAX_BUFF_SIZE];


void ReleaseIPMVars(void)
{
   VariantClear(&varPath);
   if (pInstance != NULL) 
   {   
       pInstance->Release();
   }
   if (pEnumerator != NULL) 
   {
       pEnumerator->Release();
   }
   if (pClass != NULL) 
   {
       pClass->Release();
   }
   if (pSvc != 0) 
   {
        pSvc->Release();
   }
   if (pLoc != 0) 
   {
       pLoc->Release();
   }
   CoUninitialize();
}


/*******************************************************************************
*   Routine: IPMOpenIfc
*
*   Arguments:
*                   None
*   Returns:
*       0       Success
*       -1      Error
*
*   Description:
*       This routine opens the BMC interface using MS KCS Driver(IPMDrv.sys)
*
*******************************************************************************/

/* Function to open interface to IPMI device */
extern "C" // Preventing name mangling.
{
	int IPMOpenIfc()
	{
		HRESULT hres;
        unsigned long instCount;
        
		/* Initialize COM */
		hres =  CoInitializeEx(0, COINIT_MULTITHREADED);
		if (FAILED(hres))
		{
			DBG_PRINT(DBG_LVL2,"Co-Initialize Error.\n");
			return -1;      // COM Error
		}

		/* Get WMI locator */
		hres = CoCreateInstance( CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
					IID_IWbemLocator, (LPVOID *) &pLoc);
		if (FAILED(hres))
		{
			CoUninitialize();
			DBG_PRINT(DBG_LVL2,"Unable to create locator instance.\n");
			return -1;
		}

        /* @TODO: <Rajaganesh> It is possible with below method to connect 
                  to remote servers. PMTOOL can run any system in domain and 
                  can config & control the server remotely. 
        */
        
		/* Interface to WMI server */
		hres = pLoc->ConnectServer( 
		                            _bstr_t(L"ROOT\\WMI"), // Current System.
		                            NULL,   // Use current User Account.
		                            NULL,   // Use current password.
		                            0,      // Locale.
		                            NULL,   // Security flags.
		                            0,      // Authority (NTLM Domain)
		                            0,      // Context
		                            &pSvc
		                          );
		if (FAILED(hres))
		{
			pLoc->Release();
			CoUninitialize();
			DBG_PRINT(DBG_LVL2,"Unable to connect to WMI.\n");
			return -1;
		}

		/* Set security settings */
		hres = CoSetProxyBlanket( 
		                        pSvc,               // The proxy service
    		                    RPC_C_AUTHN_WINNT,  // Default Authentication 
		                        RPC_C_AUTHZ_NONE,   // NTLMSSP / Kerberos / SChannel 
    			                NULL, // Pricipal name (NULL incompatible with kerberos)
    			                RPC_C_AUTHN_LEVEL_CALL, // Authenticate at every RPC call
	    		                RPC_C_IMP_LEVEL_IMPERSONATE, // Default impersonation lvl
		    	                NULL, // needed for remote calls not used for local sys
			                    EOAC_NONE // No additional capablities
			                    );
		if (FAILED(hres))
		{
		    ReleaseIPMVars();
			DBG_PRINT(DBG_LVL2,"Unable to connect to service.\n");
			ReleaseIPMVars();
			return -1;     // Program has failed.
		}

		/* Get IPMI object */
		hres = pSvc->GetObject( L"Microsoft_IPMI", 0, NULL, &pClass, NULL);
		if (FAILED(hres))
		{
			ReleaseIPMVars();
			DBG_PRINT(DBG_LVL2,"Unable to open Microsoft IPMI Driver(IPMDrv.sys).\n");
			return -1;     // No driver installed
		}


        /* Create instance */
		hres = pSvc->CreateInstanceEnum( L"microsoft_ipmi", 0, NULL, &pEnumerator);
		if (FAILED(hres))
		{
			ReleaseIPMVars();
			DBG_PRINT(DBG_LVL2,"Unable create IPMI enumerator.\n");
			return -1;
		}

		hres = pEnumerator->Next( WBEM_INFINITE, 1, &pInstance, &instCount);
		if (FAILED(hres))
		{
			ReleaseIPMVars();
			DBG_PRINT(DBG_LVL2,"Unable to get IPMI instance.\n");
			return -1;
		}

		VariantInit(&varPath);
		hres = pInstance->Get(_bstr_t(L"__RelPath"), 0, &varPath, NULL, 0);
		if (FAILED(hres))
		{
			ReleaseIPMVars();
			DBG_PRINT(DBG_LVL2,"Unable to get instance path");
			return -1;
		}

		DBG_PRINT1(DBG_LVL2,"Object created at %ls\n",V_BSTR(&varPath));

//		m_win_driver=MS_DRV;
		return 0;
	}
} // extern C

/*******************************************************************************
*   Routine: IPMCloseIFC
*
*   Arguments:
*                   None
*   Returns:
*       0        Success
*       -1       Error
*
*   Description:
*       This routine closes the BMC interface
*
*******************************************************************************/
extern "C" //Prevent name mangling
{
	int IPMCloseIfc (void)
	{
		ReleaseIPMVars();
    	return 0;
	}
} // extern C

/*******************************************************************************
*   Routine: IPMRequestResp
*
*   Arguments:
*       SystemIfcReq_T* Pointer to request data (see declaration for SystemIfcReq_T)
*       timeOut         How long to wait in ms (note: this param is ignored)
*       respDataPtr     Where to put response data (data without completion code)
*       respDataLen     Expected length of data to be returned
*       completionCode  Completion code returned from BMC
*                 
*   Returns:
*       0        Success
*       -1       Error
*
*   Description:
*       This routine would call the RequestResponse Method of WMI.
*       This handles the communication with BMC, sends and receives 
*       IPMI requests and responses.
*
*******************************************************************************/
extern "C"
{
    /* Gets the data in existing format of AMI SendCmd function. 
       Converts them to WMI format. Sends to WMI Method.
       Converts response back to our format */
    int IPMRequestResp( SystemIfcReq_T* reqPtr,
                        int             timeOut,
                        BYTE*           respDataPtr,
                        int*            respDataLen,
                        BYTE*           completionCode)
    {
        char lun,cmd,netfn,sa;
        lun = reqPtr->rsLun;
        cmd = reqPtr->cmdType;
        netfn = reqPtr->netFn;
        sa= reqPtr->rsSa;

        HRESULT hres;
        IWbemClassObject* pInParams = NULL; /*class definition*/
        IWbemClassObject* pInReq = NULL;    /*instance*/
        IWbemClassObject* pOutResp = NULL;
        SAFEARRAY* pReqsa = NULL;           /* For request */
        SAFEARRAYBOUND rgsabound[1];
        SAFEARRAY *rsa = NULL;
        int in =0;                /* rgIndices */
        long index[1];
        VARIANT varCmd, varNetfn, varLun, varSa, varSize, varData;
        long i;
  //      if (m_win_driver != MS_DRV)
  //      {
  //          return -1;        
  //      }
            
        hres = pClass->GetMethod(L"RequestResponse",0,&pInParams,NULL);
        if (FAILED(hres)) 
        {
            return -1;
        }

        /* Init Variants */
        VariantInit(&varCmd);
        VariantInit(&varNetfn);
        VariantInit(&varLun);
        VariantInit(&varSa);
        VariantInit(&varSize);

        varCmd.vt = VT_UI1;
        varCmd.bVal = cmd;
        hres = pInParams->Put(_bstr_t(L"Command"), 0, &varCmd, 0);
        VariantClear(&varCmd);
        if (FAILED(hres)) 
        {
            pInParams->Release();
            return -1;
        }

       
        varNetfn.vt = VT_UI1;
        varNetfn.bVal = netfn;
        hres = pInParams->Put(_bstr_t(L"NetworkFunction"), 0, &varNetfn, 0);
        VariantClear(&varNetfn);
        if (FAILED(hres)) 
        {
            pInParams->Release();
            return -1;
        }


        varLun.vt = VT_UI1;
        varLun.bVal = lun;
        hres = pInParams->Put(_bstr_t(L"Lun"), 0, &varLun, 0);
        VariantClear(&varLun);
        if (FAILED(hres)) 
        {
            pInParams->Release();
            return -1;
        }


        varSa.vt = VT_UI1;
        varSa.bVal = sa;
        hres = pInParams->Put(_bstr_t(L"ResponderAddress"), 0, &varSa, 0);
        VariantClear(&varSa);
        if (FAILED(hres)) 
        {
            pInParams->Release();
            return -1;
        }


        varSize.vt = VT_I4;
        varSize.lVal = reqPtr->dataLength;
        hres = pInParams->Put(_bstr_t(L"RequestDataSize"), 0, &varSize, 0);
        VariantClear(&varSize);
        if (FAILED(hres)) 
        {
            pInParams->Release();
            return -1;
        }


        rgsabound[0].cElements = reqPtr->dataLength;
        rgsabound[0].lLbound = 0;
        pReqsa = SafeArrayCreate(VT_UI1,1,rgsabound);
        /* Copy the request */
        memset(buf, 0x0, MAX_BUFF_SIZE);
        memcpy(buf,reqPtr->data,reqPtr->dataLength);
        /* Copy to safe array */
        VARIANT tvar;
        if (reqPtr->dataLength > 0)
        {
            for(in =0; in< reqPtr->dataLength; in++)
            {
                VariantInit(&tvar);
                tvar.vt = VT_UI1;
                tvar.bVal = (BYTE)buf[in];
                index[0]=in;
                hres = SafeArrayPutElement(pReqsa, index, &tvar.bVal);
                VariantClear(&tvar);
                if (FAILED(hres)) 
                { 
                    DBG_PRINT(DBG_LVL4,"Unable create request buffer\n");
                    pInParams->Release();
                    return -1;
                }
            } /*end for*/
        }

        varData.vt = VT_ARRAY | VT_UI1;
        varData.parray = pReqsa;

        hres = pInParams->Put(_bstr_t(L"RequestData"), 0, &varData, 0);
        if(FAILED(hres)) 
        {
            DBG_PRINT(DBG_LVL4,"RequestData Failed\n");
            pInParams->Release();
            return -1;
        }

        hres = pSvc->ExecMethod( V_BSTR(&varPath), _bstr_t(L"RequestResponse"), 
                                0, NULL, pInParams, &pOutResp, NULL);

        if (FAILED(hres)) 
        {
            DBG_PRINT(DBG_LVL4,"RequestResponse error \n");
            pInParams->Release();
            return -1;
        }
        else 
        {  
            /* Got a response from BMC */

            VARIANT varCC, varRSz, varRData;
            VariantInit(&varCC);
            VariantInit(&varRSz);
            VariantInit(&varRData);
            
            SAFEARRAYBOUND rgrsabound[1];
            long index[1];
            long rlen;

            hres = pOutResp->Get(_bstr_t(L"CompletionCode"),0, &varCC, NULL, 0);
            if (FAILED(hres)) 
            {
                DBG_PRINT(DBG_LVL4,"Unable to get Completion code\n");
                pInParams->Release();
                pOutResp->Release();
                return -1;
            }

            *completionCode= V_UI1(&varCC);

            hres = pOutResp->Get(_bstr_t(L"ResponseDataSize"),0, &varRSz, NULL, 0);
            if (FAILED(hres)) 
            {
                DBG_PRINT(DBG_LVL4,"Unable to get response data size\n");
                pInParams->Release();
                pOutResp->Release();
                return -1;
            }
            rlen = V_I4(&varRSz);
            *respDataLen = (int)rlen-1; // Ignore CC
          
         
            varRData.vt = VT_ARRAY|VT_UI1;          
            rgrsabound[0].cElements = V_UI4(&varRSz);
            rgrsabound[0].lLbound = 0;
            rsa = SafeArrayCreate(VT_UI1,1,rgrsabound);
            if (rsa == NULL) DBG_PRINT(DBG_LVL4,"Unable to create buffer\n");
           
            hres = pOutResp->Get(_bstr_t(L"ResponseData"),0, &varRData, NULL,0);
            if (FAILED(hres) || (hres != WBEM_S_NO_ERROR )) 
            { /*ignore failure */ 
                DBG_PRINT(DBG_LVL4,"Unable to get response data\n");
            } 
            else 
            {  /* success */
                /* Initialize variants */
                for(in=0;in<MAX_BUFF_SIZE;in++)
                {
                    VariantInit(&vRespBuf[in]);
                }
                if (*respDataLen > 0)
                {
                    for(in =0; in < rlen; in++)
                    {
                        VariantInit(&vRespBuf[in]);
                        index[0]=in;
                        hres = SafeArrayGetElement(varRData.parray, index, &vRespBuf[in].bVal);
                        if (FAILED(hres)) 
                        { 
                            DBG_PRINT(DBG_LVL4,"Unable get response buffer\n");
                            for(in=0;in<MAX_BUFF_SIZE;in++)
                            {
                                VariantClear(&vRespBuf[in]);
                            }
                            pInParams->Release();
                            pOutResp->Release();
                            return -1;
                        }
                        if (in > 0 ) 
                        {
                            Buff[in-1] = V_UI1(&vRespBuf[in]);
                        }
                    } // end for
                 } // end if
                 memcpy(respDataPtr,Buff,*respDataLen);
            }
			if (rsa != NULL)
	        {
    	        SafeArrayDestroy(rsa);
        	}

        }
        /* Uninitialize variants */
        for(in=0;in<MAX_BUFF_SIZE;in++)
        {
            VariantClear(&vRespBuf[in]);
        }
        if (pReqsa != NULL)
        {
            SafeArrayDestroy(pReqsa);
        }

        pInParams->Release();
        pOutResp->Release();
        return 0;
           
    }
}//extern C
#endif //WIN32
