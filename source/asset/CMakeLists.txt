file(GLOB ASSET_HEADERS *.hpp *.h)
target_sources(${PROJECT_NAME}Runtime PRIVATE
	Asset.cpp
	AssetLoader.cpp
	AssetRef.cpp
	AssetManager.cpp
	Scene.cpp
	${ASSET_HEADERS}
)