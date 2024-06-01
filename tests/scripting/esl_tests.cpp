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

enum ETestUntyped
{
	TEST_ENUM_1,
	TEST_ENUM_2,
	TEST_ENUM_3
};

enum ETestTyped : int
{
	TEST_TYPED_ENUM_1,
	TEST_TYPED_ENUM_2,
	TEST_TYPED_ENUM_3
};

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

	int NonVirtualFunc(esl::ScriptState state, int valueA, int valueB, int valueC)
	{
		nonVirtualCalled = true;
		return valueB;
	}

	EqString		name;
	Vector3D		position;
	ETestTyped		angle;
	ETestUntyped	length;
	int				vehicleZoneNameHash{ 0 };
	float			speedLimit{ 50.0f };
	int16			numLanes{ 0 };
	int16			laneDirs{ 0 };
	int16			laneDisabled{ 0 };
	int16			flags{ 0 };

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
	esl::ScriptState state(m_instance);
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
	GTEST_FATAL_FAILURE_(expression) << esl::runtime::GetLastError(state)

#define LUA_GTEST_CHUNK_FAIL(expression) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                                 \
  if (const ::testing::AssertionResult gtest_ar_ =              \
	::testing::AssertionResult(!state.RunChunk(expression)));    \
  else                                                          \
	GTEST_NONFATAL_FAILURE_(expression) << esl::runtime::GetLastError(state)

//-------------------------------------------------------

template<typename T>
static void PrintTypeInfo()
{
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

	esl::TypeInfo typeInfo = ScriptClass<T>::GetTypeInfo();

	Msg("type %s%s%s\n", typeInfo.className, typeInfo.baseClassName ? " : " : "", typeInfo.baseClassName ? typeInfo.baseClassName : "");
	for (const esl::Member& mem : typeInfo.members)
	{
		if (mem.type == esl::MEMB_CTOR)
			Msg("   %s(%s)\n", mem.name, mem.signature);
		else if (mem.type == esl::MEMB_VAR)
			Msg("   %s %s : %s\n", typesStr[mem.type], mem.name, mem.signature);
		else if (mem.type == esl::MEMB_FUNC)
			Msg("   %s %s(%s) %s\n", typesStr[mem.type], mem.name, mem.signature, mem.isConst ? "const" : "");
		else
			Msg("   %s\n", mem.name);
	}
	Msg("end\n");
}

TEST(EQSCRIPT_TESTS, TestBinder)
{
	//PrintTypeInfo<Spline>();
	//PrintTypeInfo<TerrainSpline>();

	// TEST: validate binder
	{
		esl::TypeInfo typeInfo = esl::ScriptClass<Spline>::GetTypeInfo();

		EXPECT_EQ(typeInfo.members[0].type, esl::MEMB_DTOR) << "Destructor must be included and be first";

		// check constructors
		EXPECT_EQ(typeInfo.members[1].type, esl::MEMB_CTOR);
		EXPECT_EQ(typeInfo.members[2].type, esl::MEMB_CTOR);
		EXPECT_EQ(EqStringRef(typeInfo.members[2].signature), EqStringRef("number"));

		// check members
		EXPECT_EQ(typeInfo.members[3].type, esl::MEMB_FUNC);
		EXPECT_EQ(EqStringRef(typeInfo.members[3].signature), EqStringRef("KVSection"));

		// TEST: validate LuaTypeAlias
		EXPECT_EQ(typeInfo.members[6].type, esl::MEMB_VAR);
		EXPECT_EQ(EqStringRef(typeInfo.members[6].signature), EqStringRef("string"));

		// TEST: validate LuaTypeAlias
		EXPECT_EQ(typeInfo.members[7].type, esl::MEMB_VAR);
		EXPECT_EQ(EqStringRef(typeInfo.members[7].signature), EqStringRef("Vector3D"));

		// TEST: validate LuaTypeAlias enum type
		EXPECT_EQ(typeInfo.members[8].type, esl::MEMB_VAR);
		EXPECT_EQ(EqStringRef(typeInfo.members[8].signature), EqStringRef("number"));

		// TEST: validate LuaTypeAlias enum type
		EXPECT_EQ(typeInfo.members[9].type, esl::MEMB_VAR);
		EXPECT_EQ(EqStringRef(typeInfo.members[9].signature), EqStringRef("number"));
	}

	// TEST: validate binder with base class
	{
		esl::TypeInfo typeInfo = esl::ScriptClass<TerrainSpline>::GetTypeInfo();

		// TEST: base class matching
		EXPECT_EQ(typeInfo.base->className, typeInfo.baseClassName);

		// TEST: derived types must have their destructor too
		EXPECT_EQ(typeInfo.members[0].type, esl::MEMB_DTOR) << "Destructor must be included and be first";

		// check constructors
		EXPECT_EQ(typeInfo.members[1].type, esl::MEMB_CTOR);
		EXPECT_EQ(typeInfo.members[2].type, esl::MEMB_CTOR);
		EXPECT_EQ(EqStringRef(typeInfo.members[2].signature), EqStringRef("number,string,Spline"));
	}
}

TEST(EQSCRIPT_TESTS, TestInstantiation)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	// TEST: default constructor
	LUA_GTEST_CHUNK("spl = Spline.new()");
	{
		Spline& testResults = *state.GetGlobal<Spline&>("spl");
		EXPECT_FALSE(testResults.nonDefaultConstructor);
	}

	// TEST: additional constructor
	LUA_GTEST_CHUNK("spl = Spline.new(9999)");
	{
		Spline& testResults = *state.GetGlobal<Spline&>("spl");
		EXPECT_TRUE(testResults.nonDefaultConstructor);
	}

	// TEST: other object and upcasting brought to native code
	LUA_GTEST_CHUNK("spl = TerrainSpline.new()");
}

TEST(EQSCRIPT_TESTS, TestVariables)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

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
		EXPECT_EQ(testResults->vehicleZoneNameHash, 333);
	}

	// TEST: constructor call with upcasting argument: TerrainSpline to Spline
	LUA_GTEST_CHUNK("spl = TerrainSpline.new(1337, checkValueString2, spl)");
	{
		TerrainSpline& testResults = *state.GetGlobal<TerrainSpline&>("spl");
		EXPECT_FALSE(testResults.nonDefaultConstructor);
	}

	// TEST: constructor passing arguments to object
	LUA_GTEST_CHUNK("EXPECT_EQ(spl.name, checkValueString2)");
	LUA_GTEST_CHUNK("EXPECT_EQ(spl.vehicleZoneNameHash, 1337)");

	// TEST: property newindex (setter) and index (getter)
	LUA_GTEST_CHUNK("spl.vehicleZoneNameHash = checkValueNumber");
	LUA_GTEST_CHUNK("EXPECT_EQ(spl.vehicleZoneNameHash, checkValueNumber)");
}

TEST(EQSCRIPT_TESTS, TestAddingToMetatable)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	LUA_GTEST_CHUNK("spl = TerrainSpline.new()");

	// TEST: adding methods to metatable
	LUA_GTEST_CHUNK("TerrainSpline.Try = function(self, v) return v end; EXPECT_EQ(120, spl:Try(120))");
}

TEST(EQSCRIPT_TESTS, TestFunctionCalls)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

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

	// TEST: check arguments with function that has Lua State as first argument
	LUA_GTEST_CHUNK("EXPECT_EQ(spl:NonVirtualFunc(1555, 625, 525), 625)");
}

TEST(EQSCRIPT_TESTS, TestInheritClassFunctionCalls)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	// TEST: default constructor
	LUA_GTEST_CHUNK("spl = Spline.new()");

	// TEST: calling member function - should fail due to 'self' is nil, must use ':' operator in order to call member function
	LUA_GTEST_CHUNK_FAIL("spl.Save(nil)");
	{
		Spline* testResults = *state.GetGlobal<Spline*>("spl");
		EXPECT_FALSE(testResults->saveBaseCalled);
	}

	// TEST: calling member function
	LUA_GTEST_CHUNK("spl:Load(nil)");
	{
		Spline* testResults = *state.GetGlobal<Spline*>("spl");
		EXPECT_TRUE(testResults->loadBaseCalled);
	}

	// TEST: other object and upcasting brought to native code
	LUA_GTEST_CHUNK("spl = TerrainSpline.new()");
	{
		Spline* testResults = *state.GetGlobal<Spline*>("spl");
		EXPECT_TRUE(testResults != nullptr) << "Upcasting TerrainSpline to Spline failure";
	}

	// TEST: calling member function with same name (that hides other function due to different args)
	LUA_GTEST_CHUNK("spl:Save(KVSection.new(), true)");
	{
		TerrainSpline* testResults = *state.GetGlobal<TerrainSpline*>("spl");
		EXPECT_FALSE(testResults->saveBaseCalled);
		EXPECT_FALSE(testResults->saveOverrideCalled);
		EXPECT_TRUE(testResults->saveOverloadCalled);
	}

	// TEST: calling hidden member function which is under other name in Lua
	LUA_GTEST_CHUNK("spl = TerrainSpline.new(); spl:SaveStandard(nil)");
	{
		TerrainSpline* testResults = *state.GetGlobal<TerrainSpline*>("spl");
		EXPECT_FALSE(testResults->saveBaseCalled);
		EXPECT_TRUE(testResults->saveOverrideCalled);
		EXPECT_FALSE(testResults->saveOverloadCalled);
	}

	// TEST: calling member function
	LUA_GTEST_CHUNK("spl:Load(nil)");
	{
		Spline* testResults = *state.GetGlobal<Spline*>("spl");
		EXPECT_TRUE(testResults != nullptr) << "Upcasting TerrainSpline to Spline failure";
		EXPECT_FALSE(testResults->loadBaseCalled);
	}
}

//---------------------------------------------

TEST(EQSCRIPT_TESTS, SetGlobal)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

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
		EXPECT_EQ(temp, spline);
		EXPECT_EQ(spline->name, EqStringRef("Some spline name"));
	}

	// TEST: calling member function on global
	LUA_GTEST_CHUNK("testSpline:Load(nil)");
	{
		EXPECT_TRUE(temp->loadBaseCalled);
	}

	delete temp;
}

TEST(EQSCRIPT_TESTS, TestGetGlobal)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

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
		EXPECT_EQ(result.value, int8(768127));
	}

	{
		auto result = state.GetGlobal<int16>("TestNumberValue");
		EXPECT_EQ(result.value, int16(768127));
	}

	{
		auto result = state.GetGlobal<int32>("TestNumberValue");
		EXPECT_EQ(result.value, int32(768127));
	}
}

TEST(EQSCRIPT_TESTS, TestFunctionInvocations)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

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
			EXPECT_EQ(result.value, 1500 / 3);
		else
			ASSERT_FAIL("Error while executing function TestFunc: %s\n", result.errorMessage.ToCString());
	}
	
	// TEST: function without return value (success must be false)
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc2");

		using TestFunc = esl::runtime::FunctionCall<void>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult);

		EXPECT_TRUE(result.success) << result.errorMessage;
	}

	// TEST: function calling nil (success must be false)
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc3");

		using TestFunc = esl::runtime::FunctionCall<void>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult);

		EXPECT_FALSE(result.success) << result.errorMessage;
	}

	// TEST: function calling with error
	{
		auto funcResult = state.GetGlobal<esl::LuaFunctionRef>("TestFunc4");

		using TestFunc = esl::runtime::FunctionCall<void>;
		TestFunc::Result result = TestFunc::Invoke(*funcResult);

		EXPECT_FALSE(result.success) << result.errorMessage;
	}
}

TEST(EQSCRIPT_TESTS, TestStrings)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

	state.RunChunk("TestString = \"string AAB value\"");
	state.SetGlobal("TestString2", "The Awesome String Value");

	EqStringRef testString = *state.GetGlobal<EqStringRef>("TestString");
	const char* testStringPtr = *state.GetGlobal<const char*>("TestString");

	EXPECT_EQ(testString.ToCString(), testStringPtr);
	EXPECT_EQ(testString, EqStringRef("string AAB value"));
	LUA_GTEST_CHUNK("EXPECT_EQ(TestString2, \"The Awesome String Value\")");
}

TEST(EQSCRIPT_TESTS, TestTables)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

	state.RegisterClass<Spline>();
	state.RegisterClass<TerrainSpline>();

	state.RunChunk("TestTable = { keyA = \"string AAB value\", num = 1657 }");

	// TEST: get table and check if it's valid
	{
		esl::LuaTable tableRes = *state.GetGlobal<esl::LuaTable>("TestTable");
		EXPECT_TRUE(tableRes.IsValid());
	}

	// TEST: get a value from table
	{
		esl::LuaTable tableRes = *state.GetGlobal<esl::LuaTable>("TestTable");
		EXPECT_TRUE(tableRes.IsValid());

		EqStringRef str = *tableRes.Get<EqStringRef>("keyA");
		const int val = *tableRes.Get<int>("num");
		EXPECT_EQ(str, EqStringRef("string AAB value"));
		EXPECT_EQ(val, 1657);
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
		EqStringRef str = *tableRes.value.Get<EqStringRef>("stringValue");
		EqStringRef str2 = *tableRes.value.Get<EqStringRef>(temp);

		EXPECT_EQ(val, 1337);
		EXPECT_EQ(str, EqStringRef("This is a string set from C++"));

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

		EXPECT_TRUE(result.success) << result.errorMessage;
	}

	// TEST: spline from TestTable must be retrieved as well as it's name (tests native getter)
	{
		auto tableRes = esl::runtime::GetGlobal<esl::LuaTable>(stateTest, "TestTable");
		ASSERT(tableRes && tableRes.value.IsValid());

		Spline* spline = *tableRes.value.Get<Spline*>("testSpline");
		EXPECT_EQ(spline->name, EqStringRef("Some name"));

		delete spline;
	}
}

static bool ValueTest(const Spline* test)
{
	return test != nullptr;
}

int ValueTestWithScriptState(esl::ScriptState state, int valueA, int valueB, int valueC)
{
	return valueB;
}

TEST(EQSCRIPT_TESTS, TestCFunction)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

	state.RegisterClass<Spline>();

	Spline* spline = PPNew Spline();
	spline->name = "Spline of test_globals";
	state.SetGlobal("spl", spline);
	state.SetGlobal("TestCFunction", EQSCRIPT_CFUNC(ValueTest));
	state.SetGlobal("TestCFunction2", EQSCRIPT_CFUNC(ValueTestWithScriptState));
	
	// TEST: C function call with arguments (nil)
	LUA_GTEST_CHUNK("EXPECT_EQ(TestCFunction(testValue), false)");

	// TEST: C function call with arguments
	LUA_GTEST_CHUNK("EXPECT_EQ(TestCFunction(spl), true)");

	// TEST: C function call which has Lua State as first argument
	LUA_GTEST_CHUNK("EXPECT_EQ(TestCFunction2(555, 968, 782), 968)");

	delete spline;
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

TEST(EQSCRIPT_TESTS, RefPtrDereference)
{
	int controlValue = 0xDEADBEEF;

	{
		LuaStateTest stateTest;
		esl::ScriptState state(stateTest);
		state.RegisterClass<TestRefPtr>();

		// TEST: refptr owned by lua, dereferenced by C++
		{
			CRefPtr<TestRefPtr> testPush = CRefPtr_new(TestRefPtr, controlValue);
			state.SetGlobal("testRefPtr", testPush);

			EXPECT_EQ(controlValue, 0xDEADBEEF);
		}

		// TEST: refptr still owns pointer
		EXPECT_NE(controlValue, 0xFEDABEEF);
	}

	// TEST: refptr deferenced by Lua GC destructor call
	EXPECT_EQ(controlValue, 0xFEDABEEF);
}

static void RefPtrTestNotNull(const CRefPtr<TestRefPtr>& testPtr)
{
	EXPECT_TRUE(testPtr != nullptr);

	// Lua owns this as well as binder
	EXPECT_EQ(testPtr->Ref_Count(), 2);
}

TEST(EQSCRIPT_TESTS, RefPtrFromLua)
{
	CRefPtr<TestRefPtr> testGet;

	{
		LuaStateTest stateTest;
		esl::ScriptState state(stateTest);
		state.RegisterClass<TestRefPtr>();

		state.SetGlobal("TestCFunction", EQSCRIPT_CFUNC(RefPtrTestNotNull));

		// TEST: refptr created inside lua
		LUA_GTEST_CHUNK("testValue = TestRefPtr.new(); testValue.value = 555; testValue.strValue = 'Hi from Lua'");

		// TEST: refptr is passed as argument to function
		LUA_GTEST_CHUNK("TestCFunction(testValue)");

		testGet = *state.GetGlobal<CRefPtr<TestRefPtr>>("testValue");
		EXPECT_EQ(testGet->Ref_Count(), 2);
	}
	
	// TEST: only C++ must own this object
	EXPECT_EQ(testGet->Ref_Count(), 1);

	EXPECT_EQ(testGet->value, 555);
	EXPECT_EQ(testGet->strValue, EqStringRef("Hi from Lua"));
}

struct TestEvent
{
	ESL_DECLARE_EVENT(Event);
};

EQSCRIPT_BIND_TYPE_NO_PARENT(TestEvent, "TestEvent", BY_REF)
EQSCRIPT_TYPE_BEGIN(TestEvent)
	EQSCRIPT_BIND_EVENT(Event)
EQSCRIPT_TYPE_END

TEST(EQSCRIPT_TESTS, TestNativeEvent)
{
	TestEvent* evtTestObj = PPNew TestEvent();

	{
		LuaStateTest stateTest;
		esl::ScriptState state(stateTest);

		state.RegisterClass<TestEvent>();
		state.SetGlobal("evtTest", evtTestObj);

		// TEST: lambda function binder
		state.SetGlobal("TestCFunction", EQSCRIPT_CFUNC(+[](const EqStringRef& message) {
			EXPECT_EQ(EqStringRef("Event Called"), message);
		}));

		// TEST: event handlers added
		LUA_GTEST_CHUNK("testHandle = evtTest.Event:AddHandler(function() end)");
		EXPECT_EQ(evtTestObj->m_eslEventEvent.GetHandlerCount(), 1);

		// TEST: call event from native code
		ESL_DECLARE_EVENT_CALL(evtTestObj, Event, const EqString&);
		ESL_CALL_EVENT(Event, "Event Called");

		LUA_GTEST_CHUNK("evtTest.Event:RemoveHandler(testHandle)");

		// TEST: handler removed
		EXPECT_EQ(evtTestObj->m_eslEventEvent.GetHandlerCount(), 0);

		// TEST: re-add event
		LUA_GTEST_CHUNK("testHandle = evtTest.Event:AddHandler(TestCFunction)");
		EXPECT_EQ(evtTestObj->m_eslEventEvent.GetHandlerCount(), 1);
	}

	// TEST: handler remove due to GC of Handle and Lua state closed
	EXPECT_EQ(evtTestObj->m_eslEventEvent.GetHandlerCount(), 0);

	delete evtTestObj;
}

struct OperatorTests
{
	int value;
};

static void ToStringTest(const OperatorTests& v, char* dest, const int destSize)
{
	snprintf(dest, destSize, "current value: %d", v.value);
}

EQSCRIPT_BIND_TYPE_NO_PARENT(OperatorTests, "TestOperators", BY_VALUE)
EQSCRIPT_TYPE_BEGIN(OperatorTests)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_OP_TOSTRING(ToStringTest)

	// currently no point to test every operator because their 
	// implementation is completely user-defined, ESL does not handle anything.

	EQSCRIPT_BIND_VAR(value)
EQSCRIPT_TYPE_END

TEST(EQSCRIPT_TESTS, TestOperatorBinding)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

	state.RegisterClass<OperatorTests>();

	LUA_GTEST_CHUNK("test = TestOperators.new(); test.value = 545999");

	// TEST: tostring operator
	LUA_GTEST_CHUNK("EXPECT_EQ(tostring(test), 'current value: 545999')");
}

struct ByValueTests
{
	ByValueTests() = default;
	ByValueTests(ByValueTests&& other) noexcept
		: value(std::move(other.value))
		, usedCopyConstructor(other.usedCopyConstructor)
		, isMoved(true)
	{
	}
	ByValueTests(const ByValueTests& other) 
		: value(other.value)
		, usedCopyConstructor(true)
	{
	}
	int		value{ -1 };
	bool	usedCopyConstructor { false };
	bool	isMoved{ false };
};

EQSCRIPT_BIND_TYPE_NO_PARENT(ByValueTests, "ByValueTests", BY_VALUE)
EQSCRIPT_TYPE_BEGIN(ByValueTests)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(const ByValueTests&)
	EQSCRIPT_BIND_VAR(value)
	EQSCRIPT_BIND_VAR(usedCopyConstructor)
	EQSCRIPT_BIND_VAR(isMoved)
EQSCRIPT_TYPE_END

ByValueTests& ByValueBypassFunc(esl::ScriptState scriptState, ByValueTests& ref)
{
	return ref;
}

esl::Object<ByValueTests> ByValueBypassFuncObj(esl::ScriptState scriptState, const esl::Object<ByValueTests>& ref)
{
	if (ref)
	{
		EXPECT_TRUE(ref);
		EXPECT_EQ(ref->value, 888666); // set in C++

		ref->value = 555;
	}

	EXPECT_EQ(esl::binder::ObjectIndexGetter::Get(ref), 1);
	return ref;
}

TEST(EQSCRIPT_TESTS, TestByValue)
{
	LuaStateTest stateTest;
	esl::ScriptState state(stateTest);

	state.RegisterClass<ByValueTests>();

	ByValueTests* testValue = PPNew ByValueTests();
	testValue->value = 888999;

	// copy to Lua
	state.SetGlobal("testByValue", testValue);

	// this is safe for BY_VALUE types
	delete testValue;

	// TEST: should copy object
	LUA_GTEST_CHUNK("EXPECT_EQ(testByValue.value, 888999)");
	LUA_GTEST_CHUNK("EXPECT_EQ(testByValue.usedCopyConstructor, true)");
	LUA_GTEST_CHUNK("EXPECT_EQ(testByValue.isMoved, false)");

	// TEST: should not use copy constructor
	LUA_GTEST_CHUNK("test2 = ByValueTests.new(); test2.value = 4444; EXPECT_EQ(test2.usedCopyConstructor, false); EXPECT_EQ(test2.isMoved, false)");

	// TEST: should use copy constructor and not be moved
	LUA_GTEST_CHUNK("test3 = ByValueTests.new(test2); EXPECT_EQ(test3.usedCopyConstructor, true); EXPECT_EQ(test3.isMoved, false)");

	// TEST: should give a reference to object in Lua
	{
		LUA_GTEST_CHUNK("test4 = ByValueTests.new(); test4.value = 4444");
		ByValueTests& test4 = *state.GetGlobal<ByValueTests&>("test4");

		EXPECT_EQ(test4.value, 4444);
		EXPECT_FALSE(test4.isMoved);
		EXPECT_FALSE(test4.usedCopyConstructor);

		// we can access object which is in Lua to modify
		test4.value = 888666;
		LUA_GTEST_CHUNK("EXPECT_EQ(test4.value, 888666)");
	}

	state.SetGlobal("TestRefArgument", EQSCRIPT_CFUNC(+[](const ByValueTests& ref, const int checkValue) {
		EXPECT_EQ(ref.value, checkValue);
		EXPECT_FALSE(ref.isMoved);
		EXPECT_FALSE(ref.usedCopyConstructor);
	}));

	state.SetGlobal("TestPtrArgument", EQSCRIPT_CFUNC(+[](ByValueTests* ptr, const int checkValue) {
		EXPECT_EQ(ptr->value, checkValue);
		EXPECT_FALSE(ptr->isMoved);
		EXPECT_FALSE(ptr->usedCopyConstructor);
	}));


	state.SetGlobal("ByValueBypassFunc", EQSCRIPT_CFUNC(ByValueBypassFunc));
	state.SetGlobal("ByValueBypassFuncObj", EQSCRIPT_CFUNC(ByValueBypassFuncObj));

	// TEST: value should not be copied or moved when passed as argument
	LUA_GTEST_CHUNK("TestRefArgument(test4, 888666)");
	LUA_GTEST_CHUNK("TestPtrArgument(test2, 4444)");

	// TEST: use copy constructor
	LUA_GTEST_CHUNK("testRet = ByValueBypassFunc(test4)");
	LUA_GTEST_CHUNK("EXPECT_EQ(testRet.isMoved, false)");
	LUA_GTEST_CHUNK("EXPECT_EQ(testRet.usedCopyConstructor, true)");

	// TEST: use bypass reference, must be the same object
	LUA_GTEST_CHUNK("testRet = ByValueBypassFuncObj(test4, 4)");
	LUA_GTEST_CHUNK("EXPECT_EQ(testRet, test4)");
	LUA_GTEST_CHUNK("EXPECT_EQ(test4.value, 555)");
	LUA_GTEST_CHUNK("EXPECT_EQ(testRet.value, 555)");
	LUA_GTEST_CHUNK("EXPECT_EQ(testRet.isMoved, false)");
	LUA_GTEST_CHUNK("EXPECT_EQ(testRet.usedCopyConstructor, false)");
}

struct BindTest
{
	int value;
};

//EQSCRIPT_BIND_TYPE_NO_PARENT(BindTest, "BindTest", BY_VALUE)
//EQSCRIPT_TYPE_BEGIN(BindTest)
//	EQSCRIPT_BIND_VAR(value)
//EQSCRIPT_TYPE_END

// _ESL_BIND_TYPE_BASICS
namespace esl {
	template<> inline const char* esl::LuaTypeAlias<BindTest, false>::value = "BindTest"; 
	template<> struct LuaTypeByVal<BindTest> : std::true_type {}; 
	template<> inline const char ScriptClass<BindTest>::className[] = "BindTest";
	template<> inline const char* ScriptClass<BindTest>::baseClassName = nullptr;
	template<> inline TypeInfo ScriptClass<BindTest>::baseClassTypeInfo = {};
}
// _ESL_TYPE_PUSHGET
namespace esl::runtime {
	template<> PushGet<BindTest>::PushFunc PushGet<BindTest>::Push = &PushGetImpl<BindTest>::PushObject; 
	template<> PushGet<BindTest>::GetFunc PushGet<BindTest>::Get = &PushGetImpl<BindTest>::GetObject;
}

// EQSCRIPT_TYPE_BEGIN / END
namespace esl::bindings 
{
template<> ArrayCRef<Member> ClassBinder<BindTest>::GetMembers() 
{
	BaseClassStorage::Add<BindClass>();
	static Member members[] = {
		MakeDestructor(),
		MakeVariable<(&BindClass::value)>("value"),
	};
	return members;
}
}