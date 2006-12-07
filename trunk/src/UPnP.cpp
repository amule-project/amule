//
// This file is part of the aMule Project.
//
// Copyright (c) 2006 Marcelo Roberto Jimenez ( phoenix@amule.org )
// Copyright (c) 2006 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//


#define UPNP_C


#include "UPnP.h"


#include <dlfcn.h>				// For dlopen(), dlsym(), dlclose()


#include "amuleIPV4Address.h"                  // For amuleIPV4Address // Do_not_auto_remove

#ifdef __GNUC__
	#if __GNUC__ >= 4
		#define REINTERPRET_CAST(x)	reinterpret_cast<x>
	#endif
#endif
#ifndef REINTERPRET_CAST
	// Let's hope that function pointers are equal in size to data pointers
	#define REINTERPRET_CAST(x)	(x)
#endif


CDynamicLibHandle::CDynamicLibHandle(const char *libname)
:
m_libname(char2unicode(libname)),
m_LibraryHandle(dlopen(libname, RTLD_LAZY))
{
	wxString msg;
	if (!m_LibraryHandle) {
		msg << wxT("error(CDynamicLibHandle): Unable to dlopen ") <<
			m_libname << wxT(". Check PATH and LD_LIBRARY_PATH.");
		AddLogLineM(true, msg);
		throw CUPnPException(msg);
	} else {
		msg << wxT("UPnP: Successfully opened ") << m_libname << wxT(".");
		AddLogLineM(false, msg);
	}
}


CDynamicLibHandle::~CDynamicLibHandle()
{
	wxString msg;
	int err_code = dlclose(m_LibraryHandle);
	if (err_code) {
		msg << wxT("error(CDynamicLibHandle): Error closing ") <<
			m_libname << wxT(": ") << char2unicode(dlerror()) <<
			wxT(".");
		AddLogLineM(true, msg);
		fprintf(stderr, "%s\n", (const char *)unicode2char(msg));
	} else {
		msg << wxT("UPnP: Successfully closed ") << m_libname << wxT(".");
		AddLogLineM(false, msg);
	}
}


const wxString &CUPnPLib::UPNP_ROOT_DEVICE =
	wxT("upnp:rootdevice");

const wxString &CUPnPLib::UPNP_DEVICE_IGW =
	wxT("urn:schemas-upnp-org:device:InternetGatewayDevice:1");
const wxString &CUPnPLib::UPNP_DEVICE_WAN =
	wxT("urn:schemas-upnp-org:device:WANDevice:1");
const wxString &CUPnPLib::UPNP_DEVICE_WAN_CONNECTION =
	wxT("urn:schemas-upnp-org:device:WANConnectionDevice:1");
const wxString &CUPnPLib::UPNP_DEVICE_LAN =
	wxT("urn:schemas-upnp-org:device:LANDevice:1");

const wxString &CUPnPLib::UPNP_SERVICE_LAYER3_FORWARDING =
	wxT("urn:schemas-upnp-org:service:Layer3Forwarding:1");
const wxString &CUPnPLib::UPNP_SERVICE_WAN_COMMON_INTERFACE_CONFIG =
	wxT("urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1");
const wxString &CUPnPLib::UPNP_SERVICE_WAN_IP_CONNECTION =
	wxT("urn:schemas-upnp-org:service:WANIPConnection:1");
const wxString &CUPnPLib::UPNP_SERVICE_WAN_PPP_CONNECTION =
	wxT("urn:schemas-upnp-org:service:WANPPPConnection:1");


const char *CUPnPLib::s_LibIXMLSymbols[] =
{
/* 0*/	"ixmlNode_getFirstChild",
/* 1*/	"ixmlNode_getNextSibling",
/* 2*/	"ixmlNode_getNodeName",
/* 3*/	"ixmlNode_getNodeValue",
/* 4*/	"ixmlNode_getAttributes",
/* 5*/	"ixmlDocument_free",
/* 6*/	"ixmlNamedNodeMap_getNamedItem",
/* 7*/	"ixmlNamedNodeMap_free",
};


const char *CUPnPLib::s_LibUPnPSymbols[] =
{
/* 0*/	"UpnpInit",
/* 1*/	"UpnpFinish",
/* 2*/	"UpnpGetServerPort",
/* 3*/	"UpnpGetServerIpAddress",
/* 4*/	"UpnpRegisterClient",
/* 5*/	"UpnpUnRegisterClient",
/* 6*/	"UpnpSearchAsync",
/* 7*/	"UpnpGetServiceVarStatus",
/* 8*/	"UpnpSendAction",
/* 9*/	"UpnpSendActionAsync",
/*10*/	"UpnpSubscribe",
/*11*/	"UpnpUnSubscribe",
/*12*/	"UpnpDownloadXmlDoc",
/*13*/	"UpnpResolveURL",
/*14*/	"UpnpMakeAction",
/*15*/	"UpnpAddToAction",
/*16*/	"UpnpGetErrorMessage",
};


CUPnPLib::CUPnPLib(CUPnPControlPoint &ctrlPoint)
:
m_ctrlPoint(ctrlPoint),
m_LibIXMLHandle("libixml.so"),
m_LibUPnPHandle("libupnp.so")
{
	// IXML
	m_ixmlNode_getFirstChild =
		REINTERPRET_CAST(IXML_Node *(*)(IXML_Node *))
		(dlsym(m_LibIXMLHandle.Get(), s_LibIXMLSymbols[0]));
	m_ixmlNode_getNextSibling =
		REINTERPRET_CAST(IXML_Node *(*)(IXML_Node *))
		(dlsym(m_LibIXMLHandle.Get(), s_LibIXMLSymbols[1]));
	m_ixmlNode_getNodeName =
		REINTERPRET_CAST(const DOMString (*)(IXML_Node *))
		(dlsym(m_LibIXMLHandle.Get(), s_LibIXMLSymbols[2]));
	m_ixmlNode_getNodeValue =
		REINTERPRET_CAST(const DOMString (*)(IXML_Node *))
		(dlsym(m_LibIXMLHandle.Get(), s_LibIXMLSymbols[3]));
	m_ixmlNode_getAttributes =
		REINTERPRET_CAST(IXML_NamedNodeMap *(*)(IXML_Node *))
		(dlsym(m_LibIXMLHandle.Get(), s_LibIXMLSymbols[4]));
	m_ixmlDocument_free =
		REINTERPRET_CAST(void (*)(IXML_Document *))
		(dlsym(m_LibIXMLHandle.Get(), s_LibIXMLSymbols[5]));
	m_ixmlNamedNodeMap_getNamedItem =
		REINTERPRET_CAST(IXML_Node *(*)(IXML_NamedNodeMap *, const DOMString))
		(dlsym(m_LibIXMLHandle.Get(), s_LibIXMLSymbols[6]));
	m_ixmlNamedNodeMap_free =
		REINTERPRET_CAST(void (*)(IXML_NamedNodeMap *))
		(dlsym(m_LibIXMLHandle.Get(), s_LibIXMLSymbols[7]));
	
	// UPnP
	m_UpnpInit =
		REINTERPRET_CAST(int (*)(const char *, int))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[0]));
	m_UpnpFinish =
		REINTERPRET_CAST(void (*)())
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[1]));
	m_UpnpGetServerPort =
		REINTERPRET_CAST(unsigned short (*)())
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[2]));
	m_UpnpGetServerIpAddress =
		REINTERPRET_CAST(char * (*)())
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[3]));
	m_UpnpRegisterClient =
		REINTERPRET_CAST(int (*)(Upnp_FunPtr, const void *, UpnpClient_Handle *))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[4]));
	m_UpnpUnRegisterClient =
		REINTERPRET_CAST(int (*)(UpnpClient_Handle))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[5]));
	m_UpnpSearchAsync =
		REINTERPRET_CAST(int (*)(UpnpClient_Handle, int, const char *, const void *))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[6]));
	m_UpnpGetServiceVarStatus =
		REINTERPRET_CAST(int (*)(UpnpClient_Handle, const char *, const char *, DOMString *))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[7]));
	m_UpnpSendAction =
		REINTERPRET_CAST(int (*)(UpnpClient_Handle, const char *, const char *, const char *,
			IXML_Document *, IXML_Document **))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[8]));
	m_UpnpSendActionAsync =
		REINTERPRET_CAST(int (*)(UpnpClient_Handle, const char *, const char *, const char *,
			IXML_Document *, Upnp_FunPtr, const void *))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[9]));
	m_UpnpSubscribe =
		REINTERPRET_CAST(int (*)(UpnpClient_Handle, const char *, int *, Upnp_SID))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[10]));
	m_UpnpUnSubscribe =
		REINTERPRET_CAST(int (*)(UpnpClient_Handle, Upnp_SID))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[11]));
	m_UpnpDownloadXmlDoc =
		REINTERPRET_CAST(int (*)(const char *, IXML_Document **))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[12]));
	m_UpnpResolveURL =
		REINTERPRET_CAST(int (*)(const char *, const char *, char *))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[13]));
	m_UpnpMakeAction =
		REINTERPRET_CAST(IXML_Document *(*)(const char *, const char *, int, const char *, ...))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[14]));
	m_UpnpAddToAction =
		REINTERPRET_CAST(int (*)(IXML_Document **, const char *, const char *, const char *, const char *))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[15]));
	m_UpnpGetErrorMessage =
		REINTERPRET_CAST(const char *(*)(int))
		(dlsym(m_LibUPnPHandle.Get(), s_LibUPnPSymbols[16]));
}


Char2UnicodeBuf CUPnPLib::GetUPnPErrorMessage(int code) const
{
	return char2unicode(m_UpnpGetErrorMessage(code));
}


wxString CUPnPLib::processUPnPErrorMessage(
	const wxString &messsage,
	int errorCode,
	IXML_Document *doc) const
{
	wxString msg;
	if (errorCode > 0) {
		CUPnPError e(*this, doc);
		msg << wxT("Error: ") <<
			messsage <<
			wxT(": Error code '") <<
			e.getErrorCode() << 
			wxT("', Error description '") <<
			e.getErrorDescription() <<
			wxT("'.");
		AddLogLineM(false, msg);
	} else {
		msg << wxT("Error: ") <<
			messsage <<
			wxT(": UPnP SDK error: ") <<
			GetUPnPErrorMessage(errorCode) << 
			wxT(" (") << errorCode << wxT(").");
		AddLogLineM(false, msg);
	}
	
	return msg;
}


void CUPnPLib::ProcessActionResponse(
	IXML_Document *RespDoc,
	const wxString &actionName) const
{
	wxString msg = wxT("Response: ");
	IXML_Element *root = Element_GetRootElement(RespDoc);
	IXML_Element *child = Element_GetFirstChild(root);
	if (child) {
		while (child) {
			const DOMString childTag = Element_GetTag(child);
			wxString childValue = Element_GetTextValue(child);
			msg << wxT("\n    ") <<
				char2unicode(childTag) << wxT("='") <<
				childValue << wxT("'");
			child = Element_GetNextSibling(child);
		}
	} else {
		msg << wxT("\n    Empty response for action '") <<
			actionName << wxT("'.");
	}
	AddDebugLogLineM(false, logUPnP, msg);
}


/**
 *  This function returns the root node of a given document.
 */
IXML_Element *CUPnPLib::Element_GetRootElement(
	IXML_Document *doc) const
{
	IXML_Element *root = reinterpret_cast<IXML_Element *>(
		m_ixmlNode_getFirstChild(
			reinterpret_cast<IXML_Node *>(doc)));

	return root;
}


/**
 *  This function returns the first child of a given element.
 */
IXML_Element *CUPnPLib::Element_GetFirstChild(
	IXML_Element *parent) const
{
	IXML_Node *node = reinterpret_cast<IXML_Node *>(parent);
	IXML_Node *child = m_ixmlNode_getFirstChild(node);

	return reinterpret_cast<IXML_Element *>(child);
}


/**
 *  This function returns the next sibling of a given child.
 */
IXML_Element *CUPnPLib::Element_GetNextSibling(
	IXML_Element *child) const
{
	IXML_Node *node = reinterpret_cast<IXML_Node *>(child);
	IXML_Node *sibling = m_ixmlNode_getNextSibling(node);

	return reinterpret_cast<IXML_Element *>(sibling);
}


/**
 *  This function returns the element tag (name)
 */
const DOMString CUPnPLib::Element_GetTag(
	IXML_Element *element) const
{
	IXML_Node *node = reinterpret_cast<IXML_Node *>(element);
	const DOMString tag = m_ixmlNode_getNodeName(node);

	return tag;
}


/**
 *  This function returns the TEXT node value of the current node.
 */
const wxString CUPnPLib::Element_GetTextValue(
	IXML_Element *element) const
{
	if (!element) {
		return wxEmptyString;
	}
	IXML_Node *text = m_ixmlNode_getFirstChild(
		reinterpret_cast<IXML_Node *>(element));
	const wxString ret = char2unicode(m_ixmlNode_getNodeValue(text));

	return ret;
}


/**
 *  This function returns the TEXT node value of the first child matching tag.
 */
const wxString CUPnPLib::Element_GetChildValueByTag(
	IXML_Element *element,
	const DOMString tag) const
{
	IXML_Element *child =
		Element_GetFirstChildByTag(element, tag);

	return Element_GetTextValue(child);
}


/**
 * Returns the first child element that matches the requested tag or
 * NULL if not found.
 */
IXML_Element *CUPnPLib::Element_GetFirstChildByTag(
	IXML_Element *element,
	const DOMString tag) const
{
	if (!element || !tag) {
		return NULL;
	}
	
	IXML_Node *node = reinterpret_cast<IXML_Node *>(element);
	IXML_Node *child = m_ixmlNode_getFirstChild(node);
	const DOMString childTag = m_ixmlNode_getNodeName(child);
	while(child && childTag && strcmp(tag, childTag)) {
		child = m_ixmlNode_getNextSibling(child);
		childTag = m_ixmlNode_getNodeName(child);
	}

	return reinterpret_cast<IXML_Element *>(child);
}


/**
 * Returns the next sibling element that matches the requested tag. Should be
 * used with the return value of Element_GetFirstChildByTag().
 */
IXML_Element *CUPnPLib::Element_GetNextSiblingByTag(
	IXML_Element *element, const DOMString tag) const
{
	if (!element || !tag) {
		return NULL;
	}
	
	IXML_Node *child = reinterpret_cast<IXML_Node *>(element);
	const DOMString childTag = NULL;
	do {
		child = m_ixmlNode_getNextSibling(child);
		childTag = m_ixmlNode_getNodeName(child);
	} while(child && childTag && strcmp(tag, childTag));

	return reinterpret_cast<IXML_Element *>(child);
}


const wxString CUPnPLib::Element_GetAttributeByTag(
	IXML_Element *element, const DOMString tag) const
{
	IXML_NamedNodeMap *NamedNodeMap = m_ixmlNode_getAttributes(
		reinterpret_cast<IXML_Node *>(element));
	IXML_Node *attribute = m_ixmlNamedNodeMap_getNamedItem(NamedNodeMap, tag);
	const DOMString s = m_ixmlNode_getNodeValue(attribute);
	const wxString ret = char2unicode(s);
	m_ixmlNamedNodeMap_free(NamedNodeMap);

	return ret;
}


CUPnPError::CUPnPError(
	const CUPnPLib &upnpLib,
	IXML_Document *errorDoc)
:
m_root            (upnpLib.Element_GetRootElement(errorDoc)),
m_ErrorCode       (upnpLib.Element_GetChildValueByTag(m_root, "errorCode")),
m_ErrorDescription(upnpLib.Element_GetChildValueByTag(m_root, "errorDescription"))
{
}


CUPnPArgument::CUPnPArgument(
	const CUPnPControlPoint &upnpControlPoint,
	CUPnPLib &upnpLib,
	IXML_Element *argument,
	const wxString &WXUNUSED(SCPDURL))
:
m_UPnPControlPoint(upnpControlPoint),
m_name                (upnpLib.Element_GetChildValueByTag(argument, "name")),
m_direction           (upnpLib.Element_GetChildValueByTag(argument, "direction")),
m_retval              (upnpLib.Element_GetFirstChildByTag(argument, "retval")),
m_relatedStateVariable(upnpLib.Element_GetChildValueByTag(argument, "relatedStateVariable"))
{
	wxString msg;
	msg <<	wxT("\n    Argument:")                  <<
		wxT("\n        name: ")	                << m_name <<
		wxT("\n        direction: ")            << m_direction <<
		wxT("\n        retval: ")               << m_retval <<
		wxT("\n        relatedStateVariable: ") << m_relatedStateVariable;
	AddDebugLogLineM(false, logUPnP, msg);
}


CUPnPAction::CUPnPAction(
	const CUPnPControlPoint &upnpControlPoint,
	CUPnPLib &upnpLib,
	IXML_Element *action,
	const wxString &SCPDURL)
:
m_UPnPControlPoint(upnpControlPoint),
m_ArgumentList(upnpControlPoint, upnpLib, action, SCPDURL),
m_name(upnpLib.Element_GetChildValueByTag(action, "name"))
{
	wxString msg;
	msg <<	wxT("\n    Action:")                    <<
		wxT("\n        name: ")	                << m_name;
	AddDebugLogLineM(false, logUPnP, msg);
}


CUPnPAllowedValue::CUPnPAllowedValue(
	const CUPnPControlPoint &upnpControlPoint,
	CUPnPLib &upnpLib,
	IXML_Element *allowedValue,
	const wxString &WXUNUSED(SCPDURL))
:
m_UPnPControlPoint(upnpControlPoint),
m_allowedValue(upnpLib.Element_GetTextValue(allowedValue))
{
	wxString msg;
	msg <<	wxT("\n    AllowedValue:")      <<
		wxT("\n        allowedValue: ")	<< m_allowedValue;
	AddDebugLogLineM(false, logUPnP, msg);
}


CUPnPStateVariable::CUPnPStateVariable(
	const CUPnPControlPoint &upnpControlPoint,
	CUPnPLib &upnpLib,
	IXML_Element *stateVariable,
	const wxString &SCPDURL)
:
m_UPnPControlPoint(upnpControlPoint),
m_AllowedValueList(upnpControlPoint, upnpLib, stateVariable, SCPDURL),
m_name        (upnpLib.Element_GetChildValueByTag(stateVariable, "name")),
m_dataType    (upnpLib.Element_GetChildValueByTag(stateVariable, "dataType")),
m_defaultValue(upnpLib.Element_GetChildValueByTag(stateVariable, "defaultValue")),
m_sendEvents  (upnpLib.Element_GetAttributeByTag (stateVariable, "sendEvents"))
{
	wxString msg;
	msg <<	wxT("\n    StateVariable:")     <<
		wxT("\n        name: ")	        << m_name <<
		wxT("\n        dataType: ")     << m_dataType <<
		wxT("\n        defaultValue: ") << m_defaultValue <<
		wxT("\n        sendEvents: ")   << m_sendEvents;
	AddDebugLogLineM(false, logUPnP, msg);
}


CUPnPSCPD::CUPnPSCPD(
	const CUPnPControlPoint &upnpControlPoint,
	CUPnPLib &upnpLib,
	IXML_Element *scpd,
	const wxString &SCPDURL)
:
m_UPnPControlPoint(upnpControlPoint),
m_ActionList(upnpControlPoint, upnpLib, scpd, SCPDURL),
m_ServiceStateTable(upnpControlPoint, upnpLib, scpd, SCPDURL),
m_SCPDURL(SCPDURL)
{
}


CUPnPArgumentValue::CUPnPArgumentValue()
:
m_argument(),
m_value()
{
}


CUPnPArgumentValue::CUPnPArgumentValue(
	const wxString &argument, const wxString &value)
:
m_argument(argument),
m_value(value)
{
}


CUPnPService::CUPnPService(
	const CUPnPControlPoint &upnpControlPoint,
	CUPnPLib &upnpLib,
	IXML_Element *service,
	const wxString &URLBase)
:
m_UPnPControlPoint(upnpControlPoint),
m_upnpLib(upnpLib),
m_serviceType(upnpLib.Element_GetChildValueByTag(service, "serviceType")),
m_serviceId  (upnpLib.Element_GetChildValueByTag(service, "serviceId")),
m_SCPDURL    (upnpLib.Element_GetChildValueByTag(service, "SCPDURL")),
m_controlURL (upnpLib.Element_GetChildValueByTag(service, "controlURL")),
m_eventSubURL(upnpLib.Element_GetChildValueByTag(service, "eventSubURL")),
m_timeout(1801),
m_SCPD(NULL)
{
	wxString msg;
	int errcode;
	
	char *scpdURL = new char[URLBase.Length() + m_SCPDURL.Length() + 1];
	errcode = upnpLib.m_UpnpResolveURL(
		unicode2char(URLBase),
		unicode2char(m_SCPDURL),
		scpdURL);
	if( errcode != UPNP_E_SUCCESS ) {
		msg << wxT("Error generating scpdURL from ") <<
			wxT("|") << URLBase << wxT("|") <<
			m_SCPDURL << wxT("|.");
		AddDebugLogLineM(false, logUPnP, msg);
	} else {
		m_absSCPDURL = char2unicode(scpdURL);
	}

	char *controlURL = new char[URLBase.Length() + m_controlURL.Length() + 1];
	errcode = upnpLib.m_UpnpResolveURL(
		unicode2char(URLBase),
		unicode2char(m_controlURL),
		controlURL);
	if( errcode != UPNP_E_SUCCESS ) {
		msg << wxT("Error generating controlURL from ") <<
			wxT("|") << URLBase << wxT("|") <<
			m_controlURL << wxT("|.");
		AddDebugLogLineM(false, logUPnP, msg);
	} else {
		m_absControlURL = char2unicode(controlURL);
	}

	char *eventURL = new char[URLBase.Length() + m_eventSubURL.Length() + 1];
	errcode = upnpLib.m_UpnpResolveURL(
		unicode2char(URLBase),
		unicode2char(m_eventSubURL),
		eventURL);
	if( errcode != UPNP_E_SUCCESS ) {
		msg << wxT("Error generating eventURL from ") <<
			wxT("|") << URLBase << wxT("|") <<
			m_eventSubURL << wxT("|.");
		AddDebugLogLineM(false, logUPnP, msg);
	} else {
		m_absEventSubURL = char2unicode(eventURL);
	}

	msg <<	wxT("\n    Service:")           <<
		wxT("\n        serviceType: ")	<< m_serviceType <<
		wxT("\n        serviceId: ")	<< m_serviceId <<
		wxT("\n        SCPDURL: ")	<< m_SCPDURL <<
		wxT("\n        absSCPDURL: ")	<< m_absSCPDURL <<
		wxT("\n        controlURL: ")	<< m_controlURL <<
		wxT("\n        absControlURL: ")<< m_absControlURL <<
		wxT("\n        eventSubURL: ")	<< m_eventSubURL <<
		wxT("\n        absEventSubURL: ")<< m_absEventSubURL;
	AddDebugLogLineM(false, logUPnP, msg);

	if (	m_serviceType == upnpLib.UPNP_SERVICE_WAN_IP_CONNECTION ||
		m_serviceType == upnpLib.UPNP_SERVICE_WAN_PPP_CONNECTION) {
		if (!upnpLib.m_ctrlPoint.GetWanServiceDetected()) {
			// This condition can be used to suspend the parse
			// of the XML tree.
			upnpLib.m_ctrlPoint.SetWanServiceDetected(true);
			upnpLib.m_ctrlPoint.SetWanService(this);
			// Log it
			msg = wxT("WAN Service Detected: ");
			msg << m_serviceType << wxT(".");
			AddDebugLogLineM(true, logUPnP, msg);
			// Subscribe
			upnpLib.m_ctrlPoint.Subscribe(*this);
		} else {
			msg = wxT("WAN service detected again: ");
			msg << m_serviceType <<
				wxT(". Will only use the first instance.");
			AddDebugLogLineM(true, logUPnP, msg);
		}
	} else {
		msg = wxT("Uninteresting service detected: ");
		msg << m_serviceType << wxT(". Ignoring.");
		AddDebugLogLineM(true, logUPnP, msg);
	}
}


CUPnPService::~CUPnPService()
{
	delete m_SCPD;
}


bool CUPnPService::Execute(
	const wxString &ActionName,
	const std::vector<CUPnPArgumentValue> &ArgValue) const
{
	wxString msg;
	wxString msgAction = wxT("Sending action ");
	// Check for correct action name
	ActionList::const_iterator itAction =
		m_SCPD->GetActionList().find(ActionName);
	if (itAction == m_SCPD->GetActionList().end()) {
		msg << wxT("Invalid action name '") << ActionName <<
			wxT("' for service '") << GetServiceId() << wxT("'.");
		AddDebugLogLineM(false, logUPnP, msg);
		return false;
	}
	msgAction << ActionName << wxT("(");
	bool firstTime = true;
	// Check for correct Argument/Value pairs
	const CUPnPAction &action = *(itAction->second);
	for (unsigned int i = 0; i < ArgValue.size(); ++i) {
		ArgumentList::const_iterator itArg =
			action.GetArgumentList().find(ArgValue[i].GetArgument());
		if (itArg == action.GetArgumentList().end()) {
			msg << wxT("Invalid argument name '") << ArgValue[i].GetArgument() <<
				wxT("' for action '") << action.GetName() <<
				wxT("' for service '") << GetServiceId() << wxT("'.");
			AddDebugLogLineM(false, logUPnP, msg);
			return false;
		}
		const CUPnPArgument &argument = *(itArg->second);
		if (argument.GetDirection().Lower() != wxT("in")) {
			msg << wxT("Invalid direction for argument '") <<
				ArgValue[i].GetArgument() <<
				wxT("' for action '") << action.GetName() <<
				wxT("' for service '") << GetServiceId() << wxT("'.");
			AddDebugLogLineM(false, logUPnP, msg);
			return false;
		}
		const wxString relatedStateVariableName =
			argument.GetRelatedStateVariable();
		if (!relatedStateVariableName.IsEmpty()) {
			ServiceStateTable::const_iterator itSVT =
				m_SCPD->GetServiceStateTable().
				find(relatedStateVariableName);
			if (itSVT == m_SCPD->GetServiceStateTable().end()) {
				msg << wxT("Inconsistent Service State Table, did not find '") <<
					relatedStateVariableName <<
					wxT("' for argument '") << argument.GetName() <<
					wxT("' for action '") << action.GetName() <<
					wxT("' for service '") << GetServiceId() << wxT("'.");
				AddDebugLogLineM(false, logUPnP, msg);
				return false;
			}
			const CUPnPStateVariable &stateVariable = *(itSVT->second);
			if (	!stateVariable.GetAllowedValueList().empty() &&
				stateVariable.GetAllowedValueList().find(ArgValue[i].GetValue()) ==
					stateVariable.GetAllowedValueList().end()) {
				msg << wxT("Value not allowed '") << ArgValue[i].GetValue() <<
					wxT("' for state variable '") << relatedStateVariableName <<
					wxT("' for argument '") << argument.GetName() <<
					wxT("' for action '") << action.GetName() <<
					wxT("' for service '") << GetServiceId() << wxT("'.");
				AddDebugLogLineM(false, logUPnP, msg);
				return false;
			}
		}
		if (firstTime) {
			firstTime = false;
		} else {
			msgAction << wxT(", ");
		}
		msgAction <<
			ArgValue[i].GetArgument() <<
			wxT("='") <<
			ArgValue[i].GetValue() <<
			wxT("'");
	}
	msgAction << wxT(")");		
	AddDebugLogLineM(false, logUPnP, msgAction);
	// Everything is ok, make the action
	IXML_Document *ActionDoc = NULL;
	if (ArgValue.size()) {
		for (unsigned int i = 0; i < ArgValue.size(); ++i) {
			int ret = m_upnpLib.m_UpnpAddToAction(
				&ActionDoc,
				unicode2char(action.GetName()),
				unicode2char(GetServiceId()),
				unicode2char(ArgValue[i].GetArgument()),
				unicode2char(ArgValue[i].GetValue()));
			if (ret != UPNP_E_SUCCESS) {
				m_upnpLib.processUPnPErrorMessage(
					wxT("m_UpnpAddToAction"), ret, NULL);
				return false;
			}
		}
	} else {
		ActionDoc = m_upnpLib.m_UpnpMakeAction(
			unicode2char(action.GetName()),
			unicode2char(GetServiceId()),
			0, NULL);
		if (!ActionDoc) {
			msg << wxT("Error: m_UpnpMakeAction returned NULL.");
			AddLogLineM(false, msg);
			return false;
		}
	}
	// Send the action
#if 0
	m_upnpLib.m_UpnpSendActionAsync(
		m_UPnPControlPoint.GetUPnPClientHandle(),
		unicode2char(GetAbsControlURL()),
		unicode2char(GetServiceId()),
		NULL, ActionDoc,
		static_cast<Upnp_FunPtr>(&CUPnPControlPoint::Callback),
		NULL);
	return true;
#endif
	
	IXML_Document *RespDoc = NULL;
	int ret = m_upnpLib.m_UpnpSendAction(
		m_UPnPControlPoint.GetUPnPClientHandle(),
		unicode2char(GetAbsControlURL()),
		unicode2char(GetServiceId()),
		NULL, ActionDoc, &RespDoc);
	if (ret != UPNP_E_SUCCESS) {
		m_upnpLib.processUPnPErrorMessage(
			wxT("m_UpnpSendAction"), ret, RespDoc);
		m_upnpLib.m_ixmlDocument_free(ActionDoc);
		m_upnpLib.m_ixmlDocument_free(RespDoc);
		return false;
	}
	m_upnpLib.m_ixmlDocument_free(ActionDoc);
	
	// Check the response document
	m_upnpLib.ProcessActionResponse(
		RespDoc, action.GetName());
	
	// Free the response document
	m_upnpLib.m_ixmlDocument_free(RespDoc);

	return true;
}


const wxString CUPnPService::GetStateVariable(
	const wxString &stateVariableName) const
{
	wxString msg;
	DOMString StVarVal;
	int ret = m_upnpLib.m_UpnpGetServiceVarStatus(
		m_UPnPControlPoint.GetUPnPClientHandle(),
		unicode2char(GetAbsControlURL()),
		unicode2char(stateVariableName),
		&StVarVal);
	if (ret != UPNP_E_SUCCESS) {
		msg << wxT("GetStateVariable(\"") <<
			stateVariableName <<
			wxT("\"): in a call to m_UpnpGetServiceVarStatus");
		AddDebugLogLineM(false, logUPnP, msg);
		m_upnpLib.processUPnPErrorMessage(msg, ret, NULL);
		return wxEmptyString;
	}
	msg << wxT("GetStateVariable: ") <<
		stateVariableName <<
		wxT("='") <<
		char2unicode(StVarVal) <<
		wxT("'.");
	AddDebugLogLineM(false, logUPnP, msg);
	return wxEmptyString;
}


CUPnPDevice::CUPnPDevice(
	const CUPnPControlPoint &upnpControlPoint,
	CUPnPLib &upnpLib,
	IXML_Element *device,
	const wxString &URLBase)
:
m_UPnPControlPoint(upnpControlPoint),
m_DeviceList(upnpControlPoint, upnpLib, device, URLBase),
m_ServiceList(upnpControlPoint, upnpLib, device, URLBase),
m_deviceType       (upnpLib.Element_GetChildValueByTag(device, "deviceType")),
m_friendlyName     (upnpLib.Element_GetChildValueByTag(device, "friendlyName")),
m_manufacturer     (upnpLib.Element_GetChildValueByTag(device, "manufacturer")),
m_manufacturerURL  (upnpLib.Element_GetChildValueByTag(device, "manufacturerURL")),
m_modelDescription (upnpLib.Element_GetChildValueByTag(device, "modelDescription")),
m_modelName        (upnpLib.Element_GetChildValueByTag(device, "modelName")),
m_modelNumber      (upnpLib.Element_GetChildValueByTag(device, "modelNumber")),
m_modelURL         (upnpLib.Element_GetChildValueByTag(device, "modelURL")),
m_serialNumber     (upnpLib.Element_GetChildValueByTag(device, "serialNumber")),
m_UDN              (upnpLib.Element_GetChildValueByTag(device, "UDN")),
m_UPC              (upnpLib.Element_GetChildValueByTag(device, "UPC")),
m_presentationURL  (upnpLib.Element_GetChildValueByTag(device, "presentationURL"))
{
	wxString msg;
#warning Check this limit.
	char presURL[200];
	
	int errcode = upnpLib.m_UpnpResolveURL(
		unicode2char(URLBase),
		unicode2char(m_presentationURL),
		presURL);
	if (errcode != UPNP_E_SUCCESS) {
		msg << wxT("Error generating presentationURL from ") <<
			wxT("|") << URLBase << wxT("|") <<
			m_presentationURL << wxT("|.");
		AddDebugLogLineM(false, logUPnP, msg);
	} else {
		m_presentationURL = char2unicode(presURL);
	}
	
	msg.Empty();
	msg <<	wxT("\n    Device: ")			<<
		wxT("\n        friendlyName: ")		<< m_friendlyName <<
		wxT("\n        deviceType: ")		<< m_deviceType <<
		wxT("\n        manufacturer: ")		<< m_manufacturer <<
		wxT("\n        manufacturerURL: ")	<< m_manufacturerURL <<
		wxT("\n        modelDescription: ")	<< m_modelDescription <<
		wxT("\n        modelName: ")		<< m_modelName <<
		wxT("\n        modelNumber: ")		<< m_modelNumber <<
		wxT("\n        modelURL: ")		<< m_modelURL <<
		wxT("\n        serialNumber: ")		<< m_serialNumber <<
		wxT("\n        UDN: ")			<< m_UDN <<
		wxT("\n        UPC: ")			<< m_UPC <<
		wxT("\n        presentationURL: ")	<< m_presentationURL;
	AddDebugLogLineM(false, logUPnP, msg);
}


CUPnPRootDevice::CUPnPRootDevice(
	const CUPnPControlPoint &upnpControlPoint,
	CUPnPLib &upnpLib,
	IXML_Element *rootDevice,
	const wxString &OriginalURLBase,
	const wxString &FixedURLBase,
	const char *location,
	int expires)
:
CUPnPDevice(upnpControlPoint, upnpLib, rootDevice, FixedURLBase),
m_UPnPControlPoint(upnpControlPoint),
m_URLBase(OriginalURLBase),
m_location(char2unicode(location)),
m_expires(expires)
{
	wxString msg;
	msg <<
		wxT("\n    Root Device: ")		<<
		wxT("\n        URLBase: ")		<< m_URLBase <<
		wxT("\n        Fixed URLBase: ")	<< FixedURLBase <<
		wxT("\n        location: ")		<< m_location <<
		wxT("\n        expires: ")		<< m_expires;
	AddDebugLogLineM(false, logUPnP, msg);
}


CUPnPControlPoint *CUPnPControlPoint::s_CtrlPoint = NULL;


CUPnPControlPoint::CUPnPControlPoint(unsigned short udpPort)
:
m_upnpLib(*this),
m_UPnPClientHandle(),
m_RootDeviceList(),
m_RootDeviceListMutex(),
m_IGWDeviceDetected(false),
m_WanServiceDetected(false),
m_WanService(NULL)
{
	// Pointer to self
	s_CtrlPoint = this;
	// Null string at first
	wxString msg;
	
	// Start UPnP
	int ret;
	char *ipAddress = NULL;
	unsigned short port = 0;
	ret = m_upnpLib.m_UpnpInit(ipAddress, udpPort);
	if (ret != UPNP_E_SUCCESS) {
		msg += wxT("error(UpnpInit): Error code ");
		goto error;
	}
	port = m_upnpLib.m_UpnpGetServerPort();
	ipAddress = m_upnpLib.m_UpnpGetServerIpAddress();
	msg << wxT("UPnP: bound to ") << char2unicode(ipAddress) << wxT(":") <<
		port << wxT(".");
	AddLogLineM(false, msg);
	msg.Empty();
	ret = m_upnpLib.m_UpnpRegisterClient(static_cast<Upnp_FunPtr>(&CUPnPControlPoint::Callback),
		&m_UPnPClientHandle, &m_UPnPClientHandle);
	if (ret != UPNP_E_SUCCESS) {
		msg += wxT("error(UpnpRegisterClient): Error registering callback: ");
		goto error;
	}

	// We could ask for just the right device here. If the root device
	// contains the device we want, it will respond with the full XML doc,
	// including the root device and every sub-device it has.
	// 
	// But lets find out what we have in our network by calling UPNP_ROOT_DEVICE.
	//
	// We should not search twice, because this will produce two
	// UPNP_DISCOVERY_SEARCH_TIMEOUT events, and we might end with problems
	// on the mutex.
	ret = m_upnpLib.m_UpnpSearchAsync(m_UPnPClientHandle, 3, unicode2char(m_upnpLib.UPNP_ROOT_DEVICE), NULL);
	//ret = m_upnpLib.m_UpnpSearchAsync(m_UPnPClientHandle, 3, unicode2char(m_upnpLib.UPNP_DEVICE_IGW), this);
	//ret = m_upnpLib.m_UpnpSearchAsync(m_UPnPClientHandle, 3, unicode2char(m_upnpLib.UPNP_DEVICE_LAN), this);
	//ret = m_upnpLib.m_UpnpSearchAsync(m_UPnPClientHandle, 3, unicode2char(m_upnpLib.UPNP_DEVICE_WAN_CONNECTION), this);
	if (ret != UPNP_E_SUCCESS) {
		msg += wxT("error(UpnpSearchAsync): Error sending search request: ");
		goto error;
	}

	// Wait for the UPnP initialization to complete.
	{
		// Lock the search timeout mutex
		m_WaitForSearchTimeout.Lock();

		// Lock it again, so that we block. Unlocking will only happen
		// when the UPNP_DISCOVERY_SEARCH_TIMEOUT event occurs at the
		// callback.
		wxMutexLocker lock(m_WaitForSearchTimeout);
	}
	return;

	// Error processing
error:
	m_upnpLib.m_UpnpFinish();
	msg << ret << wxT(": ") << m_upnpLib.GetUPnPErrorMessage(ret) << wxT(".");
	throw CUPnPException(msg);
}


CUPnPControlPoint::~CUPnPControlPoint()
{
	for(	RootDeviceList::iterator it = m_RootDeviceList.begin();
		it != m_RootDeviceList.end();
		++it) {
		delete (*it).second;
	}
	// Remove all first
	// RemoveAll();
	m_upnpLib.m_UpnpUnRegisterClient(m_UPnPClientHandle);
	m_upnpLib.m_UpnpFinish();
}


/**
 * portArray[0]: TCP (EC)
 * portArray[1]: UDP (TCP+3)
 * portArray[2]: TCP (TCP)
 * portArray[3]: UDP (Extended eMule)
 */
bool CUPnPControlPoint::AcquirePortList(
	const amuleIPV4Address portArray[],
	int n)
{
	wxString msg;
	if (!GetWanServiceDetected()) {
		msg << wxT(
			"UPnP Error: "
			"CUPnPControlPoint::AcquirePortList: "
			"Wan Service not detected.");
		AddLogLineM(true, msg);
		return false;
	}
	
	bool ok = false;
	char *protocol[4] = {
		"TCP",
		"UDP",
		"TCP",
		"UDP",
	};
	
	wxString actionName(wxT("AddPortMapping"));
	std::vector<CUPnPArgumentValue> argval(8);
	argval[0].SetArgument(wxT("NewRemoteHost"));
	argval[0].SetValue(wxT(""));
	argval[1].SetArgument(wxT("NewExternalPort"));
	//argval[1].SetValue(wxT(""));
	argval[2].SetArgument(wxT("NewProtocol"));
	//argval[2].SetValue();
	argval[3].SetArgument(wxT("NewInternalPort"));
	//argval[3].SetValue();
	argval[4].SetArgument(wxT("NewInternalClient"));
	//argval[4].SetValue();
	argval[5].SetArgument(wxT("NewEnabled"));
	argval[5].SetValue(wxT("1"));
	argval[6].SetArgument(wxT("NewPortMappingDescription"));
	argval[6].SetValue(wxT("aMule port mapping"));
	argval[7].SetArgument(wxT("NewLeaseDuration"));
	argval[7].SetValue(wxT("0"));
	wxString ipAddress(char2unicode(m_upnpLib.m_UpnpGetServerIpAddress()));
	wxString port;
	for (int i = 0; i < n; ++i) {
		port.Printf(wxT("%u"), portArray[i].Service());
		// NewExternalPort
		argval[1].SetValue(port);
		// NewProtocol
		argval[2].SetValue(char2unicode(protocol[i]));
		// NewInternalPort
		argval[3].SetValue(port);
		// NewInternalClient
		argval[4].SetValue(ipAddress);
		m_WanService->Execute(actionName, argval);
	}
	m_WanService->GetStateVariable(wxT("ConnectionType"));
	m_WanService->GetStateVariable(wxT("PossibleConnectionTypes"));
	m_WanService->GetStateVariable(wxT("ConnectionStatus"));
	m_WanService->GetStateVariable(wxT("Uptime"));
	m_WanService->GetStateVariable(wxT("LastConnectionError"));
	m_WanService->GetStateVariable(wxT("RSIPAvailable"));
	m_WanService->GetStateVariable(wxT("NATEnabled"));
	m_WanService->GetStateVariable(wxT("ExternalIPAddress"));
	m_WanService->GetStateVariable(wxT("PortMappingNumberOfEntries"));
	m_WanService->GetStateVariable(wxT("PortMappingLeaseDuration"));
	argval.resize(0);
//	argval[0].SetArgument(wxT("NewConnectionStatus"));
//	argval[0].SetValue(wxT(""));
	m_WanService->Execute(wxT("GetStatusInfo"), argval);
	
#if 0
	m_WanService->GetStateVariable(wxT("PortMappingEnabled"));
	m_WanService->GetStateVariable(wxT("RemoteHost"));
	m_WanService->GetStateVariable(wxT("ExternalPort"));
	m_WanService->GetStateVariable(wxT("InternalPort"));
	m_WanService->GetStateVariable(wxT("PortMappingProtocol"));
	m_WanService->GetStateVariable(wxT("InternalClient"));
	m_WanService->GetStateVariable(wxT("PortMappingDescription"));
#endif
	// Not very good, must find a better test
	wxString PortMappingNumberOfEntries =
		m_WanService->GetStateVariable(
			wxT("PortMappingNumberOfEntries"));
	unsigned long numberOfEntries;
	PortMappingNumberOfEntries.ToULong(&numberOfEntries);
	ok = numberOfEntries >= 4;
	
	return ok;
}

// This function is static
int CUPnPControlPoint::Callback(Upnp_EventType EventType, void *Event, void *Cookie)
{
fprintf(stderr, "Callback: %d, Cookie: %p\n", EventType, Cookie);
	wxString msg;
	// Somehow, this is unreliable. UPNP_DISCOVERY_ADVERTISEMENT_ALIVE events
	// happen with a wrong cookie and... boom!
	// CUPnPControlPoint *upnpCP = static_cast<CUPnPControlPoint *>(Cookie);
	CUPnPControlPoint *upnpCP = CUPnPControlPoint::s_CtrlPoint;
	
	switch (EventType) {
	case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
fprintf(stderr, "Callback: UPNP_DISCOVERY_ADVERTISEMENT_ALIVE\n");
	case UPNP_DISCOVERY_SEARCH_RESULT: {
fprintf(stderr, "Callback: UPNP_DISCOVERY_SEARCH_RESULT\n");
		// UPnP Discovery
		msg << wxT("error(UPNP_DISCOVERY_{ADVERTISEMENT_ALIVE,SEARCH_RESULT}): ");
		struct Upnp_Discovery *d_event = (struct Upnp_Discovery *)Event;
		IXML_Document *doc = NULL;
		int ret;
		if (d_event->ErrCode != UPNP_E_SUCCESS) {
			msg << upnpCP->m_upnpLib.GetUPnPErrorMessage(d_event->ErrCode) << wxT(".");
			AddDebugLogLineM(true, logUPnP, msg);
		}
		// Get the XML tree device description in doc
		ret = upnpCP->m_upnpLib.m_UpnpDownloadXmlDoc(d_event->Location, &doc); 
		if (ret != UPNP_E_SUCCESS) {
			msg << wxT("Error retrieving device description from ") <<
				char2unicode(d_event->Location) << wxT(": ") <<
				upnpCP->m_upnpLib.GetUPnPErrorMessage(d_event->ErrCode) << wxT(".");
			AddDebugLogLineM(true, logUPnP, msg);
		}
		if (doc) {
			// Get the root node
			IXML_Element *root =
				upnpCP->m_upnpLib.Element_GetRootElement(doc);
			// Extract the URLBase
			const wxString urlBase = upnpCP->m_upnpLib.
				Element_GetChildValueByTag(root, "URLBase");
			// Get the root device
			IXML_Element *rootDevice = upnpCP->m_upnpLib.
				Element_GetFirstChildByTag(root, "device");
			// Extract the deviceType
			wxString devType(upnpCP->m_upnpLib.
				Element_GetChildValueByTag(rootDevice, "deviceType"));
			// Only add device if it is an InternetGatewayDevice
			if (!devType.CmpNoCase(upnpCP->m_upnpLib.UPNP_DEVICE_IGW)) {
				// This condition can be used to auto-detect
				// the UPnP device we are interested in.
				// Obs.: Don't block the entry here on this 
				// condition! There may be more than one device,
				// and the first that enters may not be the one
				// we are interested in!
				upnpCP->SetIGWDeviceDetected(true);
				// Log it
				msg = wxT("Internet Gateway Device Detected.");
				AddLogLineM(true, msg);
				// Add the root device to our list
				upnpCP->AddRootDevice(rootDevice, urlBase,
					d_event->Location, d_event->Expires);
			}
			// Free the XML doc tree
			upnpCP->m_upnpLib.m_ixmlDocument_free(doc);
			delete devType;
		}
		break;
	}
	case UPNP_DISCOVERY_SEARCH_TIMEOUT: {
fprintf(stderr, "Callback: UPNP_DISCOVERY_SEARCH_TIMEOUT\n");
		// Search timeout
		msg << wxT("UPNP_DISCOVERY_SEARCH_TIMEOUT.");
		AddDebugLogLineM(false, logUPnP, msg);
		
		// Unlock the search timeout mutex
		upnpCP->m_WaitForSearchTimeout.Unlock();
		
		break;
	}
	case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE: {
fprintf(stderr, "Callback: UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE\n");
		// UPnP Device Removed
		struct Upnp_Discovery *dab_event = (struct Upnp_Discovery *)Event;
		if( dab_event->ErrCode != UPNP_E_SUCCESS ) {
			msg << wxT("error(UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE): ") <<
				upnpCP->m_upnpLib.GetUPnPErrorMessage(dab_event->ErrCode) <<
				wxT(".");
			AddDebugLogLineM(true, logUPnP, msg);
		}
		wxString devType = char2unicode(dab_event->DeviceType);
		devType.Trim();
		// Check for an InternetGatewayDevice and removes it from the list
		if(devType.CmpNoCase(upnpCP->m_upnpLib.UPNP_DEVICE_IGW) == 0) {
			upnpCP->RemoveRootDevice(dab_event->DeviceId);
		}
		break;
	}
	case UPNP_EVENT_RECEIVED: {
fprintf(stderr, "Callback: UPNP_EVENT_RECEIVED\n");
		// Event reveived
		struct Upnp_Event *e_event = (struct Upnp_Event *)Event;
		const wxString Sid = char2unicode(e_event->Sid);
		// Parses the event
		upnpCP->OnEventReceived(Sid, e_event->EventKey, e_event->ChangedVariables);
		break;
	}
	case UPNP_CONTROL_ACTION_COMPLETE: {
fprintf(stderr, "Callback: UPNP_CONTROL_ACTION_COMPLETE\n");
		// This is here in case we choose to do this assynchronously
		struct Upnp_Action_Complete *a_event =
			(struct Upnp_Action_Complete *)Event;
		if (a_event->ErrCode != UPNP_E_SUCCESS) {
			upnpCP->m_upnpLib.processUPnPErrorMessage(
				wxT("m_UpnpSendActionAsync"),
				a_event->ErrCode,
				a_event->ActionResult);
		} else {
			// Check the response document
			upnpCP->m_upnpLib.ProcessActionResponse(
				a_event->ActionResult,
				wxT("<m_UpnpSendActionAsync>"));
		}
		/* No need for any processing here, just print out results.
		 * Service state table updates are handled by events.
		 */
		break;
	}
	default:
fprintf(stderr, "Callback: default... Not good.\n");
		// Humm, this is not good, we forgot to handle something...
		msg << wxT("error(UPnP::Callback): Event not handled.");
		AddDebugLogLineM(true, logUPnP, msg);
		fprintf(stderr, "%s\n", (const char *)unicode2char(msg));
		// Better not throw in the callback. Who would catch it?
//		throw CUPnPException(msg);
		break;
	}
	
	return 0;
}


void CUPnPControlPoint::OnEventReceived(
		const wxString &Sid,
		int EventKey,
		IXML_Document *ChangedVariablesDoc)
{
	wxString msg;
	msg << wxT("UPNP_EVENT_RECEIVED:") <<
		wxT("\n    SID: ") << Sid <<
		wxT("\n    Key: ") << EventKey <<
		wxT("\n    Property list:");
	IXML_Element *root =
		m_upnpLib.Element_GetRootElement(ChangedVariablesDoc);
	IXML_Element *child =
		m_upnpLib.Element_GetFirstChild(root);
	if (child) {
		while (child) {
			IXML_Element *child2 =
				m_upnpLib.Element_GetFirstChild(child);
			const DOMString childTag =
				m_upnpLib.Element_GetTag(child2);
			wxString childValue =
				m_upnpLib.Element_GetTextValue(child2);
			msg << wxT("\n        ") <<
				char2unicode(childTag) << wxT("='") <<
				childValue << wxT("'");
			child = m_upnpLib.Element_GetNextSibling(child);
		}
	} else {
		msg << wxT("\n    Empty property list.");
	}
	AddDebugLogLineM(true, logUPnP, msg);
	// Freeing that doc segfaults. Probably should not be freed.
	//m_upnpLib.m_ixmlDocument_free(ChangedVariablesDoc);
}


void CUPnPControlPoint::AddRootDevice(
	IXML_Element *rootDevice, const wxString &urlBase,
	const char *location, int expires)
{
	// Lock the Root Device List
	wxMutexLocker lock(m_RootDeviceListMutex);
	
	// Root node's URLBase
	wxString OriginalURLBase(urlBase);
	wxString FixedURLBase(OriginalURLBase.IsEmpty() ?
		char2unicode(location) :
		OriginalURLBase);

	// Get the UDN (Unique Device Name)
	wxString UDN(
		m_upnpLib.Element_GetChildValueByTag(rootDevice, "UDN"));
	RootDeviceList::iterator it = m_RootDeviceList.find(UDN);
	bool alreadyAdded = it != m_RootDeviceList.end();
	if (alreadyAdded) {
		// Just set the expires field
		(*it).second->SetExpires(expires);
	} else {
		// Add a new root device to the root device list
		CUPnPRootDevice *upnpRootDevice = new CUPnPRootDevice(
			*this, m_upnpLib, rootDevice,
			OriginalURLBase, FixedURLBase,
			location, expires);
		m_RootDeviceList[upnpRootDevice->GetUDN()] = upnpRootDevice;
	}
}


void CUPnPControlPoint::RemoveRootDevice(const char *udn)
{
	// Lock the Root Device List
	wxMutexLocker lock(m_RootDeviceListMutex);

	// Remove
	wxString UDN(char2unicode(udn));
	RootDeviceList::iterator it = m_RootDeviceList.find(UDN);
	delete (*it).second;
	m_RootDeviceList.erase(UDN);
}


void CUPnPControlPoint::Subscribe(CUPnPService &service)
{
	wxString msg;
	int errcode = m_upnpLib.m_UpnpSubscribe(m_UPnPClientHandle,
		unicode2char(service.GetAbsEventSubURL()),
		service.GetTimeoutAddr(),
		service.GetSID());
	if (errcode == UPNP_E_SUCCESS) {
		msg << wxT("Successfully subscribed to service ") <<
			service.GetServiceType() << wxT(", absEventSubURL: ") <<
			service.GetAbsEventSubURL() << wxT(".");
		AddLogLineM(true, msg);

		IXML_Document *scpdDoc = NULL;
		errcode = m_upnpLib.m_UpnpDownloadXmlDoc(
			unicode2char(service.GetAbsSCPDURL()), &scpdDoc);
		if (errcode == UPNP_E_SUCCESS) {
			// Get the root node
			IXML_Element *scpdRoot =
				m_upnpLib.Element_GetRootElement(scpdDoc);
			CUPnPSCPD *scpd = new CUPnPSCPD(*this, m_upnpLib,
				scpdRoot, service.GetAbsSCPDURL());
			service.SetSCPD(scpd);
		} else {
			msg.Empty();
			msg << wxT("Error getting SCPD Document from ") <<
				service.GetAbsSCPDURL() << wxT(".");
			AddLogLineM(true, msg);
		}
	} else {
		msg << wxT("Error subscribing to service ") <<
			service.GetServiceType() << wxT(", absEventSubURL: ") <<
			service.GetAbsEventSubURL() << wxT(", error: ") <<
			m_upnpLib.GetUPnPErrorMessage(errcode) << wxT(".");
		goto error;
	}
//	service.GetStatusInfo();
	
	return;

	// Error processing	
error:
	AddLogLineM(true, msg);
}


void CUPnPControlPoint::Unsubscribe(CUPnPService &service)
{
	m_upnpLib.m_UpnpUnSubscribe(m_UPnPClientHandle, service.GetSID());
}


#if 0
int
SampleUtil_FindAndParseService( IN IXML_Document * DescDoc,
                                IN char *location,
                                IN char *serviceType,
                                OUT char **serviceId,
                                OUT char **eventURL,
                                OUT char **controlURL )
{
    int i,
      length,
      found = 0;
    int ret;
    char *tempServiceType = NULL;
    char *baseURL = NULL;
    char *base;
    char *relcontrolURL = NULL,
     *releventURL = NULL;
    IXML_NodeList *serviceList = NULL;
    IXML_Element *service = NULL;

    baseURL = SampleUtil_GetFirstDocumentItem( DescDoc, "URLBase" );

    if( baseURL )
        base = baseURL;
    else
        base = location;

    serviceList = SampleUtil_GetFirstServiceList( DescDoc );
    length = ixmlNodeList_length( serviceList );
    for( i = 0; i < length; i++ ) {
        service = ( IXML_Element * ) ixmlNodeList_item( serviceList, i );
        tempServiceType =
            SampleUtil_GetFirstElementItem( ( IXML_Element * ) service,
                                            "serviceType" );

        if( strcmp( tempServiceType, serviceType ) == 0 ) {
            SampleUtil_Print( "Found service: %s\n", serviceType );
            *serviceId =
                SampleUtil_GetFirstElementItem( service, "serviceId" );
            SampleUtil_Print( "serviceId: %s\n", ( *serviceId ) );
            relcontrolURL =
                SampleUtil_GetFirstElementItem( service, "controlURL" );
            releventURL =
                SampleUtil_GetFirstElementItem( service, "eventSubURL" );

            *controlURL =
                malloc( strlen( base ) + strlen( relcontrolURL ) + 1 );
            if( *controlURL ) {
                ret = UpnpResolveURL( base, relcontrolURL, *controlURL );
                if( ret != UPNP_E_SUCCESS )
                    SampleUtil_Print
                        ( "Error generating controlURL from %s + %s\n",
                          base, relcontrolURL );
            }

            *eventURL =
                malloc( strlen( base ) + strlen( releventURL ) + 1 );
            if( *eventURL ) {
                ret = UpnpResolveURL( base, releventURL, *eventURL );
                if( ret != UPNP_E_SUCCESS )
                    SampleUtil_Print
                        ( "Error generating eventURL from %s + %s\n", base,
                          releventURL );
            }

            if( relcontrolURL )
                free( relcontrolURL );
            if( releventURL )
                free( releventURL );
            relcontrolURL = releventURL = NULL;

            found = 1;
            break;
        }

        if( tempServiceType )
            free( tempServiceType );
        tempServiceType = NULL;
    }

    if( tempServiceType )
        free( tempServiceType );
    if( serviceList )
        ixmlNodeList_free( serviceList );
    if( baseURL )
        free( baseURL );

    return ( found );
}

#endif

// File_checked_for_headers
