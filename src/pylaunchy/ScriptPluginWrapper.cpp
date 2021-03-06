#include "Precompiled.h"
#include "ScriptPluginWrapper.h"

#include "ScriptPlugin.h"
#include "ScriptPluginsSynchronizer.h"
#include "PythonUtils.h"

/** Convert QList<InputData> -> ScriptInputDataList */
static ScriptInputDataList prepareInputDataList(QList<InputData>* id);

ScriptPluginWrapper::ScriptPluginWrapper(
	ScriptPlugin* scriptPlugin,
	ScriptPluginsSynchronizer& scriptPluginsSynchronizer)
: m_pScriptPlugin(scriptPlugin),
  m_scriptPluginsSynchronizer(scriptPluginsSynchronizer)
{
}

ScriptPluginWrapper::~ScriptPluginWrapper()
{
}

void ScriptPluginWrapper::getID(uint* id)
{
	LOG_FUNCTRACK;
	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin getID");
		*id = m_pScriptPlugin->getID();
	);
}

void ScriptPluginWrapper::getName(QString* str)
{
	LOG_FUNCTRACK;
	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin getName");
		*str = m_pScriptPlugin->getName();
	);
}

void ScriptPluginWrapper::init()
{
	LOG_FUNCTRACK;
	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin init");
		m_pScriptPlugin->init();
	);
}

void ScriptPluginWrapper::getLabels(QList<InputData>* id)
{
	LOG_FUNCTRACK;

	ScriptInputDataList inputDataList(prepareInputDataList(id));

	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin getLabels");
		m_pScriptPlugin->getLabels(inputDataList);
	);
}

void ScriptPluginWrapper::getResults(QList<InputData>* id, QList<CatItem>* results)
{
	LOG_FUNCTRACK;

	ScriptInputDataList inputDataList(prepareInputDataList(id));
	ScriptResultsList scriptResults(*results);

	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin getResults");
		m_pScriptPlugin->getResults(inputDataList, scriptResults);
	);

	
}

void ScriptPluginWrapper::getCatalog(QList<CatItem>* items)
{
	LOG_FUNCTRACK;

	ScriptResultsList scriptResults(*items);

	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin getCatalog");
		m_pScriptPlugin->getCatalog(scriptResults);
	);
	
	
}

void ScriptPluginWrapper::launchItem(QList<InputData>* id, CatItem* item)
{
	LOG_FUNCTRACK;
	
	//CatItem& topResult = id->last().getTopResult();
	//item = &topResult;

	ScriptInputDataList inputDataList(prepareInputDataList(id));

	LOG_DEBUG("Begining to launch item");

	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin launchItem");
		m_pScriptPlugin->launchItem(inputDataList, *item);
	);

	LOG_DEBUG("Finished launching item");
	
	
}

bool ScriptPluginWrapper::hasDialog()
{
	LOG_FUNCTRACK;
	bool hasDialog = false;

	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin hasDialog");
		hasDialog = m_pScriptPlugin->hasDialog();
	);

	return hasDialog;
}

void ScriptPluginWrapper::doDialog(QWidget* parent, QWidget** newDlg) 
{
	LOG_FUNCTRACK;
	m_scriptPluginsSynchronizer.enteringDoDialog();

	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin doDialog");
		void* result = m_pScriptPlugin->doDialog((void*)parent);
		if (result) {
			*newDlg = reinterpret_cast<QWidget*>(result);
		} else {
			LOG_DEBUG("doDialog returned NULL");
			*newDlg = NULL;
		}
	);
	
	
}

void ScriptPluginWrapper::endDialog(bool accept) 
{
	LOG_FUNCTRACK;

	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin endDialog");
		m_pScriptPlugin->endDialog(accept);
	);
	
	m_scriptPluginsSynchronizer.finishedEndDialog();
}

void ScriptPluginWrapper::launchyShow() 
{
	LOG_FUNCTRACK;

	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin launchyShow");
		m_pScriptPlugin->launchyShow();
	);
	
	
}

void ScriptPluginWrapper::launchyHide() 
{
	LOG_FUNCTRACK;

	GUARDED_CALL_TO_PYTHON(
		LOG_DEBUG("Calling plugin launchyHide");
		m_pScriptPlugin->launchyHide();
	);
}

int ScriptPluginWrapper::msg(int msgId, void* wParam, void* lParam)
{
	if (!m_pScriptPlugin) {
		LOG_WARN("Called ScriptPluginWrapper, but it has no script plugin");
		return false;
	}

	// Handle locks
		
	// We can't allow Python functions to be called when doDialog was called
	// and endDialog not called yet, since it results in a crash.
	// The following tests for it
	const bool waitingForEndDialog = 
		msgId != MSG_END_DIALOG &&
		!m_scriptPluginsSynchronizer.tryLockDialogMutex();

	if ( waitingForEndDialog ) {
		LOG_DEBUG("A doDialog--endDialog sequence has not ended yet. "
			"Message id: %i", msgId);
		return false;
	}
	m_scriptPluginsSynchronizer.unlockDialogMutex(); // doDialog will lock it

	// If a python functions is called while another one hasn't returned yet,
	// a crash happens. This is a way to avoid it, not sure it's the best way.
	const bool waitingForPythonFunctionToReturn = 
		!m_scriptPluginsSynchronizer.tryLockInPythonMutex();

	if ( waitingForPythonFunctionToReturn ) { 
		LOG_DEBUG("Trying to call a Python function while another one hasn't "
			"returned yet. Message id: %i", msgId);
		return false; 
	} 
	
	// Disptach the actual Python function
	const bool result = dispatchFunction(msgId, wParam, lParam);
	
	m_scriptPluginsSynchronizer.unlockInPythonMutex();
	return result;
}

bool ScriptPluginWrapper::isInPythonFunction() const
{
	const bool waitingForPythonFunctionToReturn = 
		!m_scriptPluginsSynchronizer.tryLockInPythonMutex();

	if (waitingForPythonFunctionToReturn) {
		m_scriptPluginsSynchronizer.unlockInPythonMutex();
	}
	return waitingForPythonFunctionToReturn;
}

bool ScriptPluginWrapper::dispatchFunction(int msgId, void* wParam, void* lParam)
{
	bool handled = false;

	switch (msgId)
	{		
		case MSG_INIT:
			init();
			handled = true;
			break;
		case MSG_GET_LABELS:
			getLabels((QList<InputData>*) wParam);
			handled = true;
			break;
		case MSG_GET_ID:
			getID((uint*) wParam);
			handled = true;
			break;
		case MSG_GET_NAME:
			getName((QString*) wParam);
			handled = true;
			break;
		case MSG_GET_RESULTS:
			getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
			handled = true;
			break;
		case MSG_GET_CATALOG:
			getCatalog((QList<CatItem>*) wParam);
			handled = true;
			break;
		case MSG_LAUNCH_ITEM:
			launchItem((QList<InputData>*) wParam, (CatItem*) lParam);
			handled = true;
			break;
		case MSG_HAS_DIALOG:
			// Set to true if you provide a gui
			handled = hasDialog();
			break;
		case MSG_DO_DIALOG:
			// This isn't called unless you return true to MSG_HAS_DIALOG
			doDialog((QWidget*) wParam, (QWidget**) lParam);
			break;
		case MSG_END_DIALOG:
			// This isn't called unless you return true to MSG_HAS_DIALOG
			endDialog((bool) wParam);
			break;
		case MSG_LAUNCHY_SHOW:
			handled = true;
			launchyShow();
			break;
		case MSG_LAUNCHY_HIDE:
			handled = true;
			launchyHide();
			break;

		default:
			break;
	}

	return handled;
}

ScriptInputDataList prepareInputDataList(QList<InputData>* id)
{
	ScriptInputDataList inputDataList;
	QList<InputData>::iterator itr = id->begin();
	for ( ; itr != id->end(); ++itr) {
		inputDataList.push_back(&(*itr));
	}
	return inputDataList;
}