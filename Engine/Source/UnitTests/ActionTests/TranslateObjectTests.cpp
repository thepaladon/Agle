#include <Catch2/catch_amalgamated.hpp>

#include "LevelEditor/Actions/TranslateObject.h"

CATCH_TEST_CASE("Translate object unit tests")
{
	Ball::GameObject* object = new Ball::GameObject();

	Ball::Transform t1 = object->GetTransform();
	Ball::Transform t2 = {};
	t1.SetPosition(glm::vec3(1.f, 2.f, 3.f));
	t1.SetRotation(glm::vec3(4.f, 5.f, 6.f));
	t1.SetScale(glm::vec3(7.f, 8.f, 9.f));

	Ball::TranslateObject moveAction = Ball::TranslateObject(object, t1, t2);

	CATCH_SECTION("Execute operation")
	{
		moveAction.Execute();

		CATCH_REQUIRE(object->GetTransform().GetPosition() != t1.GetPosition());
		CATCH_REQUIRE(object->GetTransform().GetRotation() != t1.GetRotation());
		CATCH_REQUIRE(object->GetTransform().GetScale() != t1.GetScale());

		CATCH_REQUIRE(object->GetTransform().GetPosition() == t2.GetPosition());
		CATCH_REQUIRE(object->GetTransform().GetRotation() == t2.GetRotation());
		CATCH_REQUIRE(object->GetTransform().GetScale() == t2.GetScale());
	}

	CATCH_SECTION("Undo operation")
	{
		moveAction.Execute();
		moveAction.Undo();

		CATCH_REQUIRE(object->GetTransform().GetPosition() == t1.GetPosition());
		CATCH_REQUIRE(object->GetTransform().GetRotation() == t1.GetRotation());
		CATCH_REQUIRE(object->GetTransform().GetScale() == t1.GetScale());

		CATCH_REQUIRE(object->GetTransform().GetPosition() != t2.GetPosition());
		CATCH_REQUIRE(object->GetTransform().GetRotation() != t2.GetRotation());
		CATCH_REQUIRE(object->GetTransform().GetScale() != t2.GetScale());
	}
	delete object;
}
