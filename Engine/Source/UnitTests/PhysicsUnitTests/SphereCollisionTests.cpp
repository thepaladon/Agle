#include <Catch2/catch_amalgamated.hpp>

#include "Engine.h"

#include "Physics/Physics.h"
#include "Physics/CollisionPair.h"

#include "GameObjects/ObjectManager.h"
#include "GameObjects/Types/MovingPlatform.h"

#include "Headers/Physics/PhysicsObject.h"
#include "Headers/Physics/PhysicsParameters.h"
#include "Headers/Physics/DynamicSpherePhysicsObject.h"

#include "glm/glm.hpp"

using namespace ::Ball;

CATCH_TEST_CASE("SphereCollisions")
{
	CATCH_SECTION("Obvious Collision")
	{
		// Setup
		PhysicsObject* m_DynamicSphereObj = new PhysicsObject(
			glm::vec3(0.f, 8.f, 0.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(1.f), 10.f, true, PhysicsMaterial());
		m_DynamicSphereObj->AttachSphereShape(10.f);
		PhysicsObject* m_StaticSphereObj = new PhysicsObject(
			glm::vec3(-2.f, -5.f, 0.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(1.f), 0.f, true, PhysicsMaterial());
		m_StaticSphereObj->AttachSphereShape(20.f);

		CollisionPair* pair = new CollisionPair();
		bool value = Physics::IntersectSphereWithObjects(m_DynamicSphereObj, m_StaticSphereObj, 0.1f, pair);

		// Cleanup
		delete pair;
		pair = nullptr;

		delete m_StaticSphereObj;
		m_StaticSphereObj = nullptr;

		delete m_DynamicSphereObj;
		m_DynamicSphereObj = nullptr;

		CATCH_REQUIRE(value == true);
	}

	CATCH_SECTION("No Collision")
	{
		// Setup
		PhysicsObject* m_DynamicSphereObj = new PhysicsObject(
			glm::vec3(0.f, 0.f, 0.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(1.f), 10.f, true, PhysicsMaterial());
		m_DynamicSphereObj->AttachSphereShape(10.f);
		PhysicsObject* m_StaticSphereObj = new PhysicsObject(
			glm::vec3(-31.f, -5.f, 0.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(1.f), 0.f, true, PhysicsMaterial());
		m_StaticSphereObj->AttachSphereShape(20.f);

		CollisionPair* pair = new CollisionPair();
		bool value = Physics::IntersectSphereWithObjects(m_DynamicSphereObj, m_StaticSphereObj, 0.1f, pair);

		// Cleanup
		delete pair;
		pair = nullptr;

		delete m_StaticSphereObj;
		m_StaticSphereObj = nullptr;

		delete m_DynamicSphereObj;
		m_DynamicSphereObj = nullptr;

		CATCH_REQUIRE(value == false);
	}

	CATCH_SECTION("Continuous Intersection")
	{
		DynamicSpherePhysicsObject* m_DynamicSphereObj = new DynamicSpherePhysicsObject(
			glm::vec3(0.f, 40.f, 0.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(1.f), 10.f, true, PhysicsMaterial());
		m_DynamicSphereObj->AttachSphereShape(10.f);
		m_DynamicSphereObj->Initialize();
		PhysicsObject* m_StaticSphereObj = new PhysicsObject(
			glm::vec3(-2.f, -5.f, 0.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(1.f), 0.f, true, PhysicsMaterial());
		m_StaticSphereObj->AttachSphereShape(20.f);

		// Set Velocity
		m_DynamicSphereObj->ApplyLinearImpulse(glm::vec3(0.f, -100.f, 0.f));

		CollisionPair* pair = new CollisionPair();
		[[maybe_unused]] bool value =
			Physics::IntersectSphereWithObjects(m_DynamicSphereObj, m_StaticSphereObj, 10.f, pair);

		// Cleanup
		delete pair;
		pair = nullptr;

		delete m_StaticSphereObj;
		m_StaticSphereObj = nullptr;

		delete m_DynamicSphereObj;
		m_DynamicSphereObj = nullptr;

		CATCH_REQUIRE(value == true);
	}
}
