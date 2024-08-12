#include <Catch2/catch_amalgamated.hpp>

#include "Engine.h"

#include "Physics/Physics.h"
#include "Physics/CollisionPair.h"

#include "Physics/Shapes/Shape.h"
#include "Physics/Shapes/AABB.h"
#include "Physics/Shapes/TriangleShape.h"
#include "Physics/Shapes/PlaneShape.h"
#include "Physics/Shapes/SphereShape.h"

#include "GameObjects/ObjectManager.h"
#include "GameObjects/Types/MovingPlatform.h"

#include "Headers/Physics/PhysicsObject.h"
#include "Headers/Physics/PhysicsParameters.h"
#include "Headers/Physics/DynamicSpherePhysicsObject.h"

#include "glm/glm.hpp"

using namespace ::Ball;

CATCH_TEST_CASE("Triangle Collisions")
{
	CATCH_SECTION("Obvious Collision")
	{
		// Setup
		glm::vec3 v0, v1, v2;

		v0 = glm::vec3(-100.f, 12.f, -100.f);
		v1 = glm::vec3(-100.f, 12.f, 100.f);
		v2 = glm::vec3(100.f, 12.f, 100.f);

		PhysicsObject* dynamicSphereObj = new PhysicsObject(
			glm::vec3(-50.f, 12.f, 20.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(1.f), 10.f, true, PhysicsMaterial());
		dynamicSphereObj->AttachSphereShape(10.f);
		PhysicsObject* triangle = new PhysicsObject(
			glm::vec3(0.f, 0.f, 0.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(1.f), 0.f, true, PhysicsMaterial());
		triangle->AttachTriangleShape(v0, v1, v2);

		CollisionPair* pair = new CollisionPair();
		bool value = Physics::IntersectSphereWithObjects(dynamicSphereObj, triangle, 0.1f, pair);

		// Cleanup
		delete pair;
		pair = nullptr;

		delete dynamicSphereObj;
		dynamicSphereObj = nullptr;

		delete triangle;
		triangle = nullptr;

		CATCH_REQUIRE(value == true);
	}

	CATCH_SECTION("No Collision")
	{
		// Setup
		glm::vec3 v0, v1, v2;

		v0 = glm::vec3(-100.f, 12.f, -100.f);
		v1 = glm::vec3(-100.f, 12.f, 100.f);
		v2 = glm::vec3(100.f, 12.f, 100.f);

		PhysicsObject* dynamicSphereObj = new PhysicsObject(glm::vec3(-120.f, 12.f, 20.f),
															glm::quat(1.f, 0.f, 0.f, 0.f),
															glm::vec3(1.f),
															10.f,
															true,
															PhysicsMaterial());
		dynamicSphereObj->AttachSphereShape(10.f);
		PhysicsObject* triangle = new PhysicsObject(
			glm::vec3(0.f, 0.f, 0.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(1.f), 0.f, true, PhysicsMaterial());
		triangle->AttachTriangleShape(v0, v1, v2);

		CollisionPair* pair = new CollisionPair();

		bool value = Physics::IntersectSphereWithObjects(dynamicSphereObj, triangle, 0.1f, pair);

		// Cleanup
		delete pair;
		pair = nullptr;

		delete dynamicSphereObj;
		dynamicSphereObj = nullptr;

		delete triangle;
		triangle = nullptr;

		CATCH_REQUIRE(value == false);
	}
}
