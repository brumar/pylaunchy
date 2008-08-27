 TEMPLATE      = lib
 CONFIG       += plugin debug_and_release
 FORMS         = 
 VPATH         = pylaunchy/
 
 HEADERS       = catalog.h plugin_interface.h
 SOURCES       = catalog.cpp plugin_interface.cpp 
 
 HEADERS      += PyLaunchyPlugin.h NullScriptPlugin.h
 SOURCES      += PyLaunchyPlugin.cpp NullScriptPlugin.cpp
 
 HEADERS      += ScriptPlugin.h ScriptPluginWrapper.h
 SOURCES      += ScriptPluginWrapper.cpp
 
 SOURCES      += python_export.cpp python_export_pylaunchy.cpp python_export_qstring.cpp python_export_catalog.cpp python_export_ScriptPlugin.cpp
 
 HEADRES      += Precompiled.h
 SOURCES	  += Precompiled.cpp
 
 TARGET		   = PyLaunchy
 PRECOMPILED_HEADER = Precompiled.h
 
 RESOURCES     = PyLaunchyPlugin.qrc
 
 INCLUDEPATH = $$(PYTHON_ROOT)\include $$(BOOST_1_35_0) ezlogger
 
 win32 {
	DEFINES = _CRT_SECURE_NO_DEPRECATE
 	CONFIG -= embed_manifest_dll
	LIBS += shell32.lib
	LIBS += $$(PYTHON_ROOT)\libs\python25.lib
	LIBS += -L$$(BOOST_1_35_0)\lib
%	LIBS += user32.lib
%	LIBS += Gdi32.lib
%	LIBS += comctl32.lib
}
