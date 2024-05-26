//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2021
//////////////////////////////////////////////////////////////////////////////////
// Description: Lua object
//////////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include "core/core_common.h"
#include "utils/KeyValues.h"

#include "scripting/esl.h"
#include "scripting/esl_luaref.h"
#include "scripting/esl_bind.h"

class LuaStateTest
{
public:
	LuaStateTest();
	~LuaStateTest();

	operator lua_State* () { return m_instance; }

private:

	lua_State* m_instance;
};

//---------------------------------------------------

struct Spline
{
	virtual ~Spline() = default;
	Spline() = default;
	Spline(int value)
	{
		nonDefaultConstructor = true;
	}

	virtual void Load(const KVSection* section)
	{
		loadBaseCalled = true;		
	}
	virtual void Save(KVSection* section)
	{
		saveBaseCalled = true;
	}

	void NonVirtualFunc()
	{
		nonVirtualCalled = true;
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

	bool loadBaseCalled = false;
	bool saveBaseCalled = false;
	bool nonVirtualCalled = false;
	bool nonDefaultConstructor = false;
};


// Inheritance test only
struct TerrainSpline : public Spline
{
	TerrainSpline() = default;
	TerrainSpline(int value, const char* message, Spline& other)
	{
		other.Save(nullptr);
		//MsgWarning("TerrainSpline::Ctor variant called, arg %d, %s, other name: %s\n", value, message, other.name.ToCString());
		name = message;
		vehicleZoneNameHash = value;
	}

	Vector2D GetPoint(int index, const Vector2D& from, const EqString& str)
	{ 
		ASSERT(str.Length() > 0);
		//MsgWarning("TerrainSpline::GetPoint called, arg %d, vehicleZoneNameHash = %d, from = %.2f, %.2f, %s\n", index, vehicleZoneNameHash, from.x, from.y, str.ToCString());
		return position.xz();// points[index];
	}

	void Load(const KVSection* section) override
	{
		loadOverrideCalled = true;
	}

	void Save(KVSection* section) override
	{
		saveOverrideCalled = true;
	}

	void Save(KVSection* section, bool useTest)
	{
		// ToCpp gives us this opportunity
		if(section)
		{
			delete section;
		}

		saveOverloadCalled = true;
	}

	Array<Vector3D> points{ PP_SL };

	bool loadOverrideCalled = false;
	bool saveOverrideCalled = false;
	bool saveOverloadCalled = false;
};

EQSCRIPT_BIND_TYPE_NO_PARENT(Vector2D, "Vector2D", BY_VALUE)
EQSCRIPT_TYPE_BEGIN(Vector2D)
	EQSCRIPT_BIND_CONSTRUCTOR(float)
	EQSCRIPT_BIND_CONSTRUCTOR(float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector2D&)

	//BIND_VECTOR_OPERATORS()

	EQSCRIPT_BIND_VAR(x)
	EQSCRIPT_BIND_VAR(y)
EQSCRIPT_TYPE_END

EQSCRIPT_BIND_TYPE_NO_PARENT(Vector3D, "Vector3D", BY_VALUE)
EQSCRIPT_TYPE_BEGIN(Vector3D)
	EQSCRIPT_BIND_CONSTRUCTOR(float)
	EQSCRIPT_BIND_CONSTRUCTOR(float, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector2D&, float)
	EQSCRIPT_BIND_CONSTRUCTOR(float, const Vector2D&)
	EQSCRIPT_BIND_CONSTRUCTOR(const Vector3D&)

	//BIND_VECTOR_OPERATORS()

	EQSCRIPT_BIND_FUNC(xy)
	EQSCRIPT_BIND_FUNC(yz)
	EQSCRIPT_BIND_FUNC(xz)

	EQSCRIPT_BIND_VAR(x)
	EQSCRIPT_BIND_VAR(y)
	EQSCRIPT_BIND_VAR(z)
EQSCRIPT_TYPE_END

EQSCRIPT_BIND_TYPE_NO_PARENT(KVSection, "KVSection", BY_REF)
EQSCRIPT_TYPE_BEGIN(KVSection)
	EQSCRIPT_BIND_CONSTRUCTOR()
EQSCRIPT_TYPE_END

EQSCRIPT_BIND_TYPE_NO_PARENT(Spline, "Spline", BY_REF)
EQSCRIPT_TYPE_BEGIN(Spline)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(int)
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
EQSCRIPT_TYPE_END

EQSCRIPT_BIND_TYPE_WITH_PARENT(TerrainSpline, Spline, "TerrainSpline")
EQSCRIPT_TYPE_BEGIN(TerrainSpline)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(int, const char*, Spline&)
	EQSCRIPT_BIND_FUNC(GetPoint)
	EQSCRIPT_BIND_FUNC_OVERLOAD(Save, void, (KVSection*, bool), ESL_APPLY_TRAITS(void, esl::ToCpp<KVSection*)>, bool)
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SaveStandard", Save, void, (KVSection*))
EQSCRIPT_TYPE_END

//-------------------------------------------------------

LuaStateTest::LuaStateTest()
{
	m_instance = luaL_newstate();
	luaL_openlibs(m_instance);

	// TODO: move this to esl::InitState()
	EqScriptState state(m_instance);
	state.RegisterClass<esl::LuaEvent>();
	state.RegisterClass<esl::LuaEvent::Handle>();

	state.RegisterClass<Vector2D>();
	state.RegisterClass<Vector3D>();
	state.RegisterClass<KVSection>();

	// setup stuff for tests
	state.RunChunk(R"(

function EXPECT_EQ(val, exp)
	if val ~= exp then
		error("expected: " .. tostring(exp) .. ", got " .. tostring(val))
	end
end

checkValueNumber = 555
checkValueString1 = "Hello from Universe"
checkValueString2 = "BaseClass check"

)");
}

LuaStateTest::~LuaStateTest()
{
	lua_gc(m_instance, LUA_GCCOLLECT, 0);
	lua_close(m_instance);
}

#define LUA_GTEST_CHUNK(expression) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                                 \
  if (const ::testing::AssertionResult gtest_ar_ =              \
	::testing::AssertionResult(state.RunChunk(expression)));    \
  else                                                          \
	GTEST_NONFATAL_FAILURE_(expression) << esl::runtime::GetLastError(state)

#define LUA_GTEST_CHUNK_FAIL(expression) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                                 \
  if (const ::testing::AssertionResult gtest_ar_ =              \
	::testing::AssertionResult(!state.RunChunk(expression)));    \
  else                                                          \
	GTEST_NONFATAL_FAILURE_(expression) << esl::runtime::GetLastError(state)

//-------------------------------------------------------

TEST(EqScriptTests, TestBinder)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	static const char* typesStr[] = {
		"null",
		"const",
		"dtor",
		"ctor",
		"func",
		"c_func",
		"var",
		"operator",
	};

	// test type info for Spline
	{
		esl::TypeInfo typeInfo = EqScriptClass<Spline>::GetTypeInfo();

		Msg("class %s%s%s\n", typeInfo.className, typeInfo.baseClassName ? " : " : "", typeInfo.baseClassName ? typeInfo.baseClassName : "");
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

	// test type info for TerrainSpline
	{
		esl::TypeInfo typeInfo = EqScriptClass<TerrainSpline>::GetTypeInfo();

		Msg("class %s%s%s\n", typeInfo.className, typeInfo.baseClassName ? " : " : "", typeInfo.baseClassName ? typeInfo.baseClassName : "");
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
}

TEST(EqScriptTests, TestInstantiation)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	// TEST: default constructor
	LUA_GTEST_CHUNK("spl = Spline.new()");
	{
		Spline& testResults = *state.GetGlobal<Spline&>("spl");
		ASSERT_FALSE(testResults.nonDefaultConstructor);
	}

	// TEST: additional constructor
	LUA_GTEST_CHUNK("spl = Spline.new(9999)");
	{
		Spline& testResults = *state.GetGlobal<Spline&>("spl");
		ASSERT_TRUE(testResults.nonDefaultConstructor);
	}

	// TEST: other object and upcasting brought to native code
	LUA_GTEST_CHUNK("spl = TerrainSpline.new()");
}

TEST(EqScriptTests, TestVariables)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	// TEST: default constructor
	LUA_GTEST_CHUNK("spl = Spline.new()");

	// TEST: other class with additional costructor
	LUA_GTEST_CHUNK("spl = TerrainSpline.new(333, checkValueString1, spl)");

	// TEST: constructor passing arguments to object
	LUA_GTEST_CHUNK("EXPECT_EQ(spl.name, checkValueString1)");
	LUA_GTEST_CHUNK("EXPECT_EQ(spl.vehicleZoneNameHash, 333)");
	{
		TerrainSpline* testResults = *state.GetGlobal<TerrainSpline*>("spl");
		ASSERT_EQ(testResults->vehicleZoneNameHash, 333);
	}

	// TEST: constructor call with upcasting argument: TerrainSpline to Spline
	LUA_GTEST_CHUNK("spl = TerrainSpline.new(1337, checkValueString2, spl)");
	{
		TerrainSpline& testResults = *state.GetGlobal<TerrainSpline&>("spl");
		ASSERT_FALSE(testResults.nonDefaultConstructor);
	}

	// TEST: constructor passing arguments to object
	LUA_GTEST_CHUNK("EXPECT_EQ(spl.name, checkValueString2)");
	LUA_GTEST_CHUNK("EXPECT_EQ(spl.vehicleZoneNameHash, 1337)");

	// TEST: property newindex (setter) and index (getter)
	LUA_GTEST_CHUNK("spl.vehicleZoneNameHash = checkValueNumber");
	LUA_GTEST_CHUNK("EXPECT_EQ(spl.vehicleZoneNameHash, checkValueNumber)");
}

TEST(EqScriptTests, TestAddingToMetatable)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	LUA_GTEST_CHUNK("spl = TerrainSpline.new()");

	// TEST: adding methods to metatable
	LUA_GTEST_CHUNK("TerrainSpline.Try = function(self, v) return v end; EXPECT_EQ(120, spl:Try(120))");
}

TEST(EqScriptTests, TestFunctionCalls)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	// TEST: default constructor
	LUA_GTEST_CHUNK("spl = TerrainSpline.new()");

	// TEST: function call with arguments
	LUA_GTEST_CHUNK("spl:GetPoint(1337, Vector2D.new(0, 68), \"Hello from Lua\")");

	// TEST: argument matching - should fail on nil
	LUA_GTEST_CHUNK_FAIL("spl:GetPoint(666, nil, \"From Russia with NIL\")");

	// TEST: argument matching - should fail on non-userdata
	LUA_GTEST_CHUNK_FAIL("spl:GetPoint(1900 / 2, 9999, \"Whoops\")");

	// TEST: argument matching - should fail on incorrect userdata
	LUA_GTEST_CHUNK_FAIL("spl:GetPoint(1900 / 2, spl, \"Whoops\")");
}

TEST(EqScriptTests, TestInheritClassFunctionCalls)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	// TEST: default constructor
	LUA_GTEST_CHUNK("spl = Spline.new()");

	// TEST: calling member function - should fail due to 'self' is nil, must use ':' operator in order to call member function
	LUA_GTEST_CHUNK_FAIL("spl.Save(nil)");
	{
		Spline* testResults = *state.GetGlobal<Spline*>("spl");
		ASSERT_FALSE(testResults->saveBaseCalled);
	}

	// TEST: calling member function
	LUA_GTEST_CHUNK("spl:Load(nil)");
	{
		Spline* testResults = *state.GetGlobal<Spline*>("spl");
		ASSERT_TRUE(testResults->loadBaseCalled);
	}

	// TEST: other object and upcasting brought to native code
	LUA_GTEST_CHUNK("spl = TerrainSpline.new()");
	{
		Spline* testResults = *state.GetGlobal<Spline*>("spl");
		ASSERT_TRUE(testResults != nullptr) << "Upcasting TerrainSpline to Spline failure";
	}

	// TEST: calling member function with same name (that hides other function due to different args)
	LUA_GTEST_CHUNK("spl:Save(KVSection.new(), true)");
	{
		TerrainSpline* testResults = *state.GetGlobal<TerrainSpline*>("spl");
		ASSERT_FALSE(testResults->saveBaseCalled);
		ASSERT_FALSE(testResults->saveOverrideCalled);
		ASSERT_TRUE(testResults->saveOverloadCalled);
	}

	// TEST: calling hidden member function which is under other name in Lua
	LUA_GTEST_CHUNK("spl = TerrainSpline.new(); spl:SaveStandard(nil)");
	{
		TerrainSpline* testResults = *state.GetGlobal<TerrainSpline*>("spl");
		ASSERT_FALSE(testResults->saveBaseCalled);
		ASSERT_TRUE(testResults->saveOverrideCalled);
		ASSERT_FALSE(testResults->saveOverloadCalled);
	}

	// TEST: calling member function
	LUA_GTEST_CHUNK("spl:Load(nil)");
	{
		Spline* testResults = *state.GetGlobal<Spline*>("spl");
		ASSERT_TRUE(testResults != nullptr) << "Upcasting TerrainSpline to Spline failure";
		ASSERT_FALSE(testResults->loadBaseCalled);
	}
}

//---------------------------------------------

TEST(EqScriptTests, SetGlobal)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	state.RegisterClass<Spline>();

	Spline* temp = PPNew Spline();
	{
		temp->name = "Some spline name";
		state.SetGlobal("testSpline", temp);
	}

	// TEST: spline from TestTable must be retrieved as well as it's name (tests binding in Lua)
	LUA_GTEST_CHUNK("EXPECT_EQ(\"Some spline name\", testSpline.name)");

	// TEST: retrieve from globals, check pointers, check name (paranoid about memory damage lol)
	{
		Spline* spline = *state.GetGlobal<Spline*>("testSpline");
		ASSERT_EQ(temp, spline);
		ASSERT_EQ(spline->name, "Some spline name");
	}

	// TEST: calling member function on global
	LUA_GTEST_CHUNK("testSpline:Load(nil)");
	{
		ASSERT_TRUE(temp->loadBaseCalled);
	}

	delete temp;
}

TEST(EqScriptTests, TestGetGlobal)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	state.RunChunk("TestFunc = function(intVal, strVal) print(intVal, strVal, \"\\n\"); return 1500.0 / 3.0 end");
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

TEST(EqScriptTests, TestFunctionInvocations)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	state.RunChunk("TestFunc = function(intVal, strVal) print(intVal, strVal, \"\\n\"); return 1500 / 3 end");
	state.RunChunk("TestFunc2 = function() print(\"Simple function call\\n\") end");
	state.RunChunk("TestFunc4 = function() r = a + b end");

	// TEST: function with return value and arguments
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc");

		using TestFunc = esl::runtime::FunctionCall<float, int, const char*>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult, 1337, "Hello from C++ to Lua");
		if (result)
			ASSERT_EQ(result.value, 1500 / 3);
		else
			ASSERT_FAIL("Error while executing function TestFunc: %s\n", result.errorMessage.ToCString());
	}
	
	// TEST: function without return value (success must be false)
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc2");

		using TestFunc = esl::runtime::FunctionCall<void>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult);

		ASSERT_TRUE(result.success) << result.errorMessage;
	}

	// TEST: function calling nil (success must be false)
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc3");

		using TestFunc = esl::runtime::FunctionCall<void>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult);

		ASSERT_FALSE(result.success) << result.errorMessage;
	}

	// TEST: function calling with error
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc4");

		using TestFunc = esl::runtime::FunctionCall<void>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult);

		ASSERT_FALSE(result.success) << result.errorMessage;
	}
}

TEST(EqScriptTests, TestTables)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	state.RunChunk("TestTable = { keyA = \"string AAB value\", num = 1657 }");

	// TEST: get table and check if it's valid
	{
		esl::LuaTable tableRes = *state.GetGlobal<esl::LuaTable>("TestTable");
		ASSERT_TRUE(tableRes.IsValid());
	}

	// TEST: get a value from table
	{
		esl::LuaTable tableRes = *state.GetGlobal<esl::LuaTable>("TestTable");
		ASSERT_TRUE(tableRes.IsValid());

		const EqString str = *tableRes.Get<EqString>("keyA");
		const int val = *tableRes.Get<int>("num");
		ASSERT_EQ(str, "string AAB value");
		ASSERT_EQ(val, 1657);
		//Msg("Table has values %s, %d\n", str.ToCString(), val);
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

		ASSERT_EQ(val, 1337);
		ASSERT_EQ(str, "This is a string set from C++");

		// This will fail because the value is boxed and we always pushing new userdata.
		// We need somehow to make comparison mechanism to do this between native and Lua.
		// Doing this all in Lua is pretty fine.
		// Also CRefPtr would work fine at this point
		//ASSERT(str2 != "Spline as a key");
		ASSERT(str2.Length() == 0);

		//Msg("Table has values %s, %d\n", str.ToCString(), val);
	}

	// TEST: set a function in table and call it with self as a table
	{
		state.RunChunk("TempFunction = function(self, arg2) print(\"Simple function call with self -\", self.valueNum, arg2,\"\\n\") end");

		auto tableRes = state.GetGlobal<esl::LuaTable>("TestTable");
		ASSERT(tableRes && tableRes.value.IsValid());

		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TempFunction");

		tableRes.value.Set("func", *funcResult);

		using TestFunc = esl::runtime::FunctionCall<void, const esl::LuaTableRef&, int>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult, *tableRes, 999666);

		ASSERT_TRUE(result.success) << result.errorMessage;
	}

	// TEST: spline from TestTable must be retrieved as well as it's name (tests native getter)
	{
		auto tableRes = esl::runtime::GetGlobal<esl::LuaTable>(stateTest, "TestTable");
		ASSERT(tableRes && tableRes.value.IsValid());

		Spline* spline = *tableRes.value.Get<Spline*>("testSpline");
		ASSERT_EQ(spline->name, "Some name");
	}
}

static bool ValueTest(const Spline* test)
{
	return test != nullptr;
}

TEST(EqScriptTests, TestCFunction)
{
	LuaStateTest stateTest;
	EqScriptState state(stateTest);

	state.RegisterClass<Spline>();

	Spline* spline = PPNew Spline();
	spline->name = "Spline of test_globals";
	state.SetGlobal("spl", spline);
	state.SetGlobal("TestCFunction", EQSCRIPT_CFUNC(ValueTest));

	// TEST: C function call with arguments (nil)
	LUA_GTEST_CHUNK("EXPECT_EQ(TestCFunction(testValue), false)");

	// TEST: C function call with arguments
	LUA_GTEST_CHUNK("EXPECT_EQ(TestCFunction(spl), true)");
}

struct TestRefPtr : public RefCountedObject<TestRefPtr>
{
	TestRefPtr() = default;
	TestRefPtr(int& controlValue)
		: controlValue(controlValue)
	{
	}

	~TestRefPtr()
	{
		controlValue = 0xFEDABEEF;
	}

	int& controlValue = value;
	int					value{ 0 };
	EqString			strValue;
};

EQSCRIPT_BIND_TYPE_NO_PARENT(TestRefPtr, "TestRefPtr", REF_PTR)
EQSCRIPT_TYPE_BEGIN(TestRefPtr)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_VAR(value)
	EQSCRIPT_BIND_VAR(strValue)
EQSCRIPT_TYPE_END

TEST(EqScriptTests, RefPtrDereference)
{
	int controlValue = 0xDEADBEEF;

	{
		LuaStateTest stateTest;
		EqScriptState state(stateTest);
		state.RegisterClass<TestRefPtr>();

		// TEST: refptr owned by lua, dereferenced by C++
		{
			CRefPtr<TestRefPtr> testPush = CRefPtr_new(TestRefPtr, controlValue);
			state.SetGlobal("testRefPtr", testPush);

			ASSERT_EQ(controlValue, 0xDEADBEEF);
		}

		// TEST: refptr still owns pointer
		ASSERT_NE(controlValue, 0xFEDABEEF);
	}

	// TEST: refptr deferenced by Lua GC destructor call
	ASSERT_EQ(controlValue, 0xFEDABEEF);
}

static void RefPtrTestNotNull(const CRefPtr<TestRefPtr>& testPtr)
{
	ASSERT_TRUE(testPtr != nullptr);

	// Lua owns this as well as binder
	ASSERT_EQ(testPtr->Ref_Count(), 2);
}

TEST(EqScriptTests, RefPtrFromLua)
{
	CRefPtr<TestRefPtr> testGet;

	{
		LuaStateTest stateTest;
		EqScriptState state(stateTest);
		state.RegisterClass<TestRefPtr>();

		state.SetGlobal("TestCFunction", EQSCRIPT_CFUNC(RefPtrTestNotNull));

		// TEST: refptr created inside lua
		LUA_GTEST_CHUNK("testValue = TestRefPtr.new(); testValue.value = 555; testValue.strValue = 'Hi from Lua'");

		// TEST: refptr is passed as argument to function
		LUA_GTEST_CHUNK("TestCFunction(testValue)");

		testGet = *state.GetGlobal<CRefPtr<TestRefPtr>>("testValue");
		ASSERT_EQ(testGet->Ref_Count(), 2);
	}
	
	// TEST: only C++ must own this object
	ASSERT_EQ(testGet->Ref_Count(), 1);

	ASSERT_EQ(testGet->value, 555);
	ASSERT_EQ(testGet->strValue, "Hi from Lua");
}