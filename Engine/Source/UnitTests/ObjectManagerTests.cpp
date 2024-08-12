#include <Catch2/catch_amalgamated.hpp>

#include "Engine.h"
#include "Levels/Level.h"
#include "GameObjects/GameObject.h"
#include "GameObjects/ObjectManager.h"

class TestObject1 : public Ball::GameObject
{
public:
	float x, y, z;
	std::string placeholder;

	REFLECT(TestObject1);
};

class TestObject2 : public Ball::GameObject
{
public:
	bool active;
	int a, b, c;
	std::string testString;
	REFLECT(TestObject2);
};

CATCH_TEST_CASE("ObjectManager")
{
	// Setup objects
	Ball::ObjectManager objectManager;

	const std::string OBJ1_TEST_STRING = "Hello world";
	const std::string OBJ2_TEST_STRING = "Goodbye world";
	const std::string OBJ3_TEST_STRING = "This is a test";

	// Create objects

	TestObject1* obj1 = objectManager.AddObject<TestObject1>("");
	obj1->placeholder = OBJ1_TEST_STRING;
	TestObject2* obj2 = objectManager.AddObject<TestObject2>("");
	obj2->testString = OBJ2_TEST_STRING;
	TestObject1* obj3 = nullptr;

	CATCH_SECTION("Adding Objects")
	{
		TestObject1* obj3 = objectManager.AddObject<TestObject1>("");
		obj3->placeholder = OBJ3_TEST_STRING;

		[[maybe_unused]] auto shituck = objectManager.Size();
		CATCH_REQUIRE(obj3 != nullptr);
		CATCH_REQUIRE(objectManager.Size() == 3);
	}

	CATCH_SECTION("Iterator test 1")
	{
		int count = 0;
		TestObject1* obj3 = objectManager.AddObject<TestObject1>("");
		obj3->placeholder = OBJ3_TEST_STRING;

		for (auto it : objectManager)
		{
			if (count == 0)
			{
				TestObject1* objCast = static_cast<TestObject1*>(it);
				CATCH_REQUIRE(objCast != nullptr);
				CATCH_REQUIRE(objCast->placeholder == OBJ1_TEST_STRING);
				count++;
			}
			else if (count == 1)
			{
				TestObject2* objCast = static_cast<TestObject2*>(it);
				CATCH_REQUIRE(objCast != nullptr);
				CATCH_REQUIRE(objCast->testString == OBJ2_TEST_STRING);
				count++;
			}
			else if (count == 2)
			{
				TestObject1* objCast = static_cast<TestObject1*>(it);
				CATCH_REQUIRE(objCast != nullptr);
				CATCH_REQUIRE(objCast->placeholder == OBJ3_TEST_STRING);
				count++;
			}
		}

		CATCH_REQUIRE(count == 3);
	}

	CATCH_SECTION("Iterator test 2")
	{
		int count = 0;
		TestObject1* obj3 = objectManager.AddObject<TestObject1>("");
		obj3->placeholder = OBJ3_TEST_STRING;

		for (auto it = objectManager.begin(), end = objectManager.end(); it != end; ++it)
		{
			if (count == 0)
			{
				TestObject1* objCast = static_cast<TestObject1*>(it->get());
				CATCH_REQUIRE(objCast != nullptr);
				CATCH_REQUIRE(objCast->placeholder == OBJ1_TEST_STRING);
				count++;
			}
			else if (count == 1)
			{
				TestObject2* objCast = static_cast<TestObject2*>(it->get());
				CATCH_REQUIRE(objCast != nullptr);
				CATCH_REQUIRE(objCast->testString == OBJ2_TEST_STRING);
				count++;
			}
			else if (count == 2)
			{
				TestObject1* objCast = static_cast<TestObject1*>(it->get());
				CATCH_REQUIRE(objCast != nullptr);
				CATCH_REQUIRE(objCast->placeholder == OBJ3_TEST_STRING);
				count++;
			}
		}

		CATCH_REQUIRE(count == 3);
	}

	CATCH_SECTION("Removing Objects")
	{
		objectManager.RemoveObject(obj3);

		CATCH_REQUIRE(obj3 == nullptr);
		CATCH_REQUIRE(objectManager.Size() == 2);
	}

	CATCH_SECTION("Clear ObjectManager")
	{
		CATCH_REQUIRE(objectManager.Size() > 0);

		objectManager.Clear();

		CATCH_REQUIRE(objectManager.Size() == 0);
	}

	CATCH_SECTION("Pointer Stability")
	{
		for (int i = 0; i < 12; i++)
		{
			objectManager.AddObject<TestObject1>("");
		}

		TestObject1* obj1After = static_cast<TestObject1*>(objectManager[0]);
		TestObject2* obj2After = static_cast<TestObject2*>(objectManager[1]);

		CATCH_REQUIRE(obj1 == obj1After);
		CATCH_REQUIRE(obj2 == obj2After);

		CATCH_REQUIRE(reinterpret_cast<std::uintptr_t>(obj1) == reinterpret_cast<std::uintptr_t>(obj1After));
		CATCH_REQUIRE(reinterpret_cast<std::uintptr_t>(obj2) == reinterpret_cast<std::uintptr_t>(obj2After));

		objectManager.Clear();
	};
}