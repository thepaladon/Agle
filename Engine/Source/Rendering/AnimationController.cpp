#include "Rendering/AnimationController.h"
#include "Rendering//ModelLoading/Model.h"
namespace Ball
{
	void AnimationController::Update(float dt)
	{
		m_AnimDirtyFlag = false;
		if (!m_Paused && m_Speed != 0.f)
		{
			m_Time += m_Speed * (dt);
			if (m_AnimatedModel != nullptr)
			{
				m_AnimatedModel->UpdateAnimations(m_TimeOffset + m_Time);
				m_AnimDirtyFlag = true;
			}
		}
	}

	void AnimationController::RebuildModelBlas()
	{
		if (m_AnimDirtyFlag)
		{
			if (m_AnimatedModel != nullptr)
			{
				m_AnimatedModel->RebuildBlas();
			}
		}
	}
} // namespace Ball