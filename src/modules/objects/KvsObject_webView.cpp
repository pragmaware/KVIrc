//=============================================================================
//
//   File : KvsObject_webView.cpp
//   Creation date : Wed Mar 9 23:34:48 CEST 2011
//   by Alessandro Carbone(Noldor)
//
//   This file is part of the KVIrc irc client distribution
//   Copyright (C) 2011 Alessandro Carbone (elfonol at gmail dot com)
//
//   This program is FREE software. You can redistribute it and/or
//   modify it under the terms of the GNU General Public License
//   as published by the Free Software Foundation; either version 2
//   of the License, or (at your opinion) any later version.
//
//   This program is distributed in the HOPE that it will be USEFUL,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program. If not, write to the Free Software Foundation,
//   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=============================================================================
//

#include "kvi_settings.h"
#include "kvi_debug.h"
#include "KvsObject_webView.h"
#include "KvsObject_pixmap.h"
#include "KviError.h"
#include "KviLocale.h"

#if defined(COMPILE_WEBKIT_SUPPORT)
#include <QWebView>
#include <QWebSettings>
#include <QWebElement>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPixmap>
#include <QFile>
#include <QSize>
#include <QPoint>
#include <QVariant>

static int g_iDownloadId = 1;
KviKvsDownloadHandler::KviKvsDownloadHandler(KvsObject_webView * pParent,QFile *pFile,QNetworkReply *pNetReply,int iId)
:QObject(pParent)
{

	m_Id = iId;
	m_pParentScript = pParent;
	m_pReply = pNetReply;
	m_pFile = pFile;
	connect(m_pReply,SIGNAL(finished()),this,SLOT(slotReplyFinished()));
	connect(m_pReply,SIGNAL(readyRead()),this,SLOT(slotReadyRead()));
}

void KviKvsDownloadHandler::slotReplyFinished()
{
	KviKvsVariantList params(new KviKvsVariant((kvs_int_t)m_Id));
	m_pParentScript->callFunction(m_pParentScript,"downloadCompletedEvent",&params);
	m_pFile->close();
	delete m_pFile;
	m_pFile = 0;
	m_pReply->deleteLater();
	m_pReply = 0;
	this->deleteLater();
}

KviKvsDownloadHandler::~KviKvsDownloadHandler()
{
	if(m_pFile)
	{
		m_pFile->close();
		delete m_pFile;
		m_pFile = 0;
	}
	if(m_pReply)
	{
		delete m_pReply;
		m_pReply = 0;
	}
}
void KviKvsDownloadHandler::slotReadyRead()
{
	QVariant vSize = m_pReply->header(QNetworkRequest::ContentLengthHeader);
	int iSize = 0;
	if(!vSize.isNull())
	{
		bool bOk;
		iSize = vSize.toInt(&bOk);
	}

	QByteArray bytes = m_pReply->readAll();
	KviKvsVariantList params(new KviKvsVariant((kvs_int_t)bytes.count()),\
		new KviKvsVariant((kvs_int_t)m_Id),new KviKvsVariant((kvs_int_t)iSize));
	m_pParentScript->callFunction(m_pParentScript,"downloadProgressEvent",&params);
	m_pFile->write(bytes);
}

const char * const webattributes_tbl[] = {
	"JavascriptEnabled",
#if (QT_VERSION >= 0x040700)
	"PluginsEnabled",
#endif
	"JavascriptCanOpenWindows",
	"JavascriptCanAccessClipboard",
	"ZoomTextOnly"
#if (QT_VERSION >= 0x040700)
	,
	"LocalContentCanAccessFileUrls"
#endif
};

const QWebSettings::WebAttribute webattributes_cod[] = {
	QWebSettings::JavascriptEnabled,
#if (QT_VERSION >= 0x040700)
	QWebSettings::PluginsEnabled,
#endif
	QWebSettings::JavascriptCanOpenWindows,
	QWebSettings::JavascriptCanAccessClipboard,
	QWebSettings::ZoomTextOnly
#if (QT_VERSION >= 0x040700)
	,
	QWebSettings::LocalContentCanAccessFileUrls
#endif
};

const char * const findflag_tbl[] = {
	"FindBackward",
	"FindCaseSensitively",
	"FindWrapsAroundDocument",
	"HighlightAllOccurrences"};

const QWebPage::FindFlag findflag_cod[] = {
	QWebPage::FindBackward,
	QWebPage::FindCaseSensitively,
	QWebPage::FindWrapsAroundDocument,
	QWebPage::HighlightAllOccurrences};

#define webattributes_num (sizeof(webattributes_tbl) / sizeof(webattributes_tbl[0]))
#define findflag_num (sizeof(findflag_tbl) / sizeof(findflag_tbl[0]))

/*
	@doc:	webview
	@keyterms:
		An embedded webview widget,
	@title:
		webview
	@type:
		class
	@short:
		Provides web support in a widget using webkit.
	@inherits:
		[class]object[/class]
		[class]widget[/class]
	@description:
		Provides an embedded web browser using webkit. Page structure can be managed by web element's unique identifiers.
	@functions:
		!fn: $load(<url:string>)
		Sets the current url for the webView and starts loading it
		!fn: $findText(<txt:string>,[flag 1,flag 2,..;string])
		Finds the specified string, in the page, using the given options.
		Valid flags are:
		[pre]
		FindBackward		- Searches backwards instead of forwards;
		FindCaseSensitively	- Changes the behaviour to a case sensitive find operation.
		FindWrapsAroundDocument	- Restart from the beginning of the document if the end was reached and the text was not found.
		HighlightAllOccurrences	- Highlights all existing occurrences.
		[/pre]
		!fn: array $frames()
		Returns an array containing the names of the document frames.

		!fn: <integer> $firstChild(<element_id:integer>)
		Return the identifier of element's first childr.

		!fn: <array> $findAll(<element_id:integer>,<query:string>)
		Searches for all the elements named <query> and stores them in an array of element's identifiers.

		!fn: <integer> $findFirst(<element_id:integer>,<query:string>)
		Searches for the first element named <query>; returns the identifier.

		!fn: <integer> $parentElement(<element_id:integer>,)
		Returns the parent of <element_id>.
		!fn: <integer> $nextSibling(<element_id>)
		Returns the element just after <element_id>.
		!fn: $elementTagName(<element_id>)
		Returns the tag name of the <element_id>.
		!fn: <integer> $getDocumentElement([frame_name:string])
		Return as unique identifier the document element of the frame [frame_name].
		If no value has been specified for [frame_name] or [frame_name] is empty, the main frame of the page will be used.
		!fn: string $attributeNames(<element_id>)
		Returns a comma-separated list of the attribute names set on element <element_id>.
		!fn: string $attribute(<element_id>,<name:string>)
		Returns the value of the attribute <name> for element <element_id>.
		!fn: pixmap $makePreview()
		Returns a 212x142 thumbnail of the current webView contants.
		The returned object is an instance of the pixmap class.
		!fn: string $toPlainText(<element_id>)
		Returns the string representation of element <element_id>.
		!fn: string $setPlainText(<element_id>)
		Set the string representation of the  element <element_id>.
		!fn: $setAttribute(<element_id>,<name:string>,<value:string>)
		Sets the attribute <name> with value <value> to the element <element_id>.
		!fn: $setWebSetting(<name:string>,<value:bool>)
		Enables or disables the <name> setting depending on <value>.
		Valid settings name: JavascriptEnabled, PluginsEnabled, JavascriptCanOpenWindows, JavascriptCanAccessClipboard, ZoomTextOnly, LocalContentCanAccessFileUrls.
		!fn: $removeFromDocument(<element_id>)
		Removes the element <element_id> from the document.
		!fn: $removeClass(<element_id>,<class_name:string>)
		Removes a class from the element <element_id>.
		!fn: string $classes(<element_id>)
		Returns a comma-separated list of classes set on the element <element_id>.
		!fn: $setLinkDelegationPolicy(<policy:string>)
		Sets the link delegation policy: what happens when the users click on a link. Valid values:
		[br] DontDelegateLinks: No links are delegated. Instead, webView tries to handle them all.
		[br] DelegateExternalLinks: When activating links that point to documents not stored on the local filesystem or an equivalent then $linkClickedEvent() is executed.
		[br] DelegateAllLinks: Whenever a link is activated the $linkClickedEvent() is executed.	
		!fn: $linkClickedEvent()
		This function can be called when the user clicks on a link, depending no the current link delegation policy.
		The argument of the function is the url that has been clicked.[br]
		The default implementation emits the [classfnc:webview]$linkClicked[/classfnc]() signal.
		!fn: $loadStartedEvent()
		This function is called when the load of the page has started.
		The default implementation emits the [classfnc:webview]$loadStarted[/classfnc]() signal.
		!fn: $loadProgressEvent()
		This function can be called during the page load progress.
		The argument of the function is an int value that represent the loading progress status, ranging from 0 to 100.[br]
		The default implementation emits the [classfnc:webview]$loadProgress[/classfnc]() signal.
		!fn: $loadFinishedEvent()
		This function is called when the load of the page has finished.
		The argument of the function is a bool value that is true if the page has been loaded successfully, false otherwise.[br]
		The default implementation emits the [classfnc:webview]$loadFinished[/classfnc]() signal.
		!fn: $downloadRequestEvent()
		This function is called when the user tries to download a file.
		The argument of the function is the url of the file.[br]
		You should return a valid path in the filesystem where to save the file.[br]
		The default implementation emits the [classfnc:webview]$downloadRequest[/classfnc]() signal.
		!fn: $downloadProgressEvent()
		This function can be called during the download of a file.
		Three integer arguments are passed to this function: the number of downloaded bytes, the download id, the size of the remove file (if known).
		The default implementation emits the [classfnc:webview]$downloadProgress[/classfnc]() signal.
		!fn: $downloadCompletedEvent()
		This function can be called when a file download finishes.
		The argument of the function is the an integer value containing the download id.
		The default implementation emits the [classfnc:webview]$downloadCompleted[/classfnc]() signal.
	@signals:
		!sg: linkClicked()
		This signal is emitted by the default implementation of [classfnc:webview]linkClickedEvent[/classfnc]().

		!sg: loadStarted()
		This signal is emitted by the default implementation of [classfnc:webview]loadStartedEvent[/classfnc]().

		!sg: loadProgress()
		This signal is emitted by the default implementation of [classfnc:webview]loadProgressEvent[/classfnc]().

		!sg: loadFinished()
		This signal is emitted by the default implementation of [classfnc:webview]loadFinishedEvent[/classfnc]().

		!sg: downloadRequest()
		This signal is emitted by the default implementation of [classfnc:webview]downloadRequestEvent[/classfnc]().

		!sg: downloadProgress()
		This signal is emitted by the default implementation of [classfnc:webview]downloadProgressEvent[/classfnc]().

		!sg: downloadCompleted()
		This signal is emitted by the default implementation of [classfnc:webview]downloadCompletedEvent[/classfnc]().
*/


KVSO_BEGIN_REGISTERCLASS(KvsObject_webView,"webview","widget")
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,load)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,frames)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,firstChild)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,findAll)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,findFirst)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,findText)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,parentElement)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,nextSibling)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,elementTagName)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,getDocumentElement)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,attributeNames)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,attribute)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,makePreview)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,elementAt)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,toPlainText)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,setPlainText)

	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,setStyleProperty)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,styleProperty)

	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,setAttribute)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,setWebSetting)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,removeFromDocument)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,removeClass)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,addClass)

	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,classes)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,setLinkDelegationPolicy)

	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,addToJavaScriptWindowObject)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,evaluateJavaScript)

	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,linkClickedEvent)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,loadFinishedEvent)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,loadProgressEvent)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,loadStartedEvent)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,downloadCompletedEvent)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,downloadRequestEvent)
	KVSO_REGISTER_HANDLER_BY_NAME(KvsObject_webView,downloadProgressEvent)
KVSO_END_REGISTERCLASS(KvsObject_webView)

KVSO_BEGIN_CONSTRUCTOR(KvsObject_webView,KviKvsObject)

KVSO_END_CONSTRUCTOR(KvsObject_webView)


KVSO_BEGIN_DESTRUCTOR(KvsObject_webView)


m_elementMapper.clear();
KVSO_END_CONSTRUCTOR(KvsObject_webView)

bool KvsObject_webView::init(KviKvsRunTimeContext *c ,KviKvsVariantList *)
{
	SET_OBJECT(QWebView);
	elementMapId=1;
	m_pContext = c;
	m_pNetworkManager = new QNetworkAccessManager(this);
	QWebPage *pPage = ((QWebView *)widget())->page();
	connect(((QWebView *)widget()),SIGNAL(loadStarted()),this,SLOT(slotLoadStarted()));
	connect(((QWebView *)widget()),SIGNAL(loadFinished(bool)),this,SLOT(slotLoadFinished(bool)));
	connect(((QWebView *)widget()),SIGNAL(loadProgress(int)),this,SLOT(slotLoadProgress(int)));
	connect(pPage,SIGNAL(linkClicked(const QUrl &)),this,SLOT(slotLinkClicked(const QUrl &)));
	connect(pPage,SIGNAL(downloadRequested(const QNetworkRequest &)),this,SLOT(slotDownloadRequest(const QNetworkRequest &)));
	return true;
}
int KvsObject_webView::insertElement(const QWebElement &ele)
{
	int eleid=getElementId(ele);
	if (!eleid)
	{
		m_elementMapper[elementMapId]=ele;
		return ++elementMapId;
	}
	else return eleid;
}
int KvsObject_webView::getElementId(const QWebElement &ele)
{
	QHashIterator<int,QWebElement> it(m_elementMapper);
	while (it.hasNext())
	{
	     it.next();
	     if(it.value()==ele)
		     return it.key();
	}
	return 0;
}

QWebElement KvsObject_webView::getElement(int iIdx)
{
	return m_elementMapper.value(iIdx);
}
void KvsObject_webView::getFrames(QWebFrame *pCurFrame, QStringList &szFramesNames)
{
	szFramesNames.append(pCurFrame->title());
	QList<QWebFrame *> lFrames = pCurFrame->childFrames();
	for(int i=0; i < lFrames.count(); i++)
	{
		QWebFrame *pChildFrame = lFrames.at(i);
		if(pChildFrame->childFrames().count())
			getFrames(pChildFrame,szFramesNames);
	}
}
QWebFrame * KvsObject_webView::findFrame(QWebFrame *pCurFrame, QString &szFrameName)
{
	if(pCurFrame->title()==szFrameName) return pCurFrame;
	QList<QWebFrame *> lFrames = pCurFrame->childFrames();
	for(int i=0; i < lFrames.count(); i++)
	{
		QWebFrame *pChildFrame = lFrames.at(i);
		if(pChildFrame->childFrames().count())
		{
			pCurFrame=findFrame(pChildFrame,szFrameName);
			if(pCurFrame) return pCurFrame;
		}
	}
	return 0;
}

KVSO_CLASS_FUNCTION(webView,setLinkDelegationPolicy)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szPolicy;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("policy",KVS_PT_STRING,0,szPolicy)
	KVSO_PARAMETERS_END(c)
	QWebPage::LinkDelegationPolicy policy = QWebPage::DontDelegateLinks;
	if(KviQString::equalCI(szPolicy,"DontDelegateLinks"))
		policy=QWebPage::DontDelegateLinks;
	else if(KviQString::equalCI(szPolicy,"DelegateExternalLinks"))
		policy=QWebPage::DelegateExternalLinks;
	else if(KviQString::equalCI(szPolicy,"DelegateAllLinks"))
		policy=QWebPage::DelegateAllLinks;
	else
		c->warning(__tr2qs_ctx("Unknown delegation policy '%Q'- Switch do default dontDelegateLinks","objects"),&szPolicy);
	((QWebView *)widget())->page()->setLinkDelegationPolicy(policy);
	return true;
}

KVSO_CLASS_FUNCTION(webView,load)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szUrl;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("url",KVS_PT_STRING,0,szUrl)
	KVSO_PARAMETERS_END(c)
	((QWebView *)widget())->load(QUrl(szUrl));
	return true;
}

KVSO_CLASS_FUNCTION(webView,makePreview)
{
	CHECK_INTERNAL_POINTER(widget())
	QSize size=((QWebView *)widget())->page()->mainFrame()->contentsSize();
	QImage *pImage=new QImage(212,142, QImage::Format_RGB32);
	QWebFrame  *pFrame=((QWebView *)widget())->page()->mainFrame();
	QPainter painter(pImage);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setRenderHint(QPainter::TextAntialiasing, true);
	painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	double dWScale = size.width() > 0 ? (212.0 / (double)(size.width())) : 0;
	double dHScale = dWScale;
	if((size.height() * dWScale) < 142)
	{
		// solution 1 is to fill the missing bacgkround
		painter.fillRect(0,0,212,142,QColor(255,255,255));
		// solution 2 is to stretch the contents, but this doesn't work well because
		// the frame reports a size that doesn't exactly match the document
		//dHScale = 142.0 / (double)(size.height());
	}
	painter.scale(dWScale,dHScale);
	pFrame->documentElement().render(&painter);
	painter.end();
	KviKvsObjectClass * pClass = KviKvsKernel::instance()->objectController()->lookupClass("pixmap");
	KviKvsVariantList params;
	KviKvsObject * pObject = pClass->allocateInstance(0,"internalpixmap",c->context(),&params);
	((KvsObject_pixmap *)pObject)->setInternalImage(pImage);
	c->returnValue()->setHObject(pObject->handle());
	return true;
}

KVSO_CLASS_FUNCTION(webView,findAll)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szQuery;
	KviKvsArray *pArray;
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("element_identifier",KVS_PT_INTEGER,0,iEleId)
		KVSO_PARAMETER("query",KVS_PT_STRING,0,szQuery)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	QWebElementCollection elementCollection=element.findAll(szQuery);
	int idx=0;
	pArray=new KviKvsArray();
	for(int i=0;i<m_webElementCollection.count();i++)
	{
		QWebElement element=elementCollection.at(i);
		int id=getElementId(element);
		pArray->set(idx,new KviKvsVariant((kvs_int_t)id));
		idx++;
	}
	return true;
}

KVSO_CLASS_FUNCTION(webView,evaluateJavaScript)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szScript;
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("element_identifier",KVS_PT_INTEGER,0,iEleId)
		KVSO_PARAMETER("script_code",KVS_PT_STRING,0,szScript)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element is null: you must call getDocumentElement first","objects"));
		return true;
	}
	QVariant vRes=element.evaluateJavaScript(szScript);
	if(vRes.type()==QVariant::String)
	{
		QString szVal=vRes.toString();
		c->returnValue()->setString(szVal);
	}
	else c->warning(__tr2qs_ctx("Unsupported datatype","objects"));
	return true;
}

KVSO_CLASS_FUNCTION(webView,findFirst)
{
	QString szQuery;
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("element_identifier",KVS_PT_INTEGER,0,iEleId)
		KVSO_PARAMETER("query",KVS_PT_STRING,0,szQuery)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	QWebElement tempElement=element.findFirst(szQuery);
	if (tempElement.isNull())
	{
		c->returnValue()->setInteger(-1);
		return true;
	}
	int id=insertElement(tempElement);
	c->returnValue()->setInteger((kvs_int_t) id);
	return true;
}

KVSO_CLASS_FUNCTION(webView,removeFromDocument)
{
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("element_identifier",KVS_PT_INTEGER,0,iEleId)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	element.removeFromDocument();
	return true;
}

KVSO_CLASS_FUNCTION(webView,nextSibling)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("element_identifier",KVS_PT_INTEGER,0,iEleId)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	QWebElement tempElement=element.nextSibling();
	if (tempElement.isNull())
	{
		c->returnValue()->setInteger(-1);
		return true;
	}
	int id=insertElement(tempElement);
	c->returnValue()->setInteger((kvs_int_t) id);
	return true;
}


KVSO_CLASS_FUNCTION(webView,frames)
{
	CHECK_INTERNAL_POINTER(widget())
	QWebFrame *pFrame=((QWebView *)widget())->page()->mainFrame();
	QStringList szFramesNames;
	getFrames(pFrame,szFramesNames);
	KviKvsArray *pArray=new KviKvsArray();
	c->returnValue()->setArray(pArray);
	return true;
}

KVSO_CLASS_FUNCTION(webView,elementTagName)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("element_identifier",KVS_PT_INTEGER,0,iEleId)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	c->returnValue()->setString(element.tagName());
	return true;
}

KVSO_CLASS_FUNCTION(webView,setAttribute)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szName,szValue;
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("element_identifier",KVS_PT_INTEGER,0,iEleId)
		KVSO_PARAMETER("name",KVS_PT_NONEMPTYSTRING,0,szName)
		KVSO_PARAMETER("value",KVS_PT_STRING,0,szValue)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	element.setAttribute(szName,szValue);
	return true;
}

KVSO_CLASS_FUNCTION(webView,removeClass)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szClassName;
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("element_identifier",KVS_PT_INTEGER,0,iEleId)
		KVSO_PARAMETER("class_name",KVS_PT_NONEMPTYSTRING,0,szClassName)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	element.removeClass(szClassName);
	return true;
}
KVSO_CLASS_FUNCTION(webView,addClass)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szClassName;
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("element_identifier",KVS_PT_INTEGER,0,iEleId)
		KVSO_PARAMETER("class_name",KVS_PT_NONEMPTYSTRING,0,szClassName)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	element.addClass(szClassName);
	return true;
}
KVSO_CLASS_FUNCTION(webView,setWebSetting)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szName;
	bool bEnabled;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("name",KVS_PT_NONEMPTYSTRING,0,szName)
		KVSO_PARAMETER("value",KVS_PT_BOOLEAN,0,bEnabled)
	KVSO_PARAMETERS_END(c)
	bool found=false;
	unsigned int j = 0;
	for(; j < webattributes_num; j++)
	{
		if(KviQString::equalCI(szName,webattributes_tbl[j]))
		{
			found=true;
			break;
		}
	}
	if (found)
		((QWebView *)widget())->settings()->setAttribute(webattributes_cod[j],bEnabled);
	else
		c->warning(__tr2qs_ctx("Unknown web setting '%Q'","objects"),&szName);
	return true;
}
KVSO_CLASS_FUNCTION(webView,findText)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szName;
	QStringList szFindFlag;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("find_text",KVS_PT_NONEMPTYSTRING,0,szName)
		KVSO_PARAMETER("find_flag",KVS_PT_STRINGLIST,KVS_PF_OPTIONAL,szFindFlag)
	KVSO_PARAMETERS_END(c)
	int findflag=0;
	int sum=0;
	for ( QStringList::Iterator it = szFindFlag.begin(); it != szFindFlag.end(); ++it )
	{
		findflag = 0;
		for(unsigned int j = 0; j < findflag_num; j++)
		{
			if(KviQString::equalCI((*it), findflag_tbl[j]))
			{
				findflag=findflag_cod[j];
				break;
			}
		}
		if(findflag)
			sum = sum | findflag;
		else
			c->warning(__tr2qs_ctx("Unknown findflag  '%Q'","objects"),&(*it));

	}
	((QWebView *)widget())->findText(szName,(QWebPage::FindFlags)findflag);
	return true;
}
KVSO_CLASS_FUNCTION(webView,attribute)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szName;
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("element_identifier",KVS_PT_INTEGER,0,iEleId)
		KVSO_PARAMETER("name",KVS_PT_NONEMPTYSTRING,0,szName)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	c->returnValue()->setString(element.attribute(szName));
	return true;
}

KVSO_CLASS_FUNCTION(webView,attributeNames)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("identifier",KVS_PT_INTEGER,0,iEleId)
	KVSO_PARAMETERS_END(c)
	QString szAttributeNames;
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	szAttributeNames=element.attributeNames().join(",");
	c->returnValue()->setString(szAttributeNames);
	return true;
}
KVSO_CLASS_FUNCTION(webView,classes)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("identifier",KVS_PT_INTEGER,0,iEleId)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	QString szClasses;
	szClasses=element.classes().join(",");
	c->returnValue()->setString(szClasses);
	return true;
}

KVSO_CLASS_FUNCTION(webView,toPlainText)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("identifier",KVS_PT_INTEGER,0,iEleId)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	c->returnValue()->setString(element.toPlainText());
	return true;
}
KVSO_CLASS_FUNCTION(webView,setPlainText)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szText;
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("identifier",KVS_PT_INTEGER,0,iEleId)
		KVSO_PARAMETER("plaintext",KVS_PT_STRING,0,szText)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	element.setPlainText(szText);
	return true;
}


KVSO_CLASS_FUNCTION(webView,firstChild)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("identifier",KVS_PT_INTEGER,0,iEleId)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}

	QWebElement tempElement=element.firstChild();
	if (tempElement.isNull())
	{
		c->returnValue()->setInteger(-1);
		return true;
	}
	int id=insertElement(tempElement);
	c->returnValue()->setInteger((kvs_int_t) id);
	return true;
}

KVSO_CLASS_FUNCTION(webView,parentElement)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("identifier",KVS_PT_INTEGER,0,iEleId)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}

	QWebElement tempElement=element.parent();
	if (tempElement.isNull())
	{
		c->returnValue()->setInteger(-1);
		return true;
	}
	int id=insertElement(tempElement);
	c->returnValue()->setInteger((kvs_int_t) id);
	return true;
}


KVSO_CLASS_FUNCTION(webView,styleProperty)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szName;
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("identifier",KVS_PT_INTEGER,0,iEleId)
		KVSO_PARAMETER("name",KVS_PT_NONEMPTYSTRING,0,szName)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	c->returnValue()->setString(element.styleProperty(szName,QWebElement::CascadedStyle));
	return true;
}
KVSO_CLASS_FUNCTION(webView,setStyleProperty)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szProperty,szValue;
	kvs_int_t iEleId;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("identifier",KVS_PT_INTEGER,0,iEleId)
		KVSO_PARAMETER("property",KVS_PT_STRING,0,szProperty)
		KVSO_PARAMETER("value",KVS_PT_STRING,0,szValue)
	KVSO_PARAMETERS_END(c)
	QWebElement element=getElement(iEleId);
	if (element.isNull())
	{
		c->warning(__tr2qs_ctx("Document element whith id [%d] does not exists","objects"),&iEleId);
		return true;
	}
	element.setStyleProperty(szProperty,szValue);
	return true;
}

KVSO_CLASS_FUNCTION(webView,getDocumentElement)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szFrameName;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("frame_name",KVS_PT_STRING,KVS_PF_OPTIONAL,szFrameName)
	KVSO_PARAMETERS_END(c)
	QWebFrame *pFrame;
	pFrame=((QWebView *)widget())->page()->mainFrame();
	if(!szFrameName.isEmpty())
	{
		pFrame=findFrame(pFrame,szFrameName);
		if(!pFrame)
		{
			c->warning(__tr2qs_ctx("Unknown frame '%Q'","objects"),&szFrameName);
			return true;
		}
	}
	int id=insertElement(pFrame->documentElement());
	c->returnValue()->setInteger((kvs_int_t) id);
	return true;
}
KVSO_CLASS_FUNCTION(webView,addToJavaScriptWindowObject)
{
	CHECK_INTERNAL_POINTER(widget())
	QString szFrameName, szObjectName;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("object_name",KVS_PT_NONEMPTYSTRING,0,szObjectName)
		KVSO_PARAMETER("frame_name",KVS_PT_STRING,KVS_PF_OPTIONAL,szFrameName)
	KVSO_PARAMETERS_END(c)
	QWebFrame *pFrame;
	pFrame=((QWebView *)widget())->page()->mainFrame();
	if(!szFrameName.isEmpty())
	{
		pFrame=findFrame(pFrame,szFrameName);
		if(!pFrame)
		{
			c->warning(__tr2qs_ctx("Unknown frame '%Q'","objects"),&szFrameName);
			return true;
		}
	}
	pFrame->addToJavaScriptWindowObject(szObjectName,this);
	return true;
}

KVSO_CLASS_FUNCTION(webView,elementAt)
{
	CHECK_INTERNAL_POINTER(widget())
	kvs_int_t iYPos,iXPos;
	KVSO_PARAMETERS_BEGIN(c)
		KVSO_PARAMETER("x_pos",KVS_PT_INT,0,iXPos)
		KVSO_PARAMETER("y_pos",KVS_PT_INT,0,iYPos)
	KVSO_PARAMETERS_END(c)
	QWebFrame *pFrame;
	pFrame=((QWebView *)widget())->page()->mainFrame();
	QWebElement tmpElement=pFrame->hitTestContent(QPoint(iXPos,iYPos)).element();
	int id=insertElement(tmpElement);
	c->returnValue()->setInteger((kvs_int_t) id);
	return true;
}
KVSO_CLASS_FUNCTION(webView,loadFinishedEvent)
{
	emitSignal("loadFinished",c,c->params());
	return true;
}

KVSO_CLASS_FUNCTION(webView,loadProgressEvent)
{
	emitSignal("loadProgress",c,c->params());
	return true;
}

KVSO_CLASS_FUNCTION(webView,loadStartedEvent)
{
	emitSignal("loadStarted",c);
	return true;
}

KVSO_CLASS_FUNCTION(webView,downloadCompletedEvent)
{
	emitSignal("downloadCompleted",c,c->params());
	return true;
}

KVSO_CLASS_FUNCTION(webView,downloadProgressEvent)
{
	emitSignal("downloadProgress",c,c->params());
	return true;
}

KVSO_CLASS_FUNCTION(webView,linkClickedEvent)
{
	emitSignal("linkClicked",c,c->params());
	return true;
}

KVSO_CLASS_FUNCTION(webView,downloadRequestEvent)
{
	emitSignal("downloadRequest",c,c->params());
	return true;
}

KVSO_CLASS_FUNCTION(webView,jsSubmitEvent)
{
	emitSignal("jsSubmit",c,c->params());
	return true;
}
/*KVSO_CLASS_FUNCTION(webView,jsWindowObjectClearedEvent)
{
	emitSignal("jsWindowObjectCleared",c,c->params());
	return true;
}*/
// slots

void KvsObject_webView::slotLoadFinished(bool bOk)
{
	if (bOk)
		m_currentElement=((QWebView *)widget())->page()->mainFrame()->documentElement();
	KviKvsVariantList params(new KviKvsVariant(bOk));
	callFunction(this,"loadFinishedEvent",&params);
}

void KvsObject_webView::submit()
{
	KviKvsVariantList *lParams=0;
	qDebug("Submit");
	callFunction(this,"jsSubmitEvent",lParams);
}

void KvsObject_webView::slotLoadStarted()
{
	KviKvsVariantList *lParams=0;
	callFunction(this,"loadStartedEvent",lParams);
}

void KvsObject_webView::slotLoadProgress(int iProgress)
{
	KviKvsVariantList params(new KviKvsVariant((kvs_int_t) iProgress));
	callFunction(this,"loadProgressEvent" ,&params);
}

void KvsObject_webView::slotLinkClicked(const QUrl &url)
{
	QString szUrl=url.toString();
	KviKvsVariantList params(new KviKvsVariant(szUrl));
	callFunction(this,"linkClickedEvent" ,&params);
}
/*void KvsObject_webView::javaScriptWindowObjectCleared(const QUrl &url)
{
	//KviKvsVariantList *lParams=0;
	//callFunction(this,"jsWindowObjectClearedEvent",lParams);
	addToJavaScriptWindowObject
}*/

void KvsObject_webView::slotDownloadRequest(const QNetworkRequest &r)
{
	QNetworkReply *pReply=m_pNetworkManager->get(r);
	QString szFilePath="";
	KviKvsVariant *filepathret=new KviKvsVariant(szFilePath);
	KviKvsVariantList params(new KviKvsVariant(r.url().toString()));
	callFunction(this,"downloadRequestEvent",filepathret,&params);
	filepathret->asString(szFilePath);
	if (!szFilePath.isEmpty())
	{
		QFile *pFile=new QFile(szFilePath);
		if(!pFile->open(QIODevice::WriteOnly))
		{
			m_pContext->warning(__tr2qs_ctx("Invalid file path '%Q'","objects"),&szFilePath);
			pReply->abort();
			pReply->deleteLater();
			return;
		}
		KviKvsDownloadHandler *pHandler = new KviKvsDownloadHandler(this,pFile,pReply,g_iDownloadId);
		Q_UNUSED(pHandler);
		g_iDownloadId++;
	}
}

#endif // COMPILE_WEBKIT_SUPPORT
