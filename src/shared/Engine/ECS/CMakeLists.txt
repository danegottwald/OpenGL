add_library(OpenGLCore_ECS STATIC)

target_sources(OpenGLCore_ECS PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/Registry.h
    ${CMAKE_CURRENT_LIST_DIR}/EventQueue.h
    ${CMAKE_CURRENT_LIST_DIR}/ISystem.h
    ${CMAKE_CURRENT_LIST_DIR}/SystemScheduler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/SystemScheduler.h
    ${CMAKE_CURRENT_LIST_DIR}/Components.h
    ${CMAKE_CURRENT_LIST_DIR}/BlockEntityRegistry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/BlockEntityRegistry.h
    ${CMAKE_CURRENT_LIST_DIR}/BlockInteractionEvents.h
    ${CMAKE_CURRENT_LIST_DIR}/Systems/BlockInteractionPipeline.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Systems/BlockInteractionPipeline.h
    ${CMAKE_CURRENT_LIST_DIR}/Systems/BlockInteractionSystems.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Systems/BlockInteractionSystems.h
    ${CMAKE_CURRENT_LIST_DIR}/Systems/FurnaceSystem.cpp
    ${CMAKE_CURRENT_LIST_DIR}/Systems/FurnaceSystem.h
    ${CMAKE_CURRENT_LIST_DIR}/Resources/BlockInteractionResource.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/BlockInteraction.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Camera.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Children.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Health.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Input.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Inventory.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/ItemDrop.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Mesh.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Parent.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Physics.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Projectile.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Tags.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Transform.h
    ${CMAKE_CURRENT_LIST_DIR}/Components/Velocity.h
)

target_include_directories(OpenGLCore_ECS PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/shared
    ${CMAKE_SOURCE_DIR}/src/client
    ${CMAKE_SOURCE_DIR}/include
)

target_precompile_headers(OpenGLCore_ECS PRIVATE ${CMAKE_SOURCE_DIR}/src/pch_shared.h)

target_link_libraries(OpenGLCore_ECS PUBLIC
    glm::glm-header-only
)
