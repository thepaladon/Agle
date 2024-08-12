#pragma once
namespace Ball
{
	class Model;

	class AnimationController
	{
	public:
		AnimationController() = delete;
		AnimationController(Model* model) { m_AnimatedModel = model; }
		// Updates animation time and returns it
		void Update(float dt);
		void RebuildModelBlas();
		void SetModel(Model* model) { m_AnimatedModel = model; }
		float m_Speed = 1.f;
		float m_TimeOffset = 0.f;
		bool m_Paused = false;

	private:
		float m_Time = 0.f;
		Model* m_AnimatedModel = nullptr;
		bool m_AnimDirtyFlag = false;
	};
} // namespace Ball