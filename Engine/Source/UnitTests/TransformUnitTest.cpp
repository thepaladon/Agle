#include "Catch2/catch_amalgamated.hpp"
#include "Transform.h"

CATCH_TEST_CASE("Transform")
{
	CATCH_SECTION("Initialization")
	{
		Ball::Transform transform;

		// Check if the newly created transform has a identity matrix
		CATCH_REQUIRE(transform.GetModelMatrix() == glm::mat4(1));
	}

	CATCH_SECTION("Inverse model matrix")
	{
		Ball::Transform transform;

		// Inverse identity matrix
		glm::mat4 inverseMatrix = glm::inverse(glm::mat4(1));

		CATCH_REQUIRE(transform.GetInverseModelMatrix() == inverseMatrix);
	}

	CATCH_SECTION("Position Setter and Getter")
	{
		const auto value = glm::vec3(5.2f, 10.7f, 20.1f);

		Ball::Transform transform;

		transform.SetPosition(value);

		auto result = glm::epsilonEqual(transform.GetPosition(), value, 0.001f);
		CATCH_REQUIRE((result.x && result.y && result.z));
	}

	CATCH_SECTION("Position Setter and Getter using floats")
	{
		const auto value = glm::vec3(5.6f, 10.3f, 20.9f);

		Ball::Transform transform;

		transform.SetPosition(value.x, value.y, value.z);

		auto result = glm::epsilonEqual(transform.GetPosition(), value, 0.001f);
		CATCH_REQUIRE((result.x && result.y && result.z));
	}

	CATCH_SECTION("Position Translate function")
	{
		const auto startPos = glm::vec3(5.1f, 10.6f, 20.2f);
		const auto translateValue = glm::vec3(10.5f, 20.9f, 5.3f);
		const auto Result = glm::vec3(15.6f, 31.5f, 25.5f);

		Ball::Transform transform;

		transform.SetPosition(startPos);

		transform.Translate(translateValue);

		auto result = glm::epsilonEqual(transform.GetPosition(), Result, 0.001f);
		CATCH_REQUIRE((result.x && result.y && result.z));
	}

	CATCH_SECTION("Position Translate function using floats")
	{
		const auto startPos = glm::vec3(2.4f, 17.9f, 2.4f);
		const auto translateValue = glm::vec3(6.5f, 2.9f, 8.6f);
		const auto Result = glm::vec3(8.9f, 20.8f, 11.0f);

		Ball::Transform transform;

		transform.SetPosition(startPos);

		transform.Translate(translateValue.x, translateValue.y, translateValue.z);

		auto result = glm::epsilonEqual(transform.GetPosition(), Result, 0.001f);
		CATCH_REQUIRE((result.x && result.y && result.z));
	}

	CATCH_SECTION("Rotation Setter and Getter")
	{
		const auto value = glm::vec3(5.7f, 10.9f, 20.4f);

		Ball::Transform transform;

		transform.SetRotation(value);

		auto result = glm::epsilonEqual(transform.GetRotation(), glm::quat(value), 0.001f);
		CATCH_REQUIRE((result.x && result.y && result.z && result.w));
	}

	CATCH_SECTION("Rotation Rotate Function")
	{
		const auto start = glm::vec3(4.7f, 1.6f, 9.6f);
		const auto value = glm::vec3(15.3f, 13.7f, 24.3f);

		Ball::Transform transform;

		// Test rotation on local space
		transform.SetRotation(start);
		transform.RotateLocal(value);
		// Calculate the resulting rotation using glm
		glm::quat rotationResultLocal = glm::quat(start) * glm::quat(value);

		auto resultLocal = glm::epsilonEqual(transform.GetRotation(), rotationResultLocal, 0.001f);
		CATCH_REQUIRE((resultLocal.x && resultLocal.y && resultLocal.z && resultLocal.w));

		// Test rotation on global space
		transform.SetRotation(start);
		transform.RotateGlobal(value);
		// Calculate the resulting rotation using glm
		glm::quat rotationResultGlobal = glm::quat(value) * glm::quat(start);

		auto resultGlobal = glm::epsilonEqual(transform.GetRotation(), rotationResultGlobal, 0.001f);
		CATCH_REQUIRE((resultGlobal.x && resultGlobal.y && resultGlobal.z && resultGlobal.w));
	}

	CATCH_SECTION("Rotation AngleAxis Function")
	{
		const auto start = glm::vec3(4.7f, 1.6f, 9.6f);
		const auto value = 2.6f;

		Ball::Transform transform;

		// Test angle axis on local space
		transform.SetRotation(start);
		transform.AngleAxisLocal(value, glm::vec3(0, 1, 0));
		// Calculate the resulting rotation using glm
		glm::quat rotationResultLocal = glm::quat(start) * glm::angleAxis(value, glm::vec3(0, 1, 0));

		auto resultLocal = glm::epsilonEqual(transform.GetRotation(), rotationResultLocal, 0.001f);
		CATCH_REQUIRE((resultLocal.x && resultLocal.y && resultLocal.z && resultLocal.w));

		// Test angle axis on global space
		transform.SetRotation(start);
		transform.AngleAxisGlobal(value, glm::vec3(0, 1, 0));
		// Calculate the resulting rotation using glm
		glm::quat rotationResultGlobal = glm::angleAxis(value, glm::vec3(0, 1, 0)) * glm::quat(start);

		auto resultGlobal = glm::epsilonEqual(transform.GetRotation(), rotationResultGlobal, 0.001f);
		CATCH_REQUIRE((resultGlobal.x && resultGlobal.y && resultGlobal.z && resultGlobal.w));
	}

	CATCH_SECTION("Scale Setter and Getter")
	{
		const auto value = glm::vec3(2.1f, 3.6f, 1.7f);

		Ball::Transform transform;

		transform.SetScale(value);

		auto result = glm::epsilonEqual(transform.GetScale(), value, 0.001f);
		CATCH_REQUIRE((result.x && result.y && result.z));
	}

	CATCH_SECTION("Scale Setter and Getter using floats")
	{
		const auto value = glm::vec3(5.8f, 1.4f, 7.5f);

		Ball::Transform transform;

		transform.SetScale(value.x, value.y, value.z);

		auto result = glm::epsilonEqual(transform.GetScale(), value, 0.001f);
		CATCH_REQUIRE((result.x && result.y && result.z));
	}

	CATCH_SECTION("Euler Angles Setter and Getter")
	{
		const auto value = glm::vec3(2.1f, 3.6f, 1.7f);

		Ball::Transform transform;

		transform.SetEulerAngles(value);

		// Create a quaternion from the euler angles
		glm::quat quat = glm::quat(value);
		// Get the euler angles from the quaternion
		glm::vec3 quatEuler = glm::eulerAngles(quat);

		auto result = glm::epsilonEqual(transform.GetEulerAngles(), quatEuler, 0.001f);
		CATCH_REQUIRE((result.x && result.y && result.z));
	}

	CATCH_SECTION("Euler Angles Setter and Getter using floats")
	{
		const auto value = glm::vec3(6.7f, 32.3f, 14.4f);

		Ball::Transform transform;

		transform.SetEulerAngles(value.x, value.y, value.z);

		// Create a quaternion from the euler angles
		glm::quat quat = glm::quat(value);
		// Get the euler angles from the quaternion
		glm::vec3 quatEuler = glm::eulerAngles(quat);

		auto result = glm::epsilonEqual(transform.GetEulerAngles(), quatEuler, 0.001f);
		CATCH_REQUIRE((result.x && result.y && result.z));
	}

	CATCH_SECTION("Directional vectors")
	{
		Ball::Transform transform;

		CATCH_REQUIRE(transform.Forward() == glm::vec3(0, 0, 1));
		CATCH_REQUIRE(transform.Right() == glm::vec3(1, 0, 0));
		CATCH_REQUIRE(transform.Up() == glm::vec3(0, 1, 0));
	}
}
