#pragma once

namespace Ball
{
	class SerializeArchive;
	class IObject
	{
	public:
		virtual ~IObject() {}

		virtual void Init(){};
		virtual void Shutdown(){};

		virtual void Update(float deltaTime) = 0;

		virtual void Serialize(SerializeArchive& archive){};
	};
} // namespace Ball