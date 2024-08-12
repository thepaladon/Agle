#include <Catch2/catch_amalgamated.hpp>

#include "ResourceManager/IResourceType.h"
#include "ResourceManager/Resource.h"
#include "ResourceManager/ResourceManager.h"

using namespace Ball;
using namespace Catch::Matchers;

constexpr int DEFAULT_RESOURCE_VALUE = 111;
constexpr const char* PATH_TO_DEFAULT_ASSET = "DamagedHelmet.gltf";

constexpr int CUSTOM_RESOURCE_VALUE = 666;
constexpr const char* PATH_TO_CUSTOM_PARAM_ASSET = "CustomHelmet.gltf";

class TestResource : public IResourceType
{
public:
	// Constructor to initialize m_resourcePath
	TestResource(const std::string& path) : IResourceType(path) {}

	TestResource(const std::string& path, int customValue) : IResourceType(path), m_SomeResourceValue(customValue) {}

protected:
	void Load() override {}
	void Unload() override {}

public:
	int m_SomeResourceValue = DEFAULT_RESOURCE_VALUE;
};

CATCH_TEST_CASE("Resource management")
{
	ResourceManager<TestResource>::Clear(); // Reset for every test.

	CATCH_SECTION("GetPaths")
	{
		CATCH_SECTION("EMPTY")
		{
			auto paths = ResourceManager<TestResource>::GetAllPaths();

			CATCH_REQUIRE(paths.empty());
		}

		CATCH_SECTION("One Path")
		{
			[[maybe_unused]] auto resource = ResourceManager<TestResource>::Load(PATH_TO_DEFAULT_ASSET);
			auto paths = ResourceManager<TestResource>::GetAllPaths();

			CATCH_REQUIRE(paths.size() == 1);

			CATCH_REQUIRE(paths[0] == FileIO::GetPath(FileIO::Engine, PATH_TO_DEFAULT_ASSET));
		}

		CATCH_SECTION("Multiple Paths")
		{
			[[maybe_unused]] auto customResource = ResourceManager<TestResource>::Load(PATH_TO_CUSTOM_PARAM_ASSET, 4);
			[[maybe_unused]] auto resource = ResourceManager<TestResource>::Load(PATH_TO_DEFAULT_ASSET);
			auto paths = ResourceManager<TestResource>::GetAllPaths();

			CATCH_REQUIRE(paths.size() == 2);

			for (int i = 0; i < 2; ++i)
			{
				CATCH_REQUIRE((paths[i] == FileIO::GetPath(FileIO::Engine, PATH_TO_DEFAULT_ASSET) ||
							   paths[i] == FileIO::GetPath(FileIO::Engine, PATH_TO_CUSTOM_PARAM_ASSET)));
			}
		}
	}

	CATCH_SECTION("ResouceManagerSize")
	{
		[[maybe_unused]] auto resource = ResourceManager<TestResource>::Load(PATH_TO_DEFAULT_ASSET);

		CATCH_REQUIRE(ResourceManager<TestResource>::Size() == 1);
		ResourceManager<TestResource>::Clear();

		CATCH_REQUIRE(ResourceManager<TestResource>::Size() == 0);
	}

	CATCH_SECTION("WITH DATA") // TODO give this a better name/grouping
	{
		struct GeneratorData
		{
			GeneratorData(bool customValue) : m_CustomValue(customValue)
			{
				if (customValue)
				{
					m_Resource = ResourceManager<TestResource>::Load(PATH_TO_CUSTOM_PARAM_ASSET, CUSTOM_RESOURCE_VALUE);
				}
				else
					m_Resource = ResourceManager<TestResource>::Load(PATH_TO_DEFAULT_ASSET);
			}

			Ball::Resource<TestResource> m_Resource{};
			bool m_CustomValue = false;
		};
		bool CustomData = GENERATE(false, true);
		GeneratorData generatedData = GeneratorData(CustomData);

		CATCH_SECTION("Loading")
		{
			CATCH_REQUIRE(generatedData.m_Resource.IsLoaded());

			int targetValue = generatedData.m_CustomValue ? CUSTOM_RESOURCE_VALUE : DEFAULT_RESOURCE_VALUE;
			CATCH_REQUIRE(generatedData.m_Resource->m_SomeResourceValue == targetValue);
		}

		CATCH_SECTION("Reloading")
		{
			constexpr int RELOAD_VALUE = 999;
			constexpr int NON_DEFAULT_VALUES = 888;
			const char* path = generatedData.m_CustomValue ? PATH_TO_CUSTOM_PARAM_ASSET : PATH_TO_DEFAULT_ASSET;

			int oldValue = generatedData.m_Resource->m_SomeResourceValue;
			generatedData.m_Resource->m_SomeResourceValue = NON_DEFAULT_VALUES;
			ResourceManager<TestResource>::Unload(path);

			CATCH_REQUIRE(!generatedData.m_Resource.IsLoaded());

			ResourceManager<TestResource>::Load(path, RELOAD_VALUE);

			CATCH_REQUIRE(generatedData.m_Resource->m_SomeResourceValue != oldValue);
			CATCH_REQUIRE(generatedData.m_Resource->m_SomeResourceValue == RELOAD_VALUE);
		}
	}

	auto resource = ResourceManager<TestResource>::Load(PATH_TO_DEFAULT_ASSET);

	CATCH_SECTION("Unloading")
	{
		ResourceManager<TestResource>::Unload(PATH_TO_DEFAULT_ASSET);

		CATCH_REQUIRE(!ResourceManager<TestResource>::IsLoaded(PATH_TO_DEFAULT_ASSET));

		CATCH_REQUIRE(!resource.IsLoaded());
	}

	CATCH_SECTION("Isloaded")
	{
		CATCH_REQUIRE(ResourceManager<TestResource>::IsLoaded(PATH_TO_DEFAULT_ASSET));

		CATCH_REQUIRE(!ResourceManager<TestResource>::IsLoaded("PathToNonExistingAsset.png"));
	}
}