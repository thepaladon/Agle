#include <Catch2/catch_amalgamated.hpp>

#include "LevelEditor/ActionStack.h"

// Dummy action for testing the functionality of the Action list
class DummyAction : public Ball::Editor::Action
{
public:
	int* m_value = nullptr;

public:
	DummyAction(int& value) : Action(nullptr, false) { m_value = &value; }

	~DummyAction() override {}

	void Execute() override { *m_value += 1; }

	void Undo() override { *m_value -= 1; }
};

CATCH_TEST_CASE("Action list tests")
{
	CATCH_SECTION("Create new present")
	{
		int value = 0;
		Ball::Editor::ActionStack list = Ball::Editor::ActionStack();

		list.PushAction<DummyAction>(value);
		list.PushAction<DummyAction>(value);
		list.PushAction<DummyAction>(value);
		list.UndoAction();
		list.UndoAction();
		list.PushAction<DummyAction>(value);
		list.RedoAction();

		CATCH_REQUIRE(value == 2);
	}

	CATCH_SECTION("Undo without actions")
	{
		int value = 0;
		Ball::Editor::ActionStack list = Ball::Editor::ActionStack();

		for (int i = 0; i < 5; i++)
			list.UndoAction();

		CATCH_REQUIRE(value == 0);
	}

	CATCH_SECTION("Redo without actions")
	{
		int value = 0;
		Ball::Editor::ActionStack list = Ball::Editor::ActionStack();

		for (int i = 0; i < 10; i++)
			list.RedoAction();

		CATCH_REQUIRE(value == 0);
	}

	CATCH_SECTION("Undo overflow")
	{
		int value = 0;
		Ball::Editor::ActionStack list = Ball::Editor::ActionStack();

		for (int i = 0; i < 5; i++)
			list.PushAction<DummyAction>(value);

		for (int i = 0; i < 10; i++)
			list.UndoAction();

		CATCH_REQUIRE(value == 0);
	}

	CATCH_SECTION("Redo overflow")
	{
		int value = 0;
		Ball::Editor::ActionStack list = Ball::Editor::ActionStack();

		for (int i = 0; i < 5; i++)
			list.PushAction<DummyAction>(value);

		for (int i = 0; i < 5; i++)
			list.UndoAction();

		for (int i = 0; i < 15; i++)
			list.RedoAction();

		CATCH_REQUIRE(value == 5);
	}

	CATCH_SECTION("Action overflow")
	{
		int value = 0;
		Ball::Editor::ActionStack list = Ball::Editor::ActionStack();

		for (int i = 0; i < 7; i++)
			list.PushAction<DummyAction>(value);

		for (int i = 0; i < 5; i++)
			list.UndoAction();

		for (int i = 0; i < 5; i++)
			list.RedoAction();

		CATCH_REQUIRE(value == 7);
	}
}