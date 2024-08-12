#include "GameObjects/Serialization/ObjectFactory.h"

using namespace Ball;

GameObject* ObjectFactory::CreateObject(const std::string& TypeName)
{
	auto& registeredTypes = GetInstance().m_ObjectCreationFunctions;

	const auto object = registeredTypes.find(TypeName);

	ASSERT_MSG(LOG_GAMEOBJECTS,
			   (object != registeredTypes.end()),
			   "Tried to get an unregistered game object type (\"%s\").",
			   TypeName.c_str());

	return object->second();
}

bool ObjectFactory::Contains(const char* TypeName)
{
	return GetInstance().m_ObjectCreationFunctions.find(TypeName) != GetInstance().m_ObjectCreationFunctions.end();
}