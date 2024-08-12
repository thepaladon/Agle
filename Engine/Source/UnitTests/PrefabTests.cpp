#include <Catch2/catch_amalgamated.hpp>

#include "FileIO.h"
#include "GameObjects/GameObject.h"
#include "GameObjects/Serialization/ObjectSerializer.h"

class PrefabTestObject : public Ball::GameObject
{
public:
	std::string m_TextField = "Hello World";
	int m_IndexValue = 444;

	void Serialize(Ball::SerializeArchive& archive) override
	{
		archive.Add(ARCHIVE_VAR(m_TextField));
		archive.Add(ARCHIVE_VAR(m_IndexValue));
	}
	REFLECT(PrefabTestObject)
};

CATCH_TEST_CASE("Prefabs")
{
	CATCH_SECTION("Create Object")
	{
		Ball::GameObject* testObject = Ball::ObjectFactory::CreateObject(PrefabTestObject::TYPE_NAME);
		CATCH_REQUIRE(testObject);
		PrefabTestObject* castedObject = dynamic_cast<PrefabTestObject*>(testObject);
		CATCH_REQUIRE(castedObject);

		CATCH_REQUIRE(castedObject->m_IndexValue == 444);
		CATCH_REQUIRE(castedObject->m_TextField == "Hello World");

		delete testObject;
	}

	// TODO add support for creating Prefabs, for more robust testing...

	CATCH_SECTION("Load Prefab")
	{
		auto* object = dynamic_cast<PrefabTestObject*>(Ball::ObjectSerializer::LoadPrefab("PrefabUnitTest"));

		CATCH_REQUIRE(object);
		CATCH_REQUIRE(object->GetPrefabName() == "PrefabUnitTest");
		CATCH_REQUIRE(object->m_TextField == "Some custom value");
		CATCH_REQUIRE(object->m_IndexValue == -999);

		// As this object is not added to a level, we have to delete it !
		delete object;
	}
}