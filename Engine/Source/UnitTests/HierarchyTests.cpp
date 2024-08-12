#include <Catch2/catch_amalgamated.hpp>

#include "Engine.h"

#include "GameObjects/ObjectManager.h"

#include "Levels/Level.h"

#include "glm/glm.hpp"

CATCH_TEST_CASE("ObjectParenting")
{
	CATCH_SECTION("Generic Log test")
	{
		// Setup the level
		Ball::GetLevel().GetObjectManager().Clear();
		Ball::GetLevel().Init();

		// Adding Entities
		Ball::GameObject* Entity1 = Ball::GetLevel().GetObjectManager().AddObject<Ball::GameObject>("");
		Ball::GameObject* Entity2 = Ball::GetLevel().GetObjectManager().AddObject<Ball::GameObject>("");

		Ball::GameObject* Entity2Pointer = Entity2;

		Entity1->SetParent(Entity2);
		CATCH_REQUIRE(Entity2Pointer == Entity1->GetParentedObject());

		Ball::GetLevel().GetObjectManager().Clear();
	}

	CATCH_SECTION("ParentGetsProperlyRemoved")
	{
		Ball::ObjectManager* objectManager = new Ball::ObjectManager();

		// Adding Entities
		Ball::GameObject* Entity1 = objectManager->AddObject<Ball::GameObject>("");
		Ball::GameObject* Entity2 = objectManager->AddObject<Ball::GameObject>("");

		[[maybe_unused]] Ball::GameObject* ExpectedParentPointer = nullptr;

		Entity1->SetParent(Entity2);
		Entity1->SetParent(nullptr);

		CATCH_REQUIRE(ExpectedParentPointer == Entity1->GetParentedObject());
	};

	CATCH_SECTION("LocalChildPositionGetsConvertedToWorldPosition")
	{
		Ball::ObjectManager* objectManager = new Ball::ObjectManager();

		// Adding Entities
		Ball::GameObject* Entity1 = objectManager->AddObject<Ball::GameObject>("");
		Ball::GameObject* Entity2 = objectManager->AddObject<Ball::GameObject>("");
		Ball::GameObject* Entity3 = objectManager->AddObject<Ball::GameObject>("");

		[[maybe_unused]] Ball::GameObject* ExpectedParentPointer = nullptr;

		Entity1->SetParent(Entity2);
		Entity2->SetParent(Entity3);

		glm::vec3 E1 = glm::vec3(10.f, 10.f, 5.f);
		glm::vec3 E2 = glm::vec3(20.f, 20.f, -5.f);
		glm::vec3 E3 = glm::vec3(2.f, 3.f, 10.f);

		Entity1->GetTransform().SetPosition(E1);
		Entity2->GetTransform().SetPosition(E2);
		Entity3->GetTransform().SetPosition(E3);

		glm::mat4 Trans = Entity1->MakeLocalToWorldTransform();
		glm::vec3 Pos = glm::vec3(Trans[3][0], Trans[3][1], Trans[3][2]);

		glm::vec3 result = E1 + E2 + E3;
		CATCH_REQUIRE(Pos == result);
	}
}