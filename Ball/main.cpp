#include "Engine.h"
#include "GraphicsScene.h"
#include "Utilities/LaunchParameters.h"

#if defined(SHIPPING) && defined(PLATFORM_WINDOWS)
int WinMain()
#else
int main()
#endif
{
	int argc = Ball::LaunchParameters::GetArgc();
	char** argv = Ball::LaunchParameters::GetArgv();

	Ball::LaunchParameters::SetParameters(std::vector<std::string>(argv + 1, argv + argc));

	Ball::ApplicationConfig config;

	config.m_Game = static_cast<Ball::BaseGame*>(new GraphicsScene());

	config.m_Width = Ball::LaunchParameters::GetInt("WindowWidth", 1000);
	config.m_Height = Ball::LaunchParameters::GetInt("WindowHeight", 600);
	config.m_Title = "OffTheBubble";

	return Ball::GetEngine().Run(config);
}