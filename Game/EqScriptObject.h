//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Scripted object
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQSCRIPTOBJECT_H
#define EQSCRIPTOBJECT_H

#include "gmThread.h"
#include "EntityDataField.h"



struct scriptobj_t
{

};

class CEqScriptObject
{
public:
	static void RegisterScriptBindings();           // Register game object script bindings

	CEqScriptObject();
	virtual ~CEqScriptObject();

	gmTableObject*	GetTableObject()                 { return m_tableObject; }
	gmUserObject*	GetUserObject();

	void			ExecuteStringOnThis(const char* a_string);

	bool			ExecuteGlobalFunctionOnThis(const char* a_functionName);

	void			SetMemberInt(const char* a_memberName, int a_int);
	void			SetMemberFloat(const char* a_memberName, float a_float);
	void			SetMemberString(const char* a_memberName, const char* a_string, int a_strLength = -1);

	void			SetMemberVar(const char* a_memberName, EVariableType type, void* value);

	gmTableObject*	SetMemberTable(const char* a_memberName);

	bool			GetMemberInt(const char* a_memberName, int& a_int);
	bool			GetMemberFloat(const char* a_memberName, float& a_float);
	bool			GetMemberString(const char* a_memberName, EqString& a_string);
	bool			GetMemberVar(const char* a_memberName, EVariableType type, void* value);

	bool			GetMemberTable(const char* a_memberName, gmTableObject*& a_retTable);
	
protected:
	gmUserObject*	m_userObject;                    // The script object
	gmTableObject*	m_tableObject;                   // Table functionality for script object members
};


#define DECLARE_SCRIPTOBJ()
	static CEqScriptObject* m_spScriptObject; \
	template <typename T> friend CEqScriptObject* ScriptObject_Register(T *);

#define BEGIN_DATAMAP_GUTS( className ) \
	template <typename T> CEqScriptObject* ScriptObject_Register(T *); \
	template <> CEqScriptObject* ScriptObject_Register<className>( className * ); \
	namespace className##_ScriptObjectInit /* namespaces are ideal for this */ \
	{ \
		CEqScriptObject* g_ScriptObjectHolder = DataMapInit( (className *)NULL );  \
	} \
	\
	template <> datamap_t *DataMapInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static datavariant_t dataDesc[] = \
		{ \
			{ VTYPE_VOID, NULL, 0,0,0},	/* An empty element because we can't have empty array */


#endif // EQSCRIPTOBJECT_H