file(GLOB ECS_HEADERS *.h *.hpp)
target_sources(${PROJECT_NAME}Runtime PRIVATE
	"Manager.cpp"
	"System.cpp"
	${ECS_HEADERS}
)