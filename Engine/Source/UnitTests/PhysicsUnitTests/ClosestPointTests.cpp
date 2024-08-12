#include <Catch2/catch_amalgamated.hpp>

#include "Engine.h"

#include "Physics/Physics.h"
#include "Physics/Shapes/Shape.h"
#include "Physics/Shapes/AABB.h"
#include "Physics/Shapes/TriangleShape.h"
#include "Physics/Shapes/PlaneShape.h"

#include "GameObjects/ObjectManager.h"
#include "GameObjects/Types/MovingPlatform.h"

#include "Headers/Physics/PhysicsObject.h"
#include "Headers/Physics/PhysicsParameters.h"

#include "glm/glm.hpp"

using namespace ::Ball;

CATCH_TEST_CASE("Closest Point")
{
	CATCH_SECTION("AABB Closest Point Checks")
	{
		CATCH_SECTION("Closest Point to AABB minimum")
		{
			AABB* box = new AABB(glm::vec3(-10.f, -10.f, -10.f), glm::vec3(10.f, 10.f, 10.f));

			glm::vec3 closestPoint = Physics::GetClosestPointOnAABB(glm::vec3(-INFINITY), *box);
			glm::vec3 expectedOutcome = glm::vec3(-10.f);

			// Cleanup
			delete box;
			box = nullptr;

			CATCH_REQUIRE(closestPoint == expectedOutcome);
		}

		CATCH_SECTION("Closest Point to AABB maximum")
		{
			AABB* box = new AABB(glm::vec3(-10.f, -10.f, -10.f), glm::vec3(10.f, 10.f, 10.f));

			glm::vec3 closestPoint = Physics::GetClosestPointOnAABB(glm::vec3(INFINITY), *box);
			glm::vec3 expectedOutcome = glm::vec3(10.f);

			// Cleanup
			delete box;
			box = nullptr;

			CATCH_REQUIRE(closestPoint == expectedOutcome);
		}

		CATCH_SECTION("Point anywhere to AABB")
		{
			AABB* box = new AABB(glm::vec3(-10.f, -10.f, -10.f), glm::vec3(10.f, 10.f, 10.f));

			glm::vec3 closestPoint = Physics::GetClosestPointOnAABB(glm::vec3(3.f, 15.f, -3.f), *box);
			glm::vec3 expectedOutcome = glm::vec3(3.f, 10.f, -3.f);

			// Cleanup
			delete box;
			box = nullptr;

			CATCH_REQUIRE(closestPoint == expectedOutcome);
		}
	}

	CATCH_SECTION("Triangle Closest Point Checks")
	{
		CATCH_SECTION("Closest Point Vertex")
		{
			glm::vec3 point = glm::vec3(-10000.f, -10000.f, -1000.f);
			glm::vec3 v0, v1, v2;

			v0 = glm::vec3(-100.f, 12.f, -100.f);
			v1 = glm::vec3(-100.f, 12.f, 100.f);
			v2 = glm::vec3(100.f, 12.f, 100.f);

			bool isFace = false;
			TriangleShape triangle = TriangleShape(v0, v1, v2);
			glm::vec3 closestPoint = Physics::GetClosestPointOnTriangle(point, triangle, isFace);

			CATCH_REQUIRE(closestPoint == v0);
		}

		CATCH_SECTION("Closest Point Face")
		{
			glm::vec3 point = glm::vec3(-50.f, 14.f, 20.f);
			glm::vec3 v0, v1, v2;

			v0 = glm::vec3(-100.f, 12.f, -100.f);
			v1 = glm::vec3(-100.f, 12.f, 100.f);
			v2 = glm::vec3(100.f, 12.f, 100.f);

			bool isFace = false;
			TriangleShape triangle = TriangleShape(v0, v1, v2);
			glm::vec3 closestPoint = Physics::GetClosestPointOnTriangle(point, triangle, isFace);

			CATCH_REQUIRE(closestPoint == glm::vec3(-50.f, 12.f, 20.f));
		}
	}

	CATCH_SECTION("Plane Closest Point Checks")
	{
		CATCH_SECTION("Closest Point Plane")
		{
			glm::vec3 point = glm::vec3(-50.f, 14.f, 20.f);
			glm::vec3 v0, v1, v2;

			v0 = glm::vec3(-100.f, 12.f, -100.f);
			v1 = glm::vec3(-100.f, 12.f, 100.f);
			v2 = glm::vec3(100.f, 12.f, 100.f);

			TriangleShape triangle = TriangleShape(v0, v1, v2);
			PlaneShape plane = PlaneShape::CreatePlaneFromTriangle(triangle);
			glm::vec3 closestPoint = Physics::GetClosestPointOnPlane(point, plane);

			CATCH_REQUIRE(closestPoint == glm::vec3(-50.f, 12.f, 20.f));
		}
	}

	CATCH_SECTION("Line Closest Point Checks")
	{
		CATCH_SECTION("Closest Point Line Start")
		{
			glm::vec3 point = glm::vec3(-5.f, 5.f, -10.f);
			glm::vec3 lineStart, lineEnd;

			lineStart = glm::vec3(0.f, 0.f, 0.f);
			lineEnd = glm::vec3(0.f, 0.f, 1000.f);

			glm::vec3 closestPoint = Physics::GetClosestPointOnLine(point, lineStart, lineEnd);

			CATCH_REQUIRE(closestPoint == glm::vec3(0.f));
		}

		CATCH_SECTION("Closest Point On Line")
		{
			glm::vec3 point = glm::vec3(20.f, 5.f, 200.f);
			glm::vec3 lineStart, lineEnd;

			lineStart = glm::vec3(0.f, 0.f, 0.f);
			lineEnd = glm::vec3(0.f, 0.f, 1000.f);

			glm::vec3 closestPoint = Physics::GetClosestPointOnLine(point, lineStart, lineEnd);

			CATCH_REQUIRE(closestPoint == glm::vec3(0.f, 0.f, 200.f));
		}
	}
}
