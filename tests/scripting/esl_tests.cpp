//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2021
//////////////////////////////////////////////////////////////////////////////////
// Description: Lua object
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConCommand.h"
#include "common/drvsyn_engine_common.h"
#include "luaobj.h"
#include "LuaBinding.h"
struct KVSection;

#include "scripting/esl.h"
#include "scripting/esl_luaref.h"
#include "scripting/esl_bind.h"


//---------------------------------------------------

struct Spline
{
	virtual ~Spline() = default;
	virtual void Load(const KVSection* section)
	{
		Msg("Load Base!\n");
	}
	virtual void Save(KVSection* section)
	{
		Msg("Save Base!\n");
	}

	void NonVirtualFunc()
	{
		Msg("Called non-virtual!\n");
	}

	EqString	name;
	ushort		connections[2]{ 0xffff };	// connected roads or junctions
	Vector3D	position;
	float		angle;
	float		length;
	int			vehicleZoneNameHash{ 0 };	// hash for vehicle zone name. 0 means default.
	float		speedLimit{ 50.0f };
	int16		numLanes{ 0 };
	int16		laneDirs{ 0 };				// lane dir flags (oncoming = 1)
	int16		laneDisabled{ 0 };
	int16		flags{ 0 };					// EStraightFlags
};

// Inheritance test only
struct TerrainSpline : public Spline
{
	TerrainSpline() = default;
	TerrainSpline(int value, const char* message, Spline& other)
	{
		other.Save(nullptr);
		MsgWarning("TerrainSpline::Ctor variant called, arg %d, %s, other name: %s\n", value, message, other.name.ToCString());
		name = message;
		vehicleZoneNameHash = value;
	}

	Vector2D GetPoint(int index, const Vector2D& from, const EqString& str)
	{ 
		ASSERT(str.Length() > 0);
		MsgWarning("TerrainSpline::GetPoint called, arg %d, vehicleZoneNameHash = %d, from = %.2f, %.2f, %s\n", index, vehicleZoneNameHash, from.x, from.y, str.ToCString());
		return position.xz();// points[index];
	}

	void Load(const KVSection* section) override
	{
		Msg("Load Override!\n");
	}
	void Save(KVSection* section) override
	{
		Msg("Save Override!\n");
	}

	void Save(KVSection* section, bool useTest)
	{
		Msg("Save that hides override!\n");
	}

	Array<Vector3D> points{ PP_SL };
};

DECLARE_CMD(test_classReg, nullptr, 0)
{
	EqScriptState state(GetLuaState());

	static const char* typesStr[] = {
		"null",
		"const",
		"dtor",
		"ctor",
		"function",
		"var",
	};

	// registrator test of class with push as ByVal
	{
		esl::TypeInfo typeInfo = EqScriptClass<Vector2D>::GetTypeInfo();

		Msg("class %s : %s\n", typeInfo.className, typeInfo.baseClassName);
		for (const esl::Member& mem : typeInfo.members)
		{
			if (mem.type == esl::MEMB_CTOR)
				Msg("  %s(%s)\n", mem.name, mem.signature);
			else if (mem.type == esl::MEMB_VAR)
				Msg("  %s %s : %s\n", typesStr[mem.type], mem.name, mem.signature);
			else if (mem.type == esl::MEMB_FUNC)
				Msg("  %s %s(%s) %s\n", typesStr[mem.type], mem.name, mem.signature, mem.isConst ? "const" : "");
		}
		Msg("Members: %d\n", typeInfo.members.numElem());

		esl::RegisterType(state, typeInfo);
	}

	// registrator test
	{
		esl::TypeInfo typeInfo = EqScriptClass<Spline>::GetTypeInfo();

		Msg("class %s : %s\n", typeInfo.className, typeInfo.baseClassName);
		for (const esl::Member& mem : typeInfo.members)
		{
			if (mem.type == esl::MEMB_CTOR)
				Msg("  %s(%s)\n", mem.name, mem.signature);
			else if (mem.type == esl::MEMB_VAR)
				Msg("  %s %s : %s\n", typesStr[mem.type], mem.name, mem.signature);
			else if (mem.type == esl::MEMB_FUNC)
				Msg("  %s %s(%s) %s\n", typesStr[mem.type], mem.name, mem.signature, mem.isConst ? "const" : "");
		}
		Msg("Members: %d\n", typeInfo.members.numElem());

		esl::RegisterType(state, typeInfo);
	}

	// registrator test with base class
	{
		esl::TypeInfo typeInfo = EqScriptClass<TerrainSpline>::GetTypeInfo();

		Msg("class %s : %s\n", typeInfo.className, typeInfo.baseClassName);
		for (const esl::Member& mem : typeInfo.members)
		{
			if (mem.type == esl::MEMB_CTOR)
				Msg("  %s(%s)\n", mem.name, mem.signature);
			else if (mem.type == esl::MEMB_VAR)
				Msg("  %s %s : %s\n", typesStr[mem.type], mem.name, mem.signature);
			else if (mem.type == esl::MEMB_FUNC)
				Msg("  %s %s(%s) %s\n", typesStr[mem.type], mem.name, mem.signature, mem.isConst ? "const" : "");
		}
		Msg("Members: %d\n", typeInfo.members.numElem());

		esl::RegisterType(state, typeInfo);
	}

	// TEST: default constructor
	if (!state.RunChunk("spl=Spline.new()"))
	{
		MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(state));
	}

	// TEST: optional constructor
	if (!state.RunChunk("spl=TerrainSpline.new(333, \"Hello from Universe\", spl)"))
	{
		MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(state));
	}

	// TEST: constructor call with upcasting argument: TerrainSpline to Spline
	if (!state.RunChunk("spl=TerrainSpline.new(1337, \"BaseClass check\", spl)"))
	{
		MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(state));
	}

	// TEST: property newindex (setter)
	if (!state.RunChunk("spl.vehicleZoneNameHash = 555"))
	{
		MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(state));
	}

	// TEST: property index (getter)
	if (!state.RunChunk("Msg(\"expected: 555\", spl.vehicleZoneNameHash)"))
	{
		MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(state));
	}

	// TEST: adding methods to metatable
	if (!state.RunChunk("TerrainSpline.Try = function(self, v) return v end;Msg(\"expected: 120\", spl:Try(120))"))
	{
		MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(state));
	}

	// TEST: function call with arguments
	if (!state.RunChunk("spl:GetPoint(1337, vec2.new(0, 68), \"Hello from Lua\")"))
	{
		MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(state));
	}

	// TEST: should fail on nil
	if (!state.RunChunk("spl:GetPoint(666, nil, \"From Russia with NIL\")"))
	{
		MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(state));
	}

	// TEST: should fail on incorrect userdata
	if (!state.RunChunk("spl:GetPoint(1900 / 2, 8192, \"Whoops\")"))
	{
		MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(state));
	}
}

#if 0

//---------------------------------------------------
// Class registrator source
// macros:
//  EQSCRIPT_BIND_TYPE_NO_PARENT
//  EQSCRIPT_BEGIN_BIND
//  EQSCRIPT_BIND_CONSTRUCTOR
//  EQSCRIPT_BIND_FUNC*
//  EQSCRIPT_BIND_VAR
//  EQSCRIPT_END_BIND
EQSCRIPT_ALIAS_TYPE(Vector2D, "vec2")
template<> struct esl::LuaTypeByVal<Vector2D> : std::true_type {};
template<> bool EqScriptClass<Vector2D>::isByVal = esl::LuaTypeByVal<Vector2D>::value;
template<> const char EqScriptClass<Vector2D>::className[] = "vec2";
template<> const char* EqScriptClass<Vector2D>::baseClassName = nullptr;
template<> esl::TypeInfo EqScriptClass<Vector2D>::baseClassTypeInfo = {};
namespace esl::bindings {
	template<> ArrayCRef<Member> ClassBinder<Vector2D>::GetMembers() {
		esl::bindings::BaseClassStorage::Add<BindClass>();
		static Member members[] = {
			MakeDestructor(),
			MakeConstructor(),
			MakeConstructor<float>(),
			MakeConstructor<float, float>(),
			MakeConstructor<const Vector2D&>(),
			MakeVariable<&BindClass::x>("x"),
			MakeVariable<&BindClass::y>("y"),
			MakeOperator<binder::OP_add>("__add"),
			MakeOperator<binder::OP_sub>("__sub"),
			MakeOperator<binder::OP_mul>("__mul"),
			MakeOperator<binder::OP_div>("__div"),
			MakeOperator<binder::OP_eq>("__eq"),
			MakeOperator<binder::OP_lt>("__lt"),
			MakeOperator<binder::OP_le>("__le"),
			{} // default/end element
		};
		return ArrayCRef<Member>(members, elementsOf(members) - 1);
	}
}

//---------------------------------------------------
// Class registrator source
EQSCRIPT_ALIAS_TYPE(Spline, "Spline")
template<> const char EqScriptClass<Spline>::className[] = "Spline";
template<> bool EqScriptClass<Spline>::isByVal = esl::LuaTypeByVal<Spline>::value;
template<> const char* EqScriptClass<Spline>::baseClassName = nullptr;
template<> esl::TypeInfo EqScriptClass<Spline>::baseClassTypeInfo = {};
namespace esl::bindings {
template<> ArrayCRef<Member> ClassBinder<Spline>::GetMembers() {
	esl::bindings::BaseClassStorage::Add<BindClass>();
	static Member members[] = {
		MakeDestructor(),
		MakeConstructor(),
		MakeFunction(&BindClass::Load, "Load"),
		MakeFunction(&BindClass::Save, "Save"),
		MakeVariable<&BindClass::name>("name"),
		MakeVariable<&BindClass::position>("position"),
		MakeVariable<&BindClass::angle>("angle"),
		MakeVariable<&BindClass::length>("length"),
		MakeVariable<&BindClass::vehicleZoneNameHash>("vehicleZoneNameHash"),
		MakeVariable<&BindClass::speedLimit>("speedLimit"),
		MakeVariable<&BindClass::numLanes>("numLanes"),
		MakeVariable<&BindClass::laneDirs>("laneDirs"),
		MakeVariable<&BindClass::laneDisabled>("laneDisabled"),
		MakeVariable<&BindClass::flags>("flags"),
		{} // default/end element
	};
	return ArrayCRef<Member>(members, elementsOf(members) - 1);
}
}


//---------------------------------------------------
// Class registrator source
EQSCRIPT_ALIAS_TYPE(TerrainSpline, "TerrainSpline")
template<> const char EqScriptClass<TerrainSpline>::className[] = "TerrainSpline";
template<> bool EqScriptClass<TerrainSpline>::isByVal = esl::LuaTypeByVal<TerrainSpline>::value;
template<> const char* EqScriptClass<TerrainSpline>::baseClassName = EqScriptClass<Spline>::className;
template<> esl::TypeInfo EqScriptClass<TerrainSpline>::baseClassTypeInfo(EqScriptClass<Spline>::GetTypeInfo());
namespace esl::bindings {
template<> ArrayCRef<Member> ClassBinder<TerrainSpline>::GetMembers() {
	esl::bindings::BaseClassStorage::Add<BindClass>();
	static Member members[] = {
		MakeDestructor(),
		MakeConstructor(),
		MakeConstructor<int, const char*, Spline&>(),
		MakeFunction(&BindClass::GetPoint, "GetPoint"),
		MakeFunction<void, KVSection*, bool>(&BindClass::Save, "Save"),
		MakeFunction<void, KVSection*>(&BindClass::Save, "SaveStandard"),
		{} // default/end element
	};
	return ArrayCRef<Member>(members, elementsOf(members) - 1); 
}
}
#else

EQSCRIPT_BIND_TYPE_NO_PARENT(Spline, "Spline", BY_REF)
EQSCRIPT_BEGIN_BIND(Spline)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_FUNC(Load)
	EQSCRIPT_BIND_FUNC(Save)
	EQSCRIPT_BIND_FUNC(NonVirtualFunc)
	EQSCRIPT_BIND_VAR(name)
	EQSCRIPT_BIND_VAR(position)
	EQSCRIPT_BIND_VAR(angle)
	EQSCRIPT_BIND_VAR(length)
	EQSCRIPT_BIND_VAR(vehicleZoneNameHash)
	EQSCRIPT_BIND_VAR(speedLimit)
	EQSCRIPT_BIND_VAR(numLanes)
	EQSCRIPT_BIND_VAR(laneDirs)
	EQSCRIPT_BIND_VAR(laneDisabled)
	EQSCRIPT_BIND_VAR(flags)
EQSCRIPT_END_BIND

EQSCRIPT_BIND_TYPE_WITH_PARENT(TerrainSpline, Spline, "TerrainSpline")
EQSCRIPT_BEGIN_BIND(TerrainSpline)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(int, const char*, Spline&)
	EQSCRIPT_BIND_FUNC(GetPoint)
	EQSCRIPT_BIND_FUNC_OVERLOAD(Save, void, (KVSection*, bool), ESL_APPLY_TRAITS(void, esl::ToCpp<KVSection*)>, bool)
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SaveStandard", Save, void, (KVSection*))
EQSCRIPT_END_BIND
#endif


//---------------------------------------------

DECLARE_CMD(test_valueGetter, nullptr, 0)
{
	EqScriptState state(GetLuaState());

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	state.RunChunk("TestFunc = function(intVal, strVal) MsgInfo(intVal, strVal, \"\\n\"); return 1500.0 / 3.0 end");
	state.RunChunk("TestValue = Spline.new()");
	state.RunChunk("TestValueUpcast = TerrainSpline.new()");
	state.RunChunk("TestNilValue = nil");
	state.RunChunk("TestNumberValue = 768127");
	state.RunChunk("TestStringValue = \"chars\"");

	// TEST: get function value
	{
		auto result = state.GetGlobal<esl::LuaFunctionRef>("TestFunc");
		ASSERT(result && result.value.IsValid());
	}

	// TEST: get spline value
	{
		auto result = state.GetGlobal<Spline*>("TestValue");
		ASSERT(result && result.value != nullptr);
	}

	// TEST: get upcasted spline value (TerrainSpline as Spline)
	{
		auto result = state.GetGlobal<Spline*>("TestValueUpcast");
		ASSERT(result && result.value != nullptr);
	}

	// TEST: get downcasted value failure (should be null)
	{
		auto result = state.GetGlobal<TerrainSpline*>("TestValue");
		ASSERT(!result);
	}

	// TEST: get nil value failure (should be null)
	{
		auto result = state.GetGlobal<TerrainSpline*>("TestNilValue");
		ASSERT(!result);
	}

	// TEST: get number values
	{
		auto result = state.GetGlobal<int8>("TestNumberValue");
		ASSERT(result.value == int8(768127));
	}

	{
		auto result = state.GetGlobal<int16>("TestNumberValue");
		ASSERT(result.value == int16(768127));
	}

	{
		auto result = state.GetGlobal<int32>("TestNumberValue");
		ASSERT(result.value == int32(768127));
	}
}

DECLARE_CMD(test_funcCall, nullptr, 0)
{
	EqScriptState state(GetLuaState());

	state.RunChunk("TestFunc = function(intVal, strVal) MsgInfo(intVal, strVal, \"\\n\"); return 1500.0 / 3.0 end");
	state.RunChunk("TestFunc2 = function() MsgInfo(\"Simple function call\\n\") end");
	state.RunChunk("TestFunc4 = function() r = a + b end");

	// TEST: function with return value and arguments
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc");

		using TestFunc = esl::runtime::FunctionCall<float, int, const char*>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult, 1337, "Hello from C++ to Lua");
		if (result)
			Msg("invoke result: %.2f\n", result.value);
		else
			MsgError("Error while executing function TestFunc: %s\n", result.errorMessage.ToCString());
	}
	
	// TEST: function without return value
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc2");

		using TestFunc = esl::runtime::FunctionCall<void>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult);

		if (result)
			Msg("invoke success\n");
		else
			MsgError("Error while executing function TestFunc2: %s\n", result.errorMessage.ToCString());
	}

	// TEST: function calling nil
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc3");

		using TestFunc = esl::runtime::FunctionCall<void>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult);

		if (result)
			Msg("invoke success\n");
		else
			MsgError("Error while executing function TestFunc3: %s\n", result.errorMessage.ToCString());
	}

	// TEST: function calling with error
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc4");

		using TestFunc = esl::runtime::FunctionCall<void>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult);

		if (result)
			Msg("invoke success\n");
		else
			MsgError("Error while executing function TestFunc4: %s\n", result.errorMessage.ToCString());
	}
}

DECLARE_CMD(test_tables, nullptr, 0)
{
	EqScriptState state(GetLuaState());

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	state.RunChunk("TestTable = { keyA = \"string AAB value\", num = 1657 }");

	lua_State* L = GetLuaState();
	// TEST: get table and check if it's valid
	{
		esl::LuaTable tableRes = *state.GetGlobal<esl::LuaTable>("TestTable");
		ASSERT(tableRes.IsValid());
	}

	// TEST: get a value from table
	{
		esl::LuaTable tableRes = *state.GetGlobal<esl::LuaTable>("TestTable");
		ASSERT(tableRes.IsValid());

		const EqString str = *tableRes.Get<EqString>("keyA");
		const int val = *tableRes.Get<int>("num");
		ASSERT(str == "string AAB value");
		ASSERT(val == 1657);
		Msg("Table has values %s, %d\n", str.ToCString(), val);
	}

	// TEST: set a value in table
	{
		auto tableRes = state.GetGlobal<esl::LuaTable>("TestTable");
		ASSERT(tableRes && tableRes.value.IsValid());

		Spline* temp = PPNew Spline();
		temp->name = "Some name";

		tableRes.value.Set("valueNum", 1337);
		tableRes.value.Set("stringValue", "This is a string set from C++");
		tableRes.value.Set(temp, "Spline as a key");
		tableRes.value.Set("testSpline", temp);

		const int val = *tableRes.value.Get<int>("valueNum");
		const EqString str = *tableRes.value.Get<EqString>("stringValue");
		const EqString str2 = *tableRes.value.Get<EqString>(temp);

		ASSERT(val == 1337);
		ASSERT(str == "This is a string set from C++");

		// This will fail because the value is boxed and we always pushing new userdata.
		// We need somehow to make comparison mechanism to do this between native and Lua.
		// Doing this all in Lua is pretty fine.
		// Also CRefPtr would work fine at this point
		//ASSERT(str2 != "Spline as a key");
		ASSERT(str2.Length() == 0);

		Msg("Table has values %s, %d\n", str.ToCString(), val);
	}

	// TEST: set a function in table and call it with self as a table
	{
		if(!state.RunChunk("TempFunction = function(self, arg2) MsgInfo(\"Simple function call with self -\", self.valueNum, arg2,\"\\n\") end"))
		{
			MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(GetLuaState()));
		}

		auto tableRes = state.GetGlobal<esl::LuaTable>("TestTable");
		ASSERT(tableRes && tableRes.value.IsValid());

		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TempFunction");

		tableRes.value.Set("func", *funcResult);

		using TestFunc = esl::runtime::FunctionCall<void, const esl::LuaTableRef&, int>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult, *tableRes, 999666);

		if (result)
			Msg("invoke success\n");
		else
			MsgError("Error while executing function TestTable.TempFunction: %s\n", result.errorMessage.ToCString());
	}

	// TEST: spline from TestTable must be retrieved as well as it's name (tests binding in Lua)
	if (!state.RunChunk("MsgInfo(\"Spline name\", TestTable.testSpline.name, \"\\n\")"))
	{
		MsgError("Error while running chunk: %s\n", esl::runtime::GetLastError(GetLuaState()));
	}

	// TEST: spline from TestTable must be retrieved as well as it's name (tests native getter)
	{
		auto tableRes = esl::runtime::GetGlobal<esl::LuaTable>(L, "TestTable");
		ASSERT(tableRes && tableRes.value.IsValid());

		Spline* spline = *tableRes.value.Get<Spline*>("testSpline");
		ASSERT(spline->name == "Some name");
	}
}

static bool ValueTest(const Spline* test)
{
	return test != nullptr;
}

DECLARE_CMD(test_globals, nullptr, 0)
{
	EqScriptState state(GetLuaState());

	Spline* spline = PPNew Spline();
	spline->name = "Spline of test_globals";
	state.SetGlobal("spl", spline);
	state.SetGlobal("TestCFunction", esl::binder::BindCFunction<ValueTest>());
}

//---------------------------------------------------
