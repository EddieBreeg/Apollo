file(GLOB ASSET_HEADERS *.hpp *.h)
target_sources(${PROJECT_NAME}Runtime PRIVATE
	AssetLoader.cpp
	Asset.cpp
	AssetManager.cpp
	${ASSET_HEADERS}
)