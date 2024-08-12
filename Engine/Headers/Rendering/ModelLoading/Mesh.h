#pragma once
#include <vector>

namespace tinygltf
{
	class Model;
}

namespace Ball
{
	class Primitive;

	class Mesh
	{
	public:
		Mesh() = delete;
		Mesh(const tinygltf::Model& model, int index);
		~Mesh();

		const std::vector<Primitive>& GetPrimitives() const { return m_Primitives; }
		std::vector<Primitive>& GetPrimitivesRef() { return m_Primitives; }

	private:
		std::vector<Primitive> m_Primitives;
	};

} // namespace Ball