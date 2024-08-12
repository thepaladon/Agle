#include <Catch2/catch_amalgamated.hpp>

#include "Engine.h"

#include "Physics/Physics.h"

#include "GameObjects/ObjectManager.h"
#include "GameObjects/Types/MovingPlatform.h"

#include "GameObjects/Types/Cube.h"
#include "Headers/Physics/PhysicsObject.h"
#include "Levels/Level.h"
#include "Headers/Physics/PhysicsParameters.h"
#include "Physics/CollisionPair.h"
#include "GameObjects/Serialization/PrefabReader.h"
#include "Headers/Rendering/ModelLoading/ModelManager.h"
#include "Physics/Shapes/AABB.h"

#include "glm/glm.hpp"

using namespace ::Ball;

CATCH_TEST_CASE("RayCasts")
{
	CATCH_SECTION("RayCast to Sphere")
	{
		PhysicsObject* m_StaticSphereObj = new PhysicsObject(
			glm::vec3(-5.f, -5.f, -5.f), glm::quat(1.f, 0.f, 0.f, 0.f), glm::vec3(1.f), 0.f, true, PhysicsMaterial());
		m_StaticSphereObj->AttachSphereShape(10.f);

		glm::vec3 rayStart = glm::vec3(-5.f, 9.f, -5.f);
		glm::vec3 rayDirection = glm::vec3(0.f, -1.f, 0.f);
		float rayDistance = 5.f;

		// Check rayCast
		bool value = Physics::IntersectRayWithObject(rayStart, rayDirection, rayDistance, m_StaticSphereObj);

		// Cleanup
		delete m_StaticSphereObj;
		m_StaticSphereObj = nullptr;

		CATCH_REQUIRE(value == true);
	}

	CATCH_SECTION("RayCast to Triangle")
	{
		glm::vec3 v0 = glm::vec3(-100.f, 0.f, -100.f);
		glm::vec3 v1 = glm::vec3(-100.f, 0.f, 100.f);
		glm::vec3 v2 = glm::vec3(100.f, 0.f, 100.f);

		glm::vec3 rayOrigin = glm::vec3(0.f, 30.f, 50.f);
		glm::vec3 rayDirection = glm::vec3(0.f, -40.f, 0.f);

		float intersection = 0;
		glm::vec3 outNormal = glm::vec3(0.f);

		bool value = Physics::RayTriangleIntersection(rayOrigin, rayDirection, v0, v1, v2, intersection, outNormal);

		CATCH_REQUIRE(value == true);
	}

	CATCH_SECTION("RayCast to Triangle Fail ray")
	{
		glm::vec3 v0 = glm::vec3(-100.f, 0.f, -100.f);
		glm::vec3 v1 = glm::vec3(-100.f, 0.f, 100.f);
		glm::vec3 v2 = glm::vec3(100.f, 0.f, 100.f);

		glm::vec3 rayOrigin = glm::vec3(0.f, 30.f, 110.f);
		glm::vec3 rayDirection = glm::vec3(0.f, -40.f, 0.f);

		float intersection = 0;
		glm::vec3 outNormal = glm::vec3(0.f);

		bool value = Physics::RayTriangleIntersection(rayOrigin, rayDirection, v0, v1, v2, intersection, outNormal);

		CATCH_REQUIRE(value == false);
	}

	CATCH_SECTION("RayCast to Triangle Fail length")
	{
		glm::vec3 v0 = glm::vec3(-100.f, 0.f, -100.f);
		glm::vec3 v1 = glm::vec3(-100.f, 0.f, 100.f);
		glm::vec3 v2 = glm::vec3(100.f, 0.f, 100.f);

		glm::vec3 rayOrigin = glm::vec3(0.f, 30.f, 50.f);
		glm::vec3 rayDirection = glm::vec3(0.f, -20.f, 0.f);

		float intersection = 0;
		glm::vec3 outNormal = glm::vec3(0.f);

		Physics::RayTriangleIntersection(rayOrigin, rayDirection, v0, v1, v2, intersection, outNormal);

		CATCH_REQUIRE(intersection > 1);
	}

	CATCH_SECTION("RayCast to Triangle Normal")
	{
		glm::vec3 v0 = glm::vec3(-100.f, 0.f, -100.f);
		glm::vec3 v1 = glm::vec3(-100.f, 0.f, 100.f);
		glm::vec3 v2 = glm::vec3(100.f, 0.f, 100.f);

		glm::vec3 rayOrigin = glm::vec3(0.f, 30.f, 50.f);
		glm::vec3 rayDirection = glm::vec3(0.f, -40.f, 0.f);

		float intersection = 0;
		glm::vec3 outNormal = glm::vec3(0.f);

		Physics::RayTriangleIntersection(rayOrigin, rayDirection, v0, v1, v2, intersection, outNormal);
		CATCH_REQUIRE(outNormal == glm::vec3(0.f, 1.f, 0.f));
	}

	CATCH_SECTION("RayCast to Capsule")
	{
		glm::vec3 rayOrigin = glm::vec3(0.f, 30.f, 0.f);
		glm::vec3 rayDirection = glm::vec3(0.f, -50.f, 0.f);

		glm::vec3 capsulePoint0 = glm::vec3(-10.f, 0.f, -10.f);
		glm::vec3 capsulePoint1 = glm::vec3(10.f, 0.f, 10.f);

		float capsuleRadius = 5.f;

		float intersection0, intersection1;

		bool value = Physics::RayCapsuleIntersection(
			rayOrigin, rayDirection, capsulePoint0, capsulePoint1, capsuleRadius, intersection0, intersection1);
		CATCH_REQUIRE(value == true);
	}

	CATCH_SECTION("RayCast to Capsule Fail")
	{
		glm::vec3 rayOrigin = glm::vec3(0.f, 30.f, 0.f);
		glm::vec3 rayDirection = glm::vec3(0.f, 50.f, 0.f);

		glm::vec3 capsulePoint0 = glm::vec3(-10.f, 0.f, -10.f);
		glm::vec3 capsulePoint1 = glm::vec3(10.f, 0.f, -10.f);

		float capsuleRadius = 5.f;

		float intersection0, intersection1;

		bool value = Physics::RayCapsuleIntersection(
			rayOrigin, rayDirection, capsulePoint0, capsulePoint1, capsuleRadius, intersection0, intersection1);
		CATCH_REQUIRE(value == false);
	}

	CATCH_SECTION("RayCast to AABB")
	{
		glm::vec3 rayOrigin = glm::vec3(0.f, 3000.f, 0.f);
		glm::vec3 rayDirection = glm::vec3(0.f, -5.f, 0.f);

		Ball::AABB box = Ball::AABB(glm::vec3(-5.f), glm::vec3(5.f));

		float i0 = FLT_MIN;
		float i1 = FLT_MAX;

		bool value = Physics::RayAABBIntersection(rayOrigin, rayDirection, box, i0, i1);

		CATCH_REQUIRE(value == true);
	}

	CATCH_SECTION("RayCast to AABB fail")
	{
		glm::vec3 rayOrigin = glm::vec3(0.f, 30.f, 0.f);
		glm::vec3 rayDirection = glm::vec3(0.f, 5.f, 0.f);

		Ball::AABB box = Ball::AABB(glm::vec3(-5.f), glm::vec3(5.f));

		float i0 = FLT_MIN;
		float i1 = FLT_MAX;

		bool value = Physics::RayAABBIntersection(rayOrigin, rayDirection, box, i0, i1);

		CATCH_REQUIRE(value == false);
	}
}
