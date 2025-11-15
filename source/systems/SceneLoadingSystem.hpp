#include <PCH.hpp>

#include "SceneComponents.hpp"

namespace apollo {
	class IAssetManager;
	class GameTime;

	class SceneLoadingSystem
	{
	public:
		APOLLO_API SceneLoadingSystem(IAssetManager& assetManager);
		APOLLO_API void PostInit();

		~SceneLoadingSystem() = default;

		APOLLO_API void Update(entt::registry&, const GameTime&);

		/* Returns a reference to a temporary entity world, used when loading a new scene.
		 * This is important to avoid data races, as the scene will be loaded on a separate thread
		 * and entity worlds are not thread safe
		 */
		[[nodiscard]] APOLLO_API static entt::registry& GetTempWorld() noexcept;

	private:
		enum class EState: uint8 {
			Default = 0,
			Loading,
			Finished,
		};

		IAssetManager& m_AssetManager;
		entt::entity m_SceneEntity;
		std::atomic<EState> m_State = EState::Default;
	};
} // namespace apollo